#include "board.h"
#include "magic_bitboards.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace {
const MagicBitboards attackTables;
const Zobrist zobrist;
}
Board::Board()
    : pieceBB{
          0x000000000000FFFFULL,
          0xFFFF000000000000ULL,
          0x00FF00000000FF00ULL,
          0x2400000000000024ULL,
          0x4200000000000042ULL,
          0x8100000000000081ULL,
          0x0800000000000008ULL,
          0x1000000000000010ULL
      } {
        halfmoveClock = 0;
        moveCount = 0;
        whiteEval = calculateEval(White);
        hash = calculateHash();
      }

u64 Board::getPieces(side s, enumPiece p) const {
    return pieceBB[s] & pieceBB[p + 2];
}

u64 Board::getSidePieces(side s) const {
    return pieceBB[s];
}

u64 Board::getAllPieces() const {
    return pieceBB[White] | pieceBB[Black];
}

void Board::removePieceAt(int idx) {
    if (idx < 0 || idx >= 64) {
        return;
    }

    const u64 keepMask = ~(1ULL << idx);
    const bool wasWhitePiece = (pieceBB[White] & (1ULL << idx)) != 0;
    const bool wasBlackPiece = (pieceBB[Black] & (1ULL << idx)) != 0;

    for (int i = 0; i < 8; i++) {
        u64 bb = pieceBB[i];
        pieceBB[i] &= keepMask;
        if (bb != pieceBB[i] && i > 1) {
            if (wasWhitePiece) {
                whiteEval -= PSQT[(i - 2) * 64 + idx];
                hash ^= zobrist.getPiece(White, i - 2, idx);
            } else if (wasBlackPiece) {
                whiteEval += PSQT[(i - 2) * 64 + (idx ^ 56)];
                hash ^= zobrist.getPiece(Black, i - 2, idx);
            }
        }
    }
}

int Board::getPieceAt(int idx) const
{
    for(int i = 2; i < 8; i++)
    {
        if(pieceBB[i] & (1ULL << idx))
        {
            return i - 2;
        }
    }
    return -1;
}

