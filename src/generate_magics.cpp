#include "magic_bitboards.h"
#include <iomanip>
#include <iostream>
#include <math.h>
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
        int relevantBits = __builtin_popcountll(magic.getRookMask(i));
        std::cout<<std::pow(2, relevantBits)<<'\n';
    }

}
