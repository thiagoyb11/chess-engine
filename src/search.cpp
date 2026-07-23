#include "search.h"
#include "move_generation.h"
#include "board.h"
#include <algorithm>
#include <limits>

namespace {
constexpr int MateScore = 100000;
constexpr int Infinity = std::numeric_limits<int>::max() / 2;
}

Search::Search()
    : transpositionTable(TranspositionTableSize) {}

void Search::clearTranspositionTable()
{
    std::fill(transpositionTable.begin(), transpositionTable.end(), TTEntry{});
}

int Search::score(Board cboard, int depth)
{
    return findBestMove(cboard, depth).score;
}

int Search::alphaBetaMax(Board cboard, int depth, int alpha, int beta)
{
    // Compatibility wrapper for callers that explicitly request a max node.
    clearTranspositionTable();
    return alphaBeta(cboard, depth, alpha, beta, cboard.getTurn(), 0);
}

int Search::alphaBetaMin(Board cboard, int depth, int alpha, int beta)
{
    // Compatibility wrapper for callers that explicitly request a min node.
    clearTranspositionTable();
    const Board::side perspective =
        cboard.getTurn() == Board::White ? Board::Black : Board::White;
    return alphaBeta(cboard, depth, alpha, beta, perspective, 0);
}

int Search::alphaBeta(Board& board, int depth, int alpha, int beta,
                      Board::side perspective, int ply)
{
    const int originalAlpha = alpha;
    const int originalBeta = beta;
    const u64 key = board.zobristKey();
    TTEntry& entry = transpositionTable[key & (TranspositionTableSize - 1)];
    Board::Move transpositionMove{0, 0, Board::Pawn};
    bool hasTranspositionMove = false;

    if (entry.valid && entry.key == key) {
        hasTranspositionMove = entry.hasMove;
        transpositionMove = entry.bestMove;

        if (entry.depth >= depth) {
            if (entry.bound == Bound::Exact)
                return entry.score;

            if (entry.bound == Bound::LowerBound)
                alpha = std::max(alpha, entry.score);

            if (entry.bound == Bound::UpperBound)
                beta = std::min(beta, entry.score);

            if (alpha >= beta)
                return entry.score;
        }
    }
    if (depth == 0) {
        return board.evalPosition(perspective);
    }

    const bool maximizing = board.getTurn() == perspective;
    int bestValue = maximizing ? -Infinity : Infinity;
    bool foundLegalMove = false;

    std::vector<Board::Move> moves = generateAllMoves(board);
    if (hasTranspositionMove) {
        const auto cachedMove = std::find_if(
            moves.begin(), moves.end(), [&](const Board::Move& move) {
                return move.start == transpositionMove.start &&
                       move.end == transpositionMove.end &&
                       move.promotion == transpositionMove.promotion;
            });
        if (cachedMove != moves.end()) {
            std::iter_swap(moves.begin(), cachedMove);
        }
    }

    Board::Move bestMove{0, 0, Board::Pawn};
    bool hasBestMove = false;
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
                bestMove = move;
                hasBestMove = true;
            }
            if (bestValue > alpha) {
                alpha = bestValue;
            }
        } else {
            if (value < bestValue) {
                bestValue = value;
                bestMove = move;
                hasBestMove = true;
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

    Bound bound = Bound::Exact;
    if (bestValue <= originalAlpha) {
        bound = Bound::UpperBound;
    } else if (bestValue >= originalBeta) {
        bound = Bound::LowerBound;
    }

    if (!entry.valid || entry.key == key || depth >= entry.depth) {
        entry = TTEntry{true, key, depth, bestValue, bound, bestMove, hasBestMove};
    }

    return bestValue;
}

Search::SearchResult Search::findBestMove(Board board, int depth) {
    clearTranspositionTable();
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
            best.score,
            Infinity,
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