int Board::updatePosition(int start, int end, side s, enumPiece p, enumPiece promoted) {
    if (start < 0 || start >= 64 || end < 0 || end >= 64) {
        return -1;
    }
    bool pieceCaptured = false;
    bool pawnMoved = (p == Pawn);

    MoveState previousState{};
    std::copy(std::begin(pieceBB), std::end(pieceBB), previousState.pieceBB);
    previousState.castleWhiteKingside = castleWhiteKingside;
    previousState.castleWhiteQueenside = castleWhiteQueenside;
    previousState.castleBlackKingside = castleBlackKingside;
    previousState.castleBlackQueenside = castleBlackQueenside;
    previousState.enPassantSquare = enPassantSquare;
    previousState.turn = turn;
    previousState.moveCount = moveCount;
    previousState.halfmoveClock = halfmoveClock;
    previousState.pieceCaptured = false;
    previousState.capturedPiece = Pawn;
    previousState.whiteEval = whiteEval;
    previousState.hash = hash;

    const u64 startMask = 1ULL << start;
    const u64 endMask = 1ULL << end;

    const int endRank = end / 8;
    const bool reachesPromotionRank = p == Pawn &&
        ((s == White && endRank == 7) || (s == Black && endRank == 0));
    if (reachesPromotionRank) {
        if (promoted != Bishop && promoted != Knight &&
            promoted != Rook && promoted != Queen) {
            return -1;
        }
    } else if (promoted != Pawn) {
        return -1;
    }

    const int pieceIndex = p + 2;
    const int kingStart = s == White ? 4 : 60;
    const bool kingsideCastle = p == King && start == kingStart && end == kingStart + 2;
    const bool queensideCastle = p == King && start == kingStart && end == kingStart - 2;
    const bool castling = kingsideCastle || queensideCastle;

    if ((pieceBB[s] & pieceBB[pieceIndex] & startMask) == 0) {
        return -1;
    }

    if (pieceBB[s] & endMask) {
        return -1;
    }

    const side enemy = (s == White) ? Black : White;
    // A king is never captured in chess; check and checkmate determine the
    // result. Keep attack generation independent from this rule.
    if (getPieces(enemy, King) & endMask) {
        return -1;
    }

    if (castling) {
        // getKingMoves checks the rook, path, and castling right. The king
        // also may not castle out of, through, or into check.
        if ((getKingMoves(start, s) & endMask) == 0 || kingAttacked(s)) {
            return -1;
        }

        const int middle = start + (kingsideCastle ? 1 : -1);
        pieceBB[s] = (pieceBB[s] & ~startMask) | (1ULL << middle);
        pieceBB[King + 2] = (pieceBB[King + 2] & ~startMask) | (1ULL << middle);
        const bool crossesCheck = kingAttacked(s);
        std::copy(std::begin(previousState.pieceBB), std::end(previousState.pieceBB), pieceBB);
        if (crossesCheck) {
            return -1;
        }
    }

    // An en passant opportunity lasts for exactly one opposing move.
    // Keep the square occupied by the pawn that advanced two squares so
    // pawn move generation can find it from either side.
    const int direction = s == White ? 1 : -1;
    const int capturedPawnSquare = end - direction * 8;
    const bool enPassantCapture =
        p == Pawn &&
        (pieceBB[enemy] & endMask) == 0 &&
        enPassantSquare == capturedPawnSquare &&
        (pieceBB[enemy] & pieceBB[Pawn + 2] & (1ULL << capturedPawnSquare));

    if (enPassantSquare >= 0) {
        hash ^= zobrist.getEnPassantFile(enPassantSquare % 8);
    }
    enPassantSquare = -1;

    if (pieceBB[enemy] & endMask) {
        pieceCaptured = true;
        previousState.capturedPiece = static_cast<enumPiece>(getPieceAt(end));
        removePieceAt(end);
    }
    if (enPassantCapture) {
        pieceCaptured = true;
        previousState.capturedPiece = Pawn;
        removePieceAt(capturedPawnSquare);
    }
    previousState.pieceCaptured = pieceCaptured;
    if (p == Pawn && abs(start - end) == 16) {
        enPassantSquare = end;
        hash ^= zobrist.getEnPassantFile(end % 8);
    }

    auto clearCastlingRight = [&](bool& right, int index) {
        if (right) {
            right = false;
            hash ^= zobrist.getCastling(index);
        }
    };

    if (p == King) {
        if (s == White) {
            clearCastlingRight(castleWhiteKingside, 0);
            clearCastlingRight(castleWhiteQueenside, 1);
        } else {
            clearCastlingRight(castleBlackKingside, 2);
            clearCastlingRight(castleBlackQueenside, 3);
        }
    }
    if (p == Rook) {
        if (start == 0) clearCastlingRight(castleWhiteQueenside, 1);
        if (start == 7) clearCastlingRight(castleWhiteKingside, 0);
        if (start == 56) clearCastlingRight(castleBlackQueenside, 3);
        if (start == 63) clearCastlingRight(castleBlackKingside, 2);
    }
    if (end == 0) clearCastlingRight(castleWhiteQueenside, 1);
    if (end == 7) clearCastlingRight(castleWhiteKingside, 0);
    if (end == 56) clearCastlingRight(castleBlackQueenside, 3);
    if (end == 63) clearCastlingRight(castleBlackKingside, 2);

    pieceBB[s] &= ~startMask;
    pieceBB[s] |= endMask;

    pieceBB[pieceIndex] &= ~startMask;
    pieceBB[pieceIndex] |= endMask;

    if (reachesPromotionRank)
    {
        // The pawn has already been moved onto the destination. Replace it
        // in the type bitboards while leaving the side bitboard untouched.
        pieceBB[Pawn + 2] &= ~endMask;
        pieceBB[promoted + 2] |= endMask;
        const int materialDelta = s == White ? (PSQT[promoted * 64 + end] - PSQT[start]) : (PSQT[promoted * 64 + (end ^ 56)] - PSQT[start ^ 56]);
        whiteEval += s == White ? materialDelta : -materialDelta;
        hash ^= zobrist.getPiece(s, p, start);
        hash ^= zobrist.getPiece(s, promoted, end);
    }
    else
    {
        const int materialDelta = s == White ? (PSQT[p * 64 + end] - PSQT[p * 64 + start]) : (PSQT[p * 64 + (end ^ 56)] - PSQT[p * 64 + (start ^ 56)]);
        whiteEval += s == White ? materialDelta : -materialDelta;
        hash ^= zobrist.getPiece(s, p, start);
        hash ^= zobrist.getPiece(s, p, end);
    }

    if (castling)
    {
        const int rookStart = kingsideCastle ? start + 3 : start - 4;
        const int rookEnd = kingsideCastle ? start + 1 : start - 1;
        const u64 rookStartMask = 1ULL << rookStart;
        const u64 rookEndMask = 1ULL << rookEnd;

        pieceBB[s] = (pieceBB[s] & ~rookStartMask) | rookEndMask;
        pieceBB[Rook + 2] = (pieceBB[Rook + 2] & ~rookStartMask) | rookEndMask;
        hash ^= zobrist.getPiece(s, Rook, rookStart);
        hash ^= zobrist.getPiece(s, Rook, rookEnd);
    }

    if(kingAttacked(s))
    {
        std::copy(std::begin(previousState.pieceBB), std::end(previousState.pieceBB), pieceBB);
        castleWhiteKingside = previousState.castleWhiteKingside;
        castleWhiteQueenside = previousState.castleWhiteQueenside;
        castleBlackKingside = previousState.castleBlackKingside;
        castleBlackQueenside = previousState.castleBlackQueenside;
        enPassantSquare = previousState.enPassantSquare;
        whiteEval = previousState.whiteEval;
        hash = previousState.hash;
        return -1;
    }



    turn = (turn == White) ? Black : White;
    hash ^= zobrist.getSideToMove();
    if(pieceCaptured || pawnMoved) halfmoveClock = 0;
    else halfmoveClock++;
    moveCount++;
    saveBoardState(previousState);
    return 0;
}

