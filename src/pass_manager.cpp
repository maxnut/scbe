#include "pass_manager.hpp"
#include "unit.hpp"
#include "IR/function.hpp"
#include "IR/instruction.hpp"
#include "IR/block.hpp"
#include "MIR/function.hpp"

namespace scbe {

void PassManager::run(Unit& unit) {
    for(size_t i = 0; i < m_groups.size();) {
        bool anyChange = false;
        for(auto& pass : m_groups.at(i).m_passes) {
            pass->init(unit);
            switch (pass->getKind()) {
                case Pass::Kind::Function:
                    for(auto& function : unit.m_functions) {
                        if(!function->hasBody()) continue;
                        anyChange |= ((FunctionPass*)pass.get())->run(function.get());
                    }
                    break;
                case Pass::Kind::MachineFunction:
                    for(auto& function : unit.m_functions) {
                        if(!function->hasBody()) continue;
                        anyChange |= ((MachineFunctionPass*)pass.get())->run(function->getMachineFunction());
                    }
                    break;
                case Pass::Kind::Instruction: {
                    InstructionPass* ipass = (InstructionPass*)pass.get();
                    for(auto& function : unit.m_functions) {
                        if(!function->hasBody()) continue;
                        for(auto& block : function->getBlocks()) {
                            do {
                                ipass->m_restart = false;
                                for(auto& instruction : block->getInstructions()) {
                                    anyChange |= ipass->run(instruction.get());
                                    if(ipass->m_restart) break;
                                }
                            } while(ipass->m_restart);
                        }
                    }
                    break;
                }
                case Pass::Kind::MachineInstruction: {
                    MachineInstructionPass* ipass = (MachineInstructionPass*)pass.get();
                    for(auto& function : unit.m_functions) {
                        if(!function->hasBody()) continue;
                        for(auto& block : function->getMachineFunction()->getBlocks()) {
                            do {
                                ipass->m_restart = false;
                                for(auto& instruction : block->getInstructions()) {
                                    anyChange |= ipass->run(instruction.get());
                                    if(ipass->m_restart) break;
                                }
                            } while(ipass->m_restart);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            pass->end(unit);
        }
        if(anyChange && m_groups.at(i).m_repeat) continue;
        i++;
    }
}

}