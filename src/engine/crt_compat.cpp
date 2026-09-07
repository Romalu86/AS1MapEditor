#include "mapedit/runtime.hpp"

// The VC6 build carried a local atanf wrapper in Exec\\mylib.obj
// (MapEdit.exe 0x00459140..0x0045915C).  Modern x86 MSVC can otherwise
// leave _atanf unresolved with this implementation's deliberately old-style
// CRT declarations, so keep the same source-level wrapper owner here.
extern "C" float __cdecl atanf(float value)
{
    return static_cast<float>(atan(static_cast<double>(value)));
}

// fabsf is used only as the original single-precision absolute-value helper.
// IEEE-754 fabs is exactly a sign-bit clear, including signed zero/NaN.
extern "C" float __cdecl fabsf(float value)
{
    union FloatBits {
        float f;
        uint32_t u;
    } bits;
    bits.f = value;
    bits.u &= 0x7FFFFFFFu;
    return bits.f;
}
