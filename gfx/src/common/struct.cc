#include "struct.h"

namespace cm 
{
    int _ctz(u32 v) 
    {
        u32 c = 32;
        v &= -static_cast<int32_t>(v);
        if (v) c--;
        if (v & 0x0000FFFF) c -= 16;
        if (v & 0x00FF00FF) c -= 8;
        if (v & 0x0F0F0F0F) c -= 4;
        if (v & 0x33333333) c -= 2;
        if (v & 0x55555555) c -= 1;
        return c;
    }
}