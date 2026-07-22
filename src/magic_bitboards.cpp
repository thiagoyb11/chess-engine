#include "magic_bitboards.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
constexpr int BoardSize = 8;
constexpr int SquareCount = BoardSize * BoardSize;

bool validSquare(int square)
{
    return square >= 0 && square < SquareCount;
}
}

MagicBitboards::MagicBitboards()
{
    initializeLeaperAttacks();

    for (int square = 0; square < 64; ++square) {
        rookMasks[square] = maskRookRelevant(square);
        rookRelevantBits[square] = __builtin_popcountll(rookMasks[square]);

        const int patternCount = 1 << rookRelevantBits[square];

        for (int pattern = 0; pattern < patternCount; ++pattern) {
            const u64 blockers = setOccupancy(
                pattern,
                rookRelevantBits[square],
                rookMasks[square]
            );

            const int magicIndex = static_cast<int>(
                (blockers * rookMagics[square]) >>
                (64 - rookRelevantBits[square])
            );

            rookAttackTable[square][magicIndex] =
                rookAttacksRayTraced(square, blockers);
        }
    }
}

void MagicBitboards::initializeLeaperAttacks()
{
    constexpr int kingDirections[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    constexpr int knightDirections[8][2] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };

    for (int square = 0; square < SquareCount; ++square) {
        const int row = square / BoardSize;
        const int col = square % BoardSize;

        for (const auto& direction : kingDirections) {
            const int targetRow = row + direction[0];
            const int targetCol = col + direction[1];
            if (targetRow >= 0 && targetRow < BoardSize &&
                targetCol >= 0 && targetCol < BoardSize) {
                kingAttackTable[square] |= 1ULL << (targetRow * BoardSize + targetCol);
            }
        }

        for (const auto& direction : knightDirections) {
            const int targetRow = row + direction[0];
            const int targetCol = col + direction[1];
            if (targetRow >= 0 && targetRow < BoardSize &&
                targetCol >= 0 && targetCol < BoardSize) {
                knightAttackTable[square] |= 1ULL << (targetRow * BoardSize + targetCol);
            }
        }

        for (const int color : {White, Black}) {
            const int pawnDirection = color == White ? 1 : -1;
            const int targetRow = row + pawnDirection;
            for (const int targetCol : {col - 1, col + 1}) {
                if (targetRow >= 0 && targetRow < BoardSize &&
                    targetCol >= 0 && targetCol < BoardSize) {
                    pawnAttackTable[color][square] |=
                        1ULL << (targetRow * BoardSize + targetCol);
                }
            }
        }
    }
}

u64 MagicBitboards::kingAttacks(int square) const
{
    return validSquare(square) ? kingAttackTable[square] : 0ULL;
}

u64 MagicBitboards::knightAttacks(int square) const
{
    return validSquare(square) ? knightAttackTable[square] : 0ULL;
}

u64 MagicBitboards::pawnAttacks(int color, int square) const
{
    return color >= White && color <= Black && validSquare(square)
        ? pawnAttackTable[color][square]
        : 0ULL;
}

u64 MagicBitboards::pawnAttacks(int color, u64 pawns) const
{
    u64 attacks = 0ULL;
    for (int square = 0; square < SquareCount; ++square) {
        if (pawns & (1ULL << square)) {
            attacks |= pawnAttacks(color, square);
        }
    }
    return attacks;
}

u64 MagicBitboards::maskBishopRelevant(int square) const
{
    if (!validSquare(square)) return 0ULL;

    u64 mask = 0ULL;
    const int row = square / BoardSize;
    const int col = square % BoardSize;
    constexpr int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (const auto& direction : directions) {
        for (int r = row + direction[0], c = col + direction[1];
             r >= 1 && r <= 6 && c >= 1 && c <= 6;
             r += direction[0], c += direction[1]) {
            mask |= 1ULL << (r * BoardSize + c);
        }
    }
    return mask;
}

u64 MagicBitboards::maskRookRelevant(int square) const
{
    if (!validSquare(square)) return 0ULL;

    u64 mask = 0ULL;
    const int row = square / BoardSize;
    const int col = square % BoardSize;
    constexpr int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    for (const auto& direction : directions) {
        for (int r = row + direction[0], c = col + direction[1];
             r >= 0 && r < BoardSize && c >= 0 && c < BoardSize;
             r += direction[0], c += direction[1]) {
            const int nextRow = r + direction[0];
            const int nextCol = c + direction[1];
            if (nextRow < 0 || nextRow >= BoardSize ||
                nextCol < 0 || nextCol >= BoardSize) {
                break;
            }
            mask |= 1ULL << (r * BoardSize + c);
        }
    }
    return mask;
}

