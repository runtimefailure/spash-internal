#pragma once

#include <cstdint>

/*
	version-9377ee10133e4be3
*/

namespace Reversal
{
    static const uintptr_t ControlFlowGuard = 0x75CFC0;
    static const uintptr_t BitMap = 0x1D26B8;

    enum Offsets
    {
        ByteShift 	= 12,
        PageShift 	= 224,
        BitMask 	= 255,
        PageSize 	= 0x100000000,
        PageMask 	= 0xFFFFFFFF
    };
};
