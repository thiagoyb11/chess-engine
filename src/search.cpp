#include "search.h"
#include "move_generation.h"
#include "board.h"
#include <limits>

namespace {
constexpr int MateScore = 100000;
constexpr int Infinity = std::numeric_limits<int>::max() / 2;
}

int Search::score(Board cboard, int depth)
{
    return findBestMove(cboard, depth).score;
}

int Search::alphaBetaMax(Board cboard, int depth, int alpha, int beta)
{
    // Compatibility wrapper for callers that explicitly request a max node.
    return alphaBeta(cboard, depth, alpha, beta, cboard.getTurn(), 0);
}

int Search::alphaBetaMin(Board cboard, int depth, int alpha, int beta)
{
    // Compatibility wrapper for callers that explicitly request a min node.
    const Board::side perspective =
        cboard.getTurn() == Board::White ? Board::Black : Board::White;
    return alphaBeta(cboard, depth, alpha, beta, perspective, 0);
}

int Search::alphaBeta(Board& board, int depth, int alpha, int beta,
                      Board::side perspective, int ply)
{
    if (depth == 0) {
        return board.evalPosition(perspective);
    }

    const bool maximizing = board.getTurn() == perspective;
    int bestValue = maximizing ? -Infinity : Infinity;
    bool foundLegalMove = false;

    const std::vector<Board::Move> moves = generateAllMoves(board);
    for (const Board::Move& move : moves) {
        if (getMove(move, board) != 0) {
            continue;
        }

        foundLegalMove = true;
        const int value = alphaBeta(
            board, depth - 1, alpha, beta, perspective, ply + 1);
        board.undoMove();

        if (maximizing) {
            if (value > bestValue) {
                bestValue = value;
            }
            if (bestValue > alpha) {
                alpha = bestValue;
            }
        } else {
            if (value < bestValue) {
                bestValue = value;
            }
            if (bestValue < beta) {
                beta = bestValue;
            }
        }

        if (alpha >= beta) {
            break;
        }
    }

    if (!foundLegalMove) {
        if (board.kingAttacked(board.getTurn())) {
            const bool perspectiveIsMated = board.getTurn() == perspective;
            return perspectiveIsMated
                ? -MateScore + ply
                : MateScore - ply;
        }
        return 0; // stalemate
    }

    return bestValue;
}

Search::SearchResult Search::findBestMove(Board board, int depth) {
    const Board::side perspective = board.getTurn();

    SearchResult best{
        -Infinity,
        Board::Move{0, 0, Board::Pawn},
        false
    };

    if (depth <= 0) {
        best.score = board.evalPosition(perspective);
        return best;
    }

    const auto moves = generateAllMoves(board);

    for (const Board::Move& move : moves) {
        if (getMove(move, board) != 0) {
            continue;
        }

        const int value = alphaBeta(
            board,
            depth - 1,
            std::numeric_limits<int>::min(),
            std::numeric_limits<int>::max(),
            perspective,
            1
        );

        board.undoMove();

        if (value > best.score) {
            best.score = value;
            best.move = move;
            best.hasMove = true;
        }
    }

    if (!best.hasMove) {
        // Handles checkmate and stalemate at the root.
        best.score = alphaBeta(board, depth, -Infinity, Infinity,
                               perspective, 0);
    }

    return best;
}
