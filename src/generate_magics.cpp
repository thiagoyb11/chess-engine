#include "magic_bitboards.h"
#include <iomanip>
#include <iostream>
#include <cstdint>

int main()
{
    static MagicBitboards magic;

    /* std::cout<<"const u64 MagicBitboards::bishopMagics[64] = {\n";
    for(int i = 0; i < 64; i++)
    {
        std::cout<<magic.findMagicNumber(i, __builtin_popcountll(magic.getBishopMask(i)), true)<<"ULL,\n";
    }
    std::cout<<"};\n"; */

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

    for(int i = 0; i < 64; i++)
    {
        int bishopRelevantBits = __builtin_popcountll(magic.getBishopMask(i));

        const int patternCount = 1 << bishopRelevantBits;

        for (int pattern = 0; pattern < patternCount; ++pattern)
        {
            int magicIndex = (magic.getBishopMagics(i) * magic.setOccupancy(pattern, bishopRelevantBits, magic.getBishopMask(i))) >> (64 - bishopRelevantBits);

            if(magic.getBishopAttacks(i, magicIndex) != magic.bishopAttacksRayTraced(i, magic.setOccupancy(pattern, bishopRelevantBits, magic.getBishopMask(i))))
            {
                std::cout<<"err"<<i<<" "<<pattern<<'\n';
                return 1;
            }
        }
    }
}
