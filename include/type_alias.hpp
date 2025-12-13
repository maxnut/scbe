#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace scbe {

template <class T>
using Ref = std::shared_ptr<T>;

template <class T>
using USet = std::unordered_set<T>;

template <class T, class U, class H = std::hash<T>>
using UMap = std::unordered_map<T, U, H>;

}