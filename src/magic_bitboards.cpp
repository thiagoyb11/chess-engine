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

// Required by older GCC versions when the constexpr arrays are odr-used.
constexpr std::array<std::size_t, 64> MagicBitboards::rookTableSizes;
constexpr std::array<std::size_t, 64> MagicBitboards::bishopTableSizes;

MagicBitboards::MagicBitboards()
{
    initializeLeaperAttacks();

    for (int square = 0; square < 64; ++square) {
        rookMasks[square] = maskRookRelevant(square);
        rookRelevantBits[square] = __builtin_popcountll(rookMasks[square]);
        rookAttackTable[square] = std::make_unique<u64[]>(rookTableSizes[square]);

        bishopMasks[square] = maskBishopRelevant(square);
        bishopRelevantBits[square] = __builtin_popcountll(bishopMasks[square]);
        bishopAttackTable[square] = std::make_unique<u64[]>(bishopTableSizes[square]);
    }

    for (int square = 0; square < 64; ++square) {
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

    for (int square = 0; square < 64; ++square) {
        const int patternCount = 1 << bishopRelevantBits[square];

        for (int pattern = 0; pattern < patternCount; ++pattern) {
            const u64 blockers = setOccupancy(
                pattern,
                bishopRelevantBits[square],
                bishopMasks[square]
            );

            const int magicIndex = static_cast<int>(
                (blockers * bishopMagics[square]) >>
                (64 - bishopRelevantBits[square])
            );

            bishopAttackTable[square][magicIndex] =
                bishopAttacksRayTraced(square, blockers);
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

u64 MagicBitboards::bishopAttacksRayTraced(int square, u64 blockers) const
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
        // A magic number must be validated against the independent ray tracer.
        // Using the magic lookup here makes the bishop search self-referential:
        // collisions can appear valid simply because the current table is wrong.
        attacks[index] = bishop
            ? bishopAttacksRayTraced(square, occupancies[index])
            : rookAttacksRayTraced(square, occupancies[index]);
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

u64 MagicBitboards::bishopAttacks(int square, u64 occupancy) const
{
    const u64 blockers = occupancy & bishopMasks[square];

    const int index = static_cast<int>(
        (blockers * bishopMagics[square]) >>
        (64 - bishopRelevantBits[square])
    );

    return bishopAttackTable[square][index];
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
    18020183864017673ULL,
    5075367216320517ULL,
    49579198202773633ULL,
    11294459660861504ULL,
    145245623539466240ULL,
    576603758608255044ULL,
    563518500241537ULL,
    4638745008216736256ULL,
    146939489917861968ULL,
    10378547858260953856ULL,
    5787143252952023552ULL,
    91343304033337344ULL,
    2207885819905ULL,
    1157990287604711424ULL,
    4436718608384ULL,
    9369811953356703744ULL,
    256808568823555144ULL,
    9236918056884961888ULL,
    238690867240632339ULL,
    1731635158416637960ULL,
    865819339064082980ULL,
    18296019521176576ULL,
    617133895062676098ULL,
    583787902291542564ULL,
    2256204856627216ULL,
    144751274343273492ULL,
    578721349556519492ULL,
    27039466036150464ULL,
    297520152043339776ULL,
    721706513253015558ULL,
    14143708563567149568ULL,
    865817303324838400ULL,
    4904723536727252992ULL,
    2596334016168854032ULL,
    70644729905664ULL,
    7061646416959504512ULL,
    9150144356548872ULL,
    1161966368581550720ULL,
    288586620139014148ULL,
    2254085307777282ULL,
    2595217161197717536ULL,
    1319837334060994560ULL,
    3749256329034991616ULL,
    18577490867948544ULL,
    954798442998142080ULL,
    144415359095570947ULL,
    2252933693448722ULL,
    2254016018956885ULL,
    41377466110967816ULL,
    564052283899904ULL,
    72062551640375296ULL,
    1117237284872ULL,
    585749564008202240ULL,
    6734575894528ULL,
    290491041919811588ULL,
    10143553180729664ULL,
    108227695491449860ULL,
    63050953263317520ULL,
    595742913506353668ULL,
    5044068021300364353ULL,
    288230427763688448ULL,
    2305843387355916800ULL,
    72384286631919716ULL,
    18016631926038592ULL
};