u64 Board::getPawnMoves(int idx, side turn) const
{
    u64 possibleMoves = 0;

    if (idx < 0 || idx >= 64) {
        return possibleMoves;
    }

    const u64 allPieces = getAllPieces();
    const side enemy = turn == White ? Black : White;
    const u64 enemyPieces = getSidePieces(enemy);
    possibleMoves = attackTables.pawnAttacks(static_cast<int>(turn), idx) & enemyPieces;
    const int row = idx / 8;
    const int col = idx % 8;
    const int direction = turn == White ? 1 : -1;
    const int startingRow = turn == White ? 1 : 6;
    const int nextRow = row + direction;

    // Pawns move straight ahead only into empty squares.
    if (nextRow >= 0 && nextRow < 8) {
        const int oneStep = nextRow * 8 + col;
        if ((allPieces & (1ULL << oneStep)) == 0) {
            possibleMoves |= 1ULL << oneStep;

            const int twoStepRow = row + 2 * direction;
            const int twoStep = twoStepRow * 8 + col;
            if (row == startingRow && (allPieces & (1ULL << twoStep)) == 0) {
                possibleMoves |= 1ULL << twoStep;
            }
        }

        // En passant lands on an empty diagonal square, so it is handled
        // separately from ordinary pawn captures above.
        for (const int captureCol : {col - 1, col + 1}) {
            if (captureCol < 0 || captureCol >= 8) {
                continue;
            }

            const int adjacentSquare = row * 8 + captureCol;
            const int captureSquare = nextRow * 8 + captureCol;
            if (enPassantSquare == adjacentSquare &&
                (getPieces(enemy, Pawn) & (1ULL << adjacentSquare))) {
                possibleMoves |= 1ULL << captureSquare;
            }
        }
    }

    return possibleMoves;
}

u64 Board::getKnightMoves(int idx, side turn) const
{
    if (idx < 0 || idx >= 64) {
        return 0;
    }

    return attackTables.knightAttacks(idx) & ~getSidePieces(turn);
}

u64 Board::getBishopMoves(int idx, side turn) const
{
    return attackTables.bishopAttacks(idx, getAllPieces()) & ~getSidePieces(turn);
}
u64 Board::getRookMoves(int idx, side turn) const
{
    return attackTables.rookAttacks(idx, getAllPieces()) & ~getSidePieces(turn);
}

u64 Board::getQueenMoves(int idx, side turn) const
{
    return getBishopMoves(idx, turn) | getRookMoves(idx, turn);
}

