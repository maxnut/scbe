#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace scbe {

struct SourceLocation {
    uint32_t m_fileId;
    uint32_t m_lineBegin;
    uint32_t m_columnBegin;
    uint32_t m_lineEnd;
    uint32_t m_columnEnd;
};

class DiagnosticEmitter {
public:
    virtual void error(const std::string& msg, std::optional<SourceLocation> loc) = 0;
};

}