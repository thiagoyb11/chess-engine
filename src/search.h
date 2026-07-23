#pragma once

#include "board.h"

class Search {
public:
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
};