u64 Board::getKingMoves(int idx, side turn) const
{
    u64 possibleMoves = 0;
    if (idx < 0 || idx >= 64) {
        return possibleMoves;
    }

    possibleMoves = attackTables.kingAttacks(idx) & ~getSidePieces(turn);

    const int kingStart = turn == White ? 4 : 60;
    const int kingsideRook = turn == White ? 7 : 63;
    const int queensideRook = turn == White ? 0 : 56;
    const bool canCastleKingside = turn == White ? castleWhiteKingside : castleBlackKingside;
    const bool canCastleQueenside = turn == White ? castleWhiteQueenside : castleBlackQueenside;
    const u64 allPieces = getAllPieces();

    if (idx == kingStart && (getPieces(turn, King) & (1ULL << kingStart))) {
        const u64 kingsidePath = (1ULL << (kingStart + 1)) | (1ULL << (kingStart + 2));
        if (canCastleKingside &&
            (getPieces(turn, Rook) & (1ULL << kingsideRook)) &&
            (allPieces & kingsidePath) == 0) {
            possibleMoves |= 1ULL << (kingStart + 2);
        }

        const u64 queensidePath = (1ULL << (kingStart - 1)) |
                                  (1ULL << (kingStart - 2)) |
                                  (1ULL << (kingStart - 3));
        if (canCastleQueenside &&
            (getPieces(turn, Rook) & (1ULL << queensideRook)) &&
            (allPieces & queensidePath) == 0) {
            possibleMoves |= 1ULL << (kingStart - 2);
        }
    }

    return possibleMoves;
}

u64 Board::getPawnAttacks(side color) const
{
    return attackTables.pawnAttacks(static_cast<int>(color), getPieces(color, Pawn));
}

bool Board::kingAttacked(side s)
{
    side enemySide = static_cast<side>(!s);
    u64 kingPos = getPieces(s, King);

    u64 enemyPieces = getSidePieces(enemySide);
    while (enemyPieces) {
        const int square = __builtin_ctzll(enemyPieces);
        enemyPieces &= enemyPieces - 1;

        const enumPiece piece = static_cast<enumPiece>(getPieceAt(square));
        u64 attacks = 0;

        switch (piece) {
        case Pawn:
            // Pawn moves include forward pushes, which are not attacks.
            attacks = attackTables.pawnAttacks(static_cast<int>(enemySide), square);
            break;
        case Knight:
            attacks = getKnightMoves(square, enemySide);
            break;
        case Bishop:
            attacks = getBishopMoves(square, enemySide);
            break;
        case Rook:
            attacks = getRookMoves(square, enemySide);
            break;
        case Queen:
            attacks = getQueenMoves(square, enemySide);
            break;
        case King:
            // King moves may include castling, which is not an attack.
            attacks = attackTables.kingAttacks(square);
            break;
        }

        if (attacks & kingPos) {
            return true;
        }
    }

    return false;
}

void Board::saveBoardState(const MoveState& state)
{
    moveHistory.push_back(state);
}

int Board::calculateEval(side s) const
{
    int score = 0;
    u64 allPieces = getAllPieces();
    u64 sidePieces = getSidePieces(s);

    for(int i = 0; i < 64; i++)
    {
        if(allPieces & (1ULL << i))
        {
            const int pieceIdx = getPieceAt(i);
            if (pieceIdx < Pawn || pieceIdx > King) {
                continue;
            }

            const int tableSquare =
                (pieceBB[White] & (1ULL << i)) ? i : (i ^ 56);
            const int pieceValue = PSQT[pieceIdx * 64 + tableSquare];

            if(sidePieces & (1ULL << i))
            {
                score += pieceValue;
            }
            else
            {
                score -= pieceValue;
            }
        }
    }
    return score;
}

int Board::evalPosition(side s)
{
    // Material is maintained incrementally in updatePosition.  This keeps
    // evaluation constant-time after the board has been initialized.
    return s == White ? whiteEval : -whiteEval;
}

