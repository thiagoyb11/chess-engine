#include "search.h"
#include "move_generation.h"
#include "board.h"
#include <algorithm>
#include <limits>

namespace {
constexpr int MateScore = 100000;
constexpr int Infinity = std::numeric_limits<int>::max() / 2;
constexpr int MaterialValue[6] = {
    100,   // Pawn
    330,   // Bishop
    320,   // Knight
    500,   // Rook
    900,   // Queen
    20000  // King
};

constexpr int PromotionScore = 1'000'000;
constexpr int CaptureScore   = 500'000;
constexpr int KillerScore    = 300'000;
constexpr bool UseTT = true;

bool sameMove(const Board::Move& a, const Board::Move& b)
{
    return a.start == b.start &&
           a.end == b.end &&
           a.promotion == b.promotion;
}
}

void Search::newGame()
{
    clearTranspositionTable();
    killerMoves = {};
    killerValid = {};
    std::fill(&history[0][0][0], &history[0][0][0] + 2 * 64 * 64, 0);
}

bool Search::isCapture(const Board::Move& move, const Board& board) const
{
    const Board::side enemy =
        board.getTurn() == Board::White ? Board::Black : Board::White;

    return (board.getSidePieces(enemy) & (1ULL << move.end)) != 0;
}

bool Search::isKiller(const Board::Move& move, int ply) const
{
    if (ply < 0 || ply >= MaxPly) {
        return false;
    }

    return (killerValid[ply][0] && sameMove(move, killerMoves[ply][0])) ||
           (killerValid[ply][1] && sameMove(move, killerMoves[ply][1]));
}

void Search::recordQuietBetaCutoff(const Board::Move& move,
                                   Board::side side, int ply, int depth)
{
    if (ply >= 0 && ply < MaxPly) {
        if (!killerValid[ply][0] ||
            !sameMove(move, killerMoves[ply][0])) {
            killerMoves[ply][1] = killerMoves[ply][0];
            killerValid[ply][1] = killerValid[ply][0];

            killerMoves[ply][0] = move;
            killerValid[ply][0] = true;
        }
    }

    int& score = history[side][move.start][move.end];
    score = std::min(score + depth * depth, KillerScore - 1);
}

int Search::scoreMove(const Board::Move& move, const Board& board,
                      int ply) const
{
    const bool promotion = move.promotion != Board::Pawn;
    const bool capture = isCapture(move, board);

    if (promotion) {
        int score = PromotionScore + MaterialValue[move.promotion];

        if (capture) {
            const int victim = board.getPieceAt(move.end);
            score += MaterialValue[victim] * 10;
        }

        return score;
    }

    if (capture) {
        const int attacker = board.getPieceAt(move.start);
        const int victim = board.getPieceAt(move.end);

        return CaptureScore
            + MaterialValue[victim] * 10
            - MaterialValue[attacker];
    }

    if (isKiller(move, ply)) {
        return KillerScore;
    }

    return history[board.getTurn()][move.start][move.end];
}

int Search::pickBestMove(const std::vector<Board::Move>& moves, int first,
                         const Board& board, int ply) const
{
    int best = first;
    int bestScore = scoreMove(moves[first], board, ply);

    for (int i = first + 1; i < static_cast<int>(moves.size()); ++i) {
        const int score = scoreMove(moves[i], board, ply);

        if (score > bestScore) {
            best = i;
            bestScore = score;
        }
    }

    return best;
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

    if (UseTT && entry.valid && entry.key == key) {
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
        return quiescence(board, alpha, beta, perspective, ply, 0);
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
    for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
        // TT move was already swapped into moves[0].
        if (i > 0 || !hasTranspositionMove) {
            const int bestIndex = pickBestMove(moves, i, board, ply);
            std::swap(moves[i], moves[bestIndex]);
        }

        const Board::Move move = moves[i];
        const bool quietMove =
            move.promotion == Board::Pawn && !isCapture(move, board);

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
            alpha = std::max(alpha, bestValue);
        } else {
            if (value < bestValue) {
                bestValue = value;
                bestMove = move;
                hasBestMove = true;
            }
            beta = std::min(beta, bestValue);
        }

        if (alpha >= beta) {
            if (quietMove) {
                recordQuietBetaCutoff(move, board.getTurn(), ply, depth);
            }
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

int Search::quiescence(Board& board, int alpha, int beta, Board::side perspective, int ply, int qDepth)
{
    const int standPat = board.evalPosition(perspective);
    const bool maximizing = board.getTurn() == perspective;

    if (qDepth >= 6) {
        return standPat;
    }

    if (maximizing) {
        if (standPat >= beta) return beta;
        alpha = std::max(alpha, standPat);
    } else {
        if (standPat <= alpha) return alpha;
        beta = std::min(beta, standPat);
    }

    const auto moves = generateAllMoves(board);

    for (const Board::Move& move : moves) {
        const bool isPromotion = move.promotion != Board::Pawn;
        const bool isCapture = (board.getSidePieces(board.getTurn() == Board::White ? Board::Black : Board::White) & (1ULL << move.end)) != 0;

        if (!isCapture && !isPromotion) {
            continue;
        }

        if (getMove(move, board) != 0) {
            continue;
        }

        const int value = quiescence(board, alpha, beta, perspective, ply + 1, qDepth + 1);

        board.undoMove();

        if (maximizing) {
            alpha = std::max(alpha, value);
        } else {
            beta = std::min(beta, value);
        }

        if (alpha >= beta) {
            break;
        }
    }

    return maximizing ? alpha : beta;
}

Search::SearchResult Search::findBestMove(Board board, int depth) 
{
    clearTranspositionTable();
    const Board::side perspective = board.getTurn();

    SearchResult best{-Infinity, Board::Move{0, 0, Board::Pawn}, false};

    if (depth <= 0) 
    {
        best.score = board.evalPosition(perspective);
        return best;
    }

    const auto moves = generateAllMoves(board);

    for (const Board::Move& move : moves) 
    {
        if (getMove(move, board) != 0) 
        {
            continue;
        }

        const int value = alphaBeta(board, depth - 1, best.score, Infinity, perspective,1);

        board.undoMove();

        if (value > best.score) 
        {
            best.score = value;
            best.move = move;
            best.hasMove = true;
        }
    }

    if (!best.hasMove) 
    {
        // Handles checkmate and stalemate at the root.
        best.score = alphaBeta(board, depth, -Infinity, Infinity, perspective, 0);
    }

    return best;
}