u64 MagicBitboards::bishopAttacks(int square, u64 blockers) const
{
    if (!validSquare(square)) return 0ULL;

    u64 attacks = 0ULL;
    const int row = square / BoardSize;
    const int col = square % BoardSize;
    constexpr int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (const auto& direction : directions) {
        for (int r = row + direction[0], c = col + direction[1];
             r >= 0 && r < BoardSize && c >= 0 && c < BoardSize;
             r += direction[0], c += direction[1]) {
            const u64 target = 1ULL << (r * BoardSize + c);
            attacks |= target;
            if (blockers & target) break;
        }
    }
    return attacks;
}

u64 MagicBitboards::rookAttacksRayTraced(int square, u64 blockers) const
{
    if (!validSquare(square)) return 0ULL;

    u64 attacks = 0ULL;
    const int row = square / BoardSize;
    const int col = square % BoardSize;
    constexpr int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    for (const auto& direction : directions) {
        for (int r = row + direction[0], c = col + direction[1];
             r >= 0 && r < BoardSize && c >= 0 && c < BoardSize;
             r += direction[0], c += direction[1]) {
            const u64 target = 1ULL << (r * BoardSize + c);
            attacks |= target;
            if (blockers & target) break;
        }
    }
    return attacks;
}

u64 MagicBitboards::randomU64()
{
    static std::mt19937_64 generator(std::random_device{}());
    return generator();
}

u64 MagicBitboards::randomMagicCandidate()
{
    return randomU64() & randomU64() & randomU64();
}

u64 MagicBitboards::setOccupancy(int index, int relevantBits, u64 attackMask)
{
    u64 occupancy = 0ULL;
    for (int bit = 0; bit < relevantBits; ++bit) {
        const int square = __builtin_ctzll(attackMask);
        attackMask &= attackMask - 1;
        if (index & (1 << bit)) occupancy |= 1ULL << square;
    }
    return occupancy;
}

u64 MagicBitboards::findMagicNumber(int square, int relevantBits, bool bishop) const
{
    const int occupancyCount = 1 << relevantBits;
    const u64 mask = bishop ? maskBishopRelevant(square) : maskRookRelevant(square);
    std::vector<u64> occupancies(occupancyCount);
    std::vector<u64> attacks(occupancyCount);
    std::vector<u64> usedAttacks(occupancyCount);
    std::vector<bool> used(occupancyCount, false);

    for (int index = 0; index < occupancyCount; ++index) {
        occupancies[index] = setOccupancy(index, relevantBits, mask);
        attacks[index] = bishop
            ? bishopAttacks(square, occupancies[index])
            : rookAttacks(square, occupancies[index]);
    }

    for (int attempt = 0; attempt < 100000000; ++attempt) {
        const u64 magic = randomMagicCandidate();
        if (__builtin_popcountll((mask * magic) & 0xFF00000000000000ULL) < 6) continue;

        std::fill(used.begin(), used.end(), false);
        bool failed = false;
        for (int index = 0; index < occupancyCount; ++index) {
            const int magicIndex = static_cast<int>(
                (occupancies[index] * magic) >> (64 - relevantBits));
            if (!used[magicIndex]) {
                used[magicIndex] = true;
                usedAttacks[magicIndex] = attacks[index];
            } else if (usedAttacks[magicIndex] != attacks[index]) {
                failed = true;
                break;
            }
        }
        if (!failed) return magic;
    }
    throw std::runtime_error("No magic number found");
}

u64 MagicBitboards::rookAttacks(int square, u64 occupancy) const
{
    const u64 blockers = occupancy & rookMasks[square];

    const int index = static_cast<int>(
        (blockers * rookMagics[square]) >>
        (64 - rookRelevantBits[square])
    );

    return rookAttackTable[square][index];
}

