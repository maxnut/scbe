#pragma once

#include "stack_slot.hpp"
#include "operand.hpp"
#include <vector>

namespace scbe::MIR {

class StackFrame {
public:
    StackSlot addStackSlot(uint32_t size, uint32_t alignment);
    StackSlot addStackSlot(uint32_t size, int64_t offset, uint32_t alignment);
    const StackSlot& getStackSlot(size_t index) const { return m_slots[index]; }
    size_t getNumStackSlots() const { return m_slots.size(); }
    size_t getSize();

    MIR::FrameIndex* getFrameIndex(size_t index) {
        if(!m_frameIndices.contains(index)) {
            m_frameIndices[index] = std::make_unique<MIR::FrameIndex>(index);
        }
        return m_frameIndices[index].get();
    }

private:
    std::vector<StackSlot> m_slots;
    UMap<size_t, std::unique_ptr<MIR::FrameIndex>> m_frameIndices;
};
    
}