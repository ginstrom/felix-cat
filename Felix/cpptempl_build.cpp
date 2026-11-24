// Builds third_party\cpptempl\cpptempl.cpp with Felix's precompiled header.
//
// cpptempl.cpp wraps its own #include "stdafx.h" in #ifdef _MSC_VER, which
// /Yu cannot skip cleanly ("unexpected #endif"). Including it from here makes
// that inner #include resolve to Felix's stdafx.h (already included, #pragma once).
#include "stdafx.h"
#include "cpptempl.cpp"
