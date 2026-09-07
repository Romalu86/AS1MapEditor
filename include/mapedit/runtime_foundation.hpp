#pragma once
// Primitive ABI types and forward declarations.
#include "mapedit/legacy_compiler.hpp"
// x86 ABI declarations for the derived MapEdit editor/runtime cluster.
// Canonical evidence: original MapEdit.exe reference implementation + debug metadata. The portable reference
// project is used only as a naming/layout accelerator where original evidence agrees.

#include <stddef.h>
#include <stdarg.h>

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long ulong32;
typedef int D3DFORMAT;
struct _iobuf; typedef _iobuf FILE;
struct IDirectSound;
struct IDirectSoundBuffer;
struct OggVorbis_File;
class STREAM;