void Board::loadFromFEN(const std::string& fen)
{
    std::istringstream input(fen);
    std::string placement, activeColor, castling, enPassant;
    int newHalfmoveClock, fullmoveNumber;

    if (!(input >> placement >> activeColor >> castling >> enPassant >>
          newHalfmoveClock >> fullmoveNumber) ||
        activeColor.size() != 1 ||
        (activeColor[0] != 'w' && activeColor[0] != 'b') ||
        newHalfmoveClock < 0 || fullmoveNumber < 1) {
        std::cerr << "Invalid FEN string" << '\n';
        return;
    }

    u64 newPieceBB[8] = {};
    int rank = 7;
    int file = 0;
    for (const char c : placement) {
        if (c == '/') {
            if (file != 8 || rank == 0) {
                std::cerr << "Invalid FEN string" << '\n';
                return;
            }
            --rank;
            file = 0;
            continue;
        }
        if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > 8) {
                std::cerr << "Invalid FEN string" << '\n';
                return;
            }
            continue;
        }

        const std::string pieces = "pbnrqkPBNRQK";
        const std::size_t piece = pieces.find(c);
        if (piece == std::string::npos || file >= 8) {
            std::cerr << "Invalid FEN string" << '\n';
            return;
        }

        const int square = rank * 8 + file++;
        const side color = piece < 6 ? Black : White;
        const enumPiece type = static_cast<enumPiece>(piece % 6);
        newPieceBB[color] |= 1ULL << square;
        newPieceBB[type + 2] |= 1ULL << square;
    }
    if (rank != 0 || file != 8) {
        std::cerr << "Invalid FEN string" << '\n';
        return;
    }

    bool newCastleWhiteKingside = false;
    bool newCastleWhiteQueenside = false;
    bool newCastleBlackKingside = false;
    bool newCastleBlackQueenside = false;
    if (castling != "-") {
        for (const char right : castling) {
            switch (right) {
                case 'K': newCastleWhiteKingside = true; break;
                case 'Q': newCastleWhiteQueenside = true; break;
                case 'k': newCastleBlackKingside = true; break;
                case 'q': newCastleBlackQueenside = true; break;
                default: std::cerr << "Invalid FEN string" << '\n'; return;
            }
        }
    }

    int newEnPassantSquare = -1;
    if (enPassant != "-") {
        if (enPassant.size() != 2 || enPassant[0] < 'a' || enPassant[0] > 'h' ||
            (enPassant[1] != '3' && enPassant[1] != '6')) {
            std::cerr << "Invalid FEN string" << '\n';
            return;
        }
        const int targetSquare = (enPassant[1] - '1') * 8 + (enPassant[0] - 'a');
        newEnPassantSquare = targetSquare + (activeColor[0] == 'b' ? 8 : -8);
    }

    std::copy(std::begin(newPieceBB), std::end(newPieceBB), pieceBB);
    turn = activeColor[0] == 'w' ? White : Black;
    castleWhiteKingside = newCastleWhiteKingside;
    castleWhiteQueenside = newCastleWhiteQueenside;
    castleBlackKingside = newCastleBlackKingside;
    castleBlackQueenside = newCastleBlackQueenside;
    enPassantSquare = newEnPassantSquare;
    halfmoveClock = newHalfmoveClock;
    moveCount = 2 * (fullmoveNumber - 1) + (turn == Black ? 1 : 0);
    moveHistory.clear();
    whiteEval = calculateEval(White);
    hash = calculateHash();
}

void Board::undoMove()
{
    if (moveHistory.empty()) {
        std::cerr << "No moves to undo" << '\n';
        return;
    }

    const MoveState previousState = moveHistory.back();
    moveHistory.pop_back();
    std::copy(std::begin(previousState.pieceBB), std::end(previousState.pieceBB), pieceBB);
    castleWhiteKingside = previousState.castleWhiteKingside;
    castleWhiteQueenside = previousState.castleWhiteQueenside;
    castleBlackKingside = previousState.castleBlackKingside;
    castleBlackQueenside = previousState.castleBlackQueenside;
    enPassantSquare = previousState.enPassantSquare;
    turn = previousState.turn;
    moveCount = previousState.moveCount;
    halfmoveClock = previousState.halfmoveClock;
    whiteEval = previousState.whiteEval;
    hash = previousState.hash;
}

u64 Board::calculateHash() const
{
    u64 key = 0;

    for(int square = 0; square < 64; square++)
    {
        const u64 mask = 1ULL << square;

        for(int color = 0; color < 2; color++)
        {
            if ((pieceBB[color] & mask) == 0) continue;

            for(int pieceType = Pawn; pieceType <= King; pieceType++)
            {
                if(pieceBB[pieceType + 2] & mask)
                {
                    key ^= zobrist.getPiece(color, pieceType, square);
                    break;
                }
            }
        }
    }

    if(turn == Black) key ^= zobrist.getSideToMove();

    if (castleWhiteKingside)  key ^= zobrist.getCastling(0);
    if (castleWhiteQueenside) key ^= zobrist.getCastling(1);
    if (castleBlackKingside)  key ^= zobrist.getCastling(2);
    if (castleBlackQueenside) key ^= zobrist.getCastling(3);

    if (enPassantSquare >= 0)
        key ^= zobrist.getEnPassantFile(enPassantSquare % 8);

    return key;
}
