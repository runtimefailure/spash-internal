// version-460909c4fe904aae
// use the shitty soda cfg dumper to dump this
// or paste from https://getsoda.netlify.app/hyperion
#pragma once

#include <cstdint>

namespace Reversal
{
    static const uintptr_t ControlFlowGuard = 0x420150;
    static const uintptr_t BitMap = 0x15DC8;

    enum Offsets
    {
        ByteShift = 12,
        PageShift = 224,
        BitMask = 255,
        PageSize = 0x100000000,
        PageMask = 0xFFFFFFFF
    };
};

