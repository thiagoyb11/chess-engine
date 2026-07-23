#pragma once

#include <cstdint>

using u64 = std::uint64_t;

class Zobrist {
private:
    u64 piece[2][6][64];
    u64 sideToMove;
    u64 castling[4];
    u64 enPassantFile[8];

public:
    Zobrist();

    u64 getPiece(int color, int type, int square) const;
    u64 getSideToMove() const;
    u64 getCastling(int index) const;
    u64 getEnPassantFile(int file) const;

private:
    u64 splitMix64(u64 &state);
};
