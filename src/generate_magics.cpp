#include "magic_bitboards.h"

#include <iostream>

int main()
{
    static MagicBitboards magic;

    for(int i = 0; i < 64; i++)
    {
        int rookRelevantBits = __builtin_popcountll(magic.getRookMask(i));

        const int patternCount = 1 << rookRelevantBits;

        for (int pattern = 0; pattern < patternCount; ++pattern)
        {
            int magicIndex = (magic.getRookMagics(i) * magic.setOccupancy(pattern, rookRelevantBits, magic.getRookMask(i))) >> (64 - rookRelevantBits);

            if(magic.getRookAttacks(i, magicIndex) != magic.rookAttacksRayTraced(i, magic.setOccupancy(pattern, rookRelevantBits, magic.getRookMask(i))))
            {
                std::cout<<"ERROR"<<'\n';
                return 0;
            }
        }
    }
}