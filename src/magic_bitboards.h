#pragma once

#include <array>
#include <cstdint>

using u64 = std::uint64_t;

// Owns every attack table that is generated once for the engine, plus the
// helper routines used to build and validate magic-bitboard lookup tables.
class MagicBitboards {

private:
    std::array<u64, 64> kingAttackTable{};
    std::array<u64, 64> knightAttackTable{};
    std::array<std::array<u64, 64>, 2> pawnAttackTable{};
    static const u64 rookMagics[64];
    static const u64 bishopMagics[64];

    std::array<u64, 64> rookMasks{};
    std::array<int, 64> rookRelevantBits{};
    std::array<std::array<u64, 4096>, 64> rookAttackTable{};

    std::array<u64, 64> bishopMasks{};
    std::array<int, 64> bishopRelevantBits{};
    std::array<std::array<u64, 512>, 64> bishopAttackTable{};

    static u64 randomU64();
    static u64 randomMagicCandidate();
    
    void initializeLeaperAttacks();

public:
    static constexpr int White = 0;
    static constexpr int Black = 1;

    MagicBitboards();

    static u64 setOccupancy(int index, int relevantBits, u64 attackMask);

    u64 kingAttacks(int square) const;
    u64 knightAttacks(int square) const;
    u64 pawnAttacks(int color, int square) const;
    u64 pawnAttacks(int color, u64 pawns) const;

    u64 maskBishopRelevant(int square) const;
    u64 maskRookRelevant(int square) const;
    u64 bishopAttacks(int square, u64 occupancy) const;
    u64 bishopAttacksRayTraced(int square, u64 blockers) const;
    u64 rookAttacks(int square, u64 occupancy) const;
    u64 rookAttacksRayTraced(int square, u64 blockers) const;
    u64 findMagicNumber(int square, int relevantBits, bool bishop) const;

    u64 getRookMask(int square) {return rookMasks[square];};
    u64 getRookAttacks(int square, int index) {return rookAttackTable[square][index];};
    u64 getRookMagics(int square) {return rookMagics[square];};

    u64 getBishopMask(int square) {return bishopMasks[square];};
    u64 getBishopAttacks(int square, int index) {return bishopAttackTable[square][index];};
    u64 getBishopMagics(int square) {return bishopMagics[square];};
};
