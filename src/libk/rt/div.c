#include <stdint.h>

uint64_t __udivdi3(uint64_t a, uint64_t b)
{
    uint64_t q = 0;
    uint64_t r = 0;

    for(int i = 63; i >= 0; i--)
    {
        r <<= 1;
        r |= (a >> i) & 1;

        if(r >= b)
        {
            r -= b;
            q |= (1ULL << i);
        }
    }

    return q;
}

int64_t __divdi3(int64_t a, int64_t b)
{
    int sign = (a < 0) ^ (b < 0);
    uint64_t ua = a < 0 ? -a : a;
    uint64_t ub = b < 0 ? -b : b;

    uint64_t res = __udivdi3(ua, ub);
    return sign ? -(int64_t)res : (int64_t)res;
}

uint64_t __umoddi3(uint64_t a, uint64_t b)
{
    uint64_t r = 0;
    
    for(int i = 63; i >= 0; i--)
    {
        r <<= 1;
        r |= (a >> i) & 1;

        if(r >= b)
            r -= b;
    }

    return r;
}

int64_t __moddi3(int64_t a, int64_t b)
{
    int sign = (a < 0) ^ (b < 0);
    uint64_t ua = a < 0 ? -a : a;
    uint64_t ub = b < 0 ? -b : b;

    uint64_t res = __umoddi3(ua, ub);
    return sign ? -(int64_t)res : (int64_t)res;
}
