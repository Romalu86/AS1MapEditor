#pragma once

// Compatibility surface for the original MapEdit compiler lane.
// The retail executable contains debug metadata compiler records for
// Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 12.00.8799.0
// (Visual C++ 6 / VC98).  Keep this header intentionally tiny: it only
// maps syntax introduced after VC6 to constructs that preserve the same
// x86 ABI.  It must not alter runtime semantics.

#if defined(_MSC_VER) && _MSC_VER < 1300
#  define MAPEDIT_JOIN_IMPL(a,b) a##b
#  define MAPEDIT_JOIN(a,b) MAPEDIT_JOIN_IMPL(a,b)
#  define static_assert(expr,msg) typedef char MAPEDIT_JOIN(mapedit_static_assert_,__LINE__)[(expr) ? 1 : -1]
#  define constexpr const
#endif