const u64 MagicBitboards::rookMagics[64] = {
    0x18000844000f8a0ULL,
    0x2140100020024001ULL,
    0x680100184200008ULL,
    0x4080100004820800ULL,
    0x480040080020800ULL,
    0x480020080040001ULL,
    0x8400009c02090810ULL,
    0x80110002204980ULL,
    0xc2480018420c004ULL,
    0x1141002081104000ULL,
    0x2190808020001000ULL,
    0xa0801000080081ULL,
    0x800800800400ULL,
    0x2083000400081700ULL,
    0x40a5001402000100ULL,
    0x45000080620100ULL,
    0x4040008020800044ULL,
    0x820021004e00ULL,
    0x4c820040e21200ULL,
    0x2000808010000800ULL,
    0x400808004000800ULL,
    0x20a008004008002ULL,
    0xc628040001100208ULL,
    0x401020004aa4401ULL,
    0x810200220040ULL,
    0x4285044200220881ULL,
    0x838100080200481ULL,
    0x20120200082041ULL,
    0x1008004900110004ULL,
    0x140020080800400ULL,
    0x42100a50016000cULL,
    0x200010200308044ULL,
    0x80044002402000ULL,
    0x80e001804000ULL,
    0x104101002000ULL,
    0x4148100101000820ULL,
    0x6000802401800800ULL,
    0x8800200800400ULL,
    0x8c3000401000200ULL,
    0x8400450a001084ULL,
    0x804000218004ULL,
    0x20830080c0030020ULL,
    0x100020008080ULL,
    0x201c200208a0011ULL,
    0x202c040008008080ULL,
    0x42000824620010ULL,
    0x100020110040008ULL,
    0x40090520001ULL,
    0x1c142206c1048200ULL,
    0x20200418a250200ULL,
    0x400108020420200ULL,
    0x100080880280ULL,
    0x800c800400080080ULL,
    0x8004000200800480ULL,
    0x3911500918021400ULL,
    0x283c0189004600ULL,
    0x1001008010204202ULL,
    0x824410220102ULL,
    0x4011100901422001ULL,
    0x401100061000c29ULL,
    0x3000800100433ULL,
    0xb091000882040041ULL,
    0x30200008904281aULL,
    0x802004130280a242ULL
};

const u64 MagicBitboards::bishopMagics[64] = {
    0x404200a04031010ULL,
    0x10210204004642ULL,
    0x22180108208050ULL,
    0x82a00a0812100ULL,
    0x92121008404444ULL,
    0x402088200880c008ULL,
    0xc80a061920488000ULL,
    0x42205290c100c00ULL,
    0x800040848080080ULL,
    0x200281021220220ULL,
    0x8811102410802400ULL,
    0x8009080600440100ULL,
    0x20820210200000ULL,
    0x2108210148400540ULL,
    0x8400008611504084ULL,
    0x4220442280400ULL,
    0x804044104040408ULL,
    0x804400808080040ULL,
    0x10200804882008ULL,
    0x1024c802404015ULL,
    0x29000290400805ULL,
    0x7204110082002ULL,
    0xa402100c010c2200ULL,
    0x20a2013022010401ULL,
    0x8032401020048400ULL,
    0x82000c2040108ULL,
    0x404404014040088ULL,
    0x40104044004080ULL,
    0x4800840008802002ULL,
    0x2a020001229002ULL,
    0x404011000880188ULL,
    0xa801020040220122ULL,
    0x201901000400400ULL,
    0x8802101000848120ULL,
    0x800168800101140ULL,
    0x183010800990040ULL,
    0x422208400020020ULL,
    0x1080a01002200ULL,
    0xc18008502008840ULL,
    0x45112204a0080ULL,
    0x9044a0021040ULL,
    0x8002080442010500ULL,
    0x40c20040402400ULL,
    0x1002a018000100ULL,
    0x8002083010100100ULL,
    0x201a4048404200ULL,
    0x1044013801008200ULL,
    0x14488881000204ULL,
    0x9000580a08600221ULL,
    0x2010141100040ULL,
    0x10088044a80ULL,
    0x2001220883580ULL,
    0x400002020410440ULL,
    0x8004041004084020ULL,
    0x8080800841850ULL,
    0x80414180a122884ULL,
    0x324610812101208ULL,
    0x1212010c0a80ULL,
    0xb00402804a080404ULL,
    0x6302010400aa0801ULL,
    0x9001104a08208ULL,
    0x100400520040902ULL,
    0x22020a4828008408ULL,
    0x90101000a02540ULL
};