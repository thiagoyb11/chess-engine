#pragma once

#include "board.h"
#include <cstddef>
#include <vector>

enum class Bound {
    Exact,
    LowerBound,
    UpperBound
};

struct TTEntry {
    bool valid = false;
    u64 key = 0;
    int depth = -1;
    int score = 0;
    Bound bound = Bound::Exact;
    Board::Move bestMove{0, 0, Board::Pawn};
    bool hasMove = false;
};

class Search {
public:
    Search();

    struct SearchResult {
        int score;
        Board::Move move;
        bool hasMove;
    };
    int alphaBetaMax(Board cboard, int depth, int alpha, int beta);
    int alphaBetaMin(Board cboard, int depth, int alpha, int beta);
    int score(Board cboard, int depth);
    SearchResult findBestMove(Board board, int depth);

private:
    int alphaBeta(Board& board, int depth, int alpha, int beta,
                  Board::side perspective, int ply);
    void clearTranspositionTable();

    static constexpr std::size_t TranspositionTableSize = 1U << 20;
    std::vector<TTEntry> transpositionTable;
};
