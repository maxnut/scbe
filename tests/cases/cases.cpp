#include "cases.hpp"
#include "context.hpp"
#include "opt_level.hpp"

#include <filesystem>
#include <fstream>

#define BUILD_FOLDER "tests/"

using namespace scbe;

Unit createUnit(const std::string& name) {
    auto ctx = std::make_shared<Context>();
    ctx->registerAllTargets();
    return Unit(name, ctx);
}

std::string archString(Target::Arch arch) {
    switch(arch) {
        case scbe::Target::Arch::x86_64: return "x86_64";
        case scbe::Target::Arch::AArch64: return "aarch64";
        default: break;
    }
    return "unknown";
}

std::optional<std::string> compileUnit(Unit& unit, Target::TargetSpecification spec, int debug) {
    std::filesystem::create_directories(BUILD_FOLDER);
    
    Ref<Target::Target> target = unit.getContext()->getTargetRegistry().getTarget(spec);

    auto machine = target->getTargetMachine(spec, unit.getContext());
    unit.setDataLayout(machine->getDataLayout());

    std::string filePrefix = unit.getName() + "_" + archString(spec.getArch()) + "_" + std::to_string(debug);
    std::string prefix = BUILD_FOLDER + filePrefix;

    if(spec.getArch() == Target::Arch::x86_64) {
        std::ofstream objOut(prefix + ".o");
        auto passManager = std::make_shared<PassManager>();
        machine->addPassesForCodeGeneration(passManager, objOut, Target::FileType::ObjectFile, (scbe::OptimizationLevel)debug);
        passManager->run(unit);
        objOut.close();

        std::string lkCmd = "gcc -m64 " + prefix + ".o -o " + prefix + ".out";
        if(std::system(lkCmd.c_str()) != 0) return std::nullopt;
    }
    else if(spec.getArch() == Target::Arch::AArch64) {
        auto passManager = std::make_shared<PassManager>();
        std::ofstream asmOut(prefix + ".s");
        machine->addPassesForCodeGeneration(passManager, asmOut, Target::FileType::AssemblyFile, (scbe::OptimizationLevel)debug);
        passManager->run(unit);
        asmOut.close();

        std::string asCmd = "aarch64-linux-gnu-as " + prefix + ".s -o " + prefix + ".o";
        if(std::system(asCmd.c_str()) != 0) return std::nullopt;

        std::string lkCmd = "aarch64-linux-gnu-gcc -static " + prefix + ".o -o " + prefix + ".out";
        if(std::system(lkCmd.c_str()) != 0) return std::nullopt;
    }

    return prefix + ".out";
}

uint8_t executeProgram(std::string program, Target::Arch arch, std::vector<std::string> args) {
    int result = -1;
    switch(arch) {
        case scbe::Target::Arch::x86_64: {
            std::string cmd = program;
            for(auto arg : args) cmd += " " + arg;
            result = std::system(cmd.c_str()); break;
        }
        case scbe::Target::Arch::AArch64: {
            std::string cmd = "qemu-aarch64 " + program;
            for(auto arg : args) cmd += " " + arg;
            result = std::system(cmd.c_str());
            break;
        }
        default: break;
    }

    std::filesystem::remove_all(std::filesystem::path(program).parent_path());
#if __linux__
    return WEXITSTATUS(result);
#else
    return result;
#endif
}