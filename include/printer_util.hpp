#pragma once

#include <string>
namespace scbe {

inline std::string escapeString(const std::string& input) {
    std::string escaped;
    for (char c : input) {
        switch (c) {
            case '\n': escaped += "\\n"; break;
            case '\t': escaped += "\\t"; break;
            case '\r': escaped += "\\r"; break;
            case '\\': escaped += "\\\\"; break;
            case '\"': escaped += "\\\""; break;
            case '\a': escaped += "\\a"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\v': escaped += "\\v"; break;
            case '\0': escaped += "\\0"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

}