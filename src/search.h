#pragma once

#include "board.h"
#include <cstddef>
#include <vector>
#include <array>

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
    void clearTranspositionTable();
    void newGame();

private:
    int quiescence(Board& board, int alpha, int beta, Board::side perspective, int ply, int qDepth);
    static constexpr int MaxPly = 128;

    std::array<std::array<Board::Move, 2>, MaxPly> killerMoves{};
    std::array<std::array<bool, 2>, MaxPly> killerValid{};
    int history[2][64][64]{};

    bool isCapture(const Board::Move& move, const Board& board) const;
    bool isKiller(const Board::Move& move, int ply) const;
    void recordQuietBetaCutoff(const Board::Move& move, Board::side side,
                            int ply, int depth);

    int scoreMove(const Board::Move& move, const Board& board, int ply) const;
    int pickBestMove(const std::vector<Board::Move>& moves, int first,
                    const Board& board, int ply) const;
    int alphaBeta(Board& board, int depth, int alpha, int beta,
                  Board::side perspective, int ply);

    static constexpr std::size_t TranspositionTableSize = 1U << 20;
    std::vector<TTEntry> transpositionTable;
};
