#pragma once

#include "type.hpp"
#include "MIR/operand.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace scbe::MIR {
class RegisterInfo;
}

namespace scbe::Target {

class RegisterDesc {
public:
    RegisterDesc(std::string name, std::vector<uint32_t> superRegs, std::vector<uint32_t> subRegs, std::vector<uint32_t> aliasRegs, uint32_t regClass) : m_name(name), m_superRegs(superRegs), m_subRegs(subRegs), m_aliasRegs(aliasRegs), m_regClass(regClass) {}

    const std::string& getName() const { return m_name; }
    const std::vector<uint32_t>& getSuperRegs() const { return m_superRegs; }
    const std::vector<uint32_t>& getSubRegs() const { return m_subRegs; }
    const std::vector<uint32_t>& getAliasRegs() const { return m_aliasRegs; }
    uint32_t getRegClass() const { return m_regClass; }

protected:
    std::string m_name;
    std::vector<uint32_t> m_superRegs;
    std::vector<uint32_t> m_subRegs;
    std::vector<uint32_t> m_aliasRegs;
    uint32_t m_regClass;
};

class RegisterClass {
public:
    RegisterClass(std::vector<uint32_t> regs, size_t size, size_t alignment) : m_regs(regs), m_size(size), m_alignment(alignment) {}

    const std::vector<uint32_t>& getRegs() const { return m_regs; }
    // size in bits
    size_t getSize() const { return m_size; }
    // alignment in bits
    size_t getAlignment() const { return m_alignment; }

protected:
    std::vector<uint32_t> m_regs;
    size_t m_size;
    size_t m_alignment;
};

class SubRegIndexDesc {
public:
    SubRegIndexDesc(std::string name, size_t offset, size_t size) : m_name(name), offset(offset), size(size) {}

    const std::string& getName() const { return m_name; }
    size_t getOffset() const { return offset; }
    size_t getSize() const { return size; }
    
protected:
    std::string m_name;
    size_t offset;
    size_t size;
};

class RegisterInfo {
public:
    size_t getNumRegisters() const { return m_registerDescs.size(); }
    bool isPhysicalRegister(uint32_t reg) const { return reg < m_registerDescs.size(); }
    bool isSameRegister(uint32_t reg1, uint32_t reg2) const;
    const std::string& getName(uint32_t reg) const { return m_registerDescs[reg].getName(); }

    const RegisterDesc& getRegisterDesc(uint32_t reg) const { return m_registerDescs[reg]; }
    const RegisterClass& getRegisterClass(uint32_t regClass) const { return m_registerClasses[regClass]; }
    const std::vector<uint32_t> getRegistersInClass(uint32_t regClass) const { return m_registerClasses[regClass].getRegs(); }
    uint32_t getRegisterIdClass(uint32_t reg, MIR::RegisterInfo& mirInfo) const;
    uint32_t getCanonicalRegister(uint32_t reg);
    size_t getNumRegisterClasses() const { return m_registerClasses.size(); }

    const std::vector<uint32_t>& getSubRegs(uint32_t reg) const { return m_registerDescs[reg].getSubRegs(); }
    const std::vector<uint32_t>& getSuperRegs(uint32_t reg) const { return m_registerDescs[reg].getSuperRegs(); }
    const std::vector<uint32_t>& getAliasRegs(uint32_t reg) const { return m_registerDescs[reg].getAliasRegs(); }

    std::optional<uint32_t> getRegisterWithSize(uint32_t reg, size_t desiredSize) const;

    virtual uint32_t getSubReg(uint32_t reg, uint32_t idx) = 0;
    virtual uint32_t getClassFromType(Type* type) = 0;
    virtual const std::vector<uint32_t>& getCallerSavedRegisters() const = 0;
    virtual const std::vector<uint32_t>& getCalleeSavedRegisters() const = 0;
    virtual const std::vector<uint32_t>& getReservedRegisters(uint32_t rclass) const = 0;
    virtual const std::vector<uint32_t>& getAvailableRegisters(uint32_t rclass) const = 0;
    virtual const std::vector<uint32_t>& getReservedRegistersCanonical(uint32_t rclass) const = 0;
    virtual const std::vector<uint32_t>& getAvailableRegistersCanonical(uint32_t rclass) const = 0;
    virtual bool doClassesOverlap(uint32_t class1, uint32_t class2) const = 0;
    MIR::Register* getRegister(uint32_t reg, int64_t flags = 0);

protected:
    RegisterInfo() = default;

protected:
    std::vector<RegisterDesc> m_registerDescs;
    std::vector<RegisterClass> m_registerClasses;
    std::vector<SubRegIndexDesc> m_subRegIndexDescs;

    std::vector<std::unique_ptr<MIR::Register>> m_mirRegisters;
    UMap<size_t, MIR::Register*> m_mirRegisterCache;
};

}