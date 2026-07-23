#include "zobrist_hashing.h"

u64 Zobrist::getPiece(int color, int type, int square) const
{
    return piece[color][type][square];
}

u64 Zobrist::getSideToMove() const
{
    return sideToMove;
}

u64 Zobrist::getCastling(int index) const
{
    return castling[index];
}

u64 Zobrist::getEnPassantFile(int file) const
{
    return enPassantFile[file];
}

u64 Zobrist::splitMix64(u64 &state)
{
    u64 z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

Zobrist::Zobrist()
{
    u64 seed = 0x123456789abcdef0ULL;

    for(int color = 0; color < 2; color++)
    {
        for(int type = 0; type < 6; type++)
        {
            for(int square = 0; square < 64; square++)
            {
                piece[color][type][square] = splitMix64(seed);
            }
        }
    }

    sideToMove = splitMix64(seed);

    for(int i = 0; i < 4; i++)
    {
        castling[i] = splitMix64(seed);
    }

    for(int i = 0; i < 8; i++)
    {
        enPassantFile[i] = splitMix64(seed);
    }
}
