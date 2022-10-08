#pragma once

#include "IMNODES_NAMESPACE.h"

#ifdef IMNODES_USER_CONFIG
#include "imnodes_config.h"
#else

#include <limits.h>
#include <string>

namespace IMNODES_NAMESPACE
{
// id can be any positive or negative integer, but INT_MIN is currently reserved for internal use.
using ID = int;
static constexpr ID INVALID_ID = INT_MIN;

inline void PushID(ID id) { ImGui::PushID(id); }

inline std::string IDToString(ID id) { return std::to_string(id); }

inline ID IDFromString(const std::string& str) { return std::stoi(str); }

} // namespace IMNODES_NAMESPACE

#endif