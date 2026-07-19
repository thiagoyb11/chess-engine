#include "board.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
static constexpr u64 ColA = 0x0101010101010101ULL;
static constexpr u64 ColH = 0x8080808080808080ULL;
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
      } {}

u64 Board::getPieces(side s, enumPiece p) const {
    return pieceBB[s] & pieceBB[p + 2];
}

u64 Board::getSidePieces(side s) const {
    return pieceBB[s];
}

u64 Board::getAllPieces() const {
    return pieceBB[White] | pieceBB[Black];
}

// Returns every square controlled by pawns of the requested color. Unlike
// getPawnMoves(), these squares are included even when they are empty.
u64 Board::getPawnAttacks(side color) const {
    const u64 pawns = getPieces(color, Pawn);

    if (color == White) {
        return ((pawns & ~ColA) << 7) | ((pawns & ~ColH) << 9);
    }

    return ((pawns & ~ColA) >> 9) | ((pawns & ~ColH) >> 7);
}

void Board::removePieceAt(int idx) {
    if (idx < 0 || idx >= 64) {
        return;
    }

    const u64 keepMask = ~(1ULL << idx);

    for (u64& bitboard : pieceBB) {
        bitboard &= keepMask;
    }
}

int Board::getPieceAt(int idx)
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

int Board::updatePosition(int start, int end, side s, enumPiece p) {
    if (start < 0 || start >= 64 || end < 0 || end >= 64) {
        return -1;
    }
    u64 pieceBBcopy[8];
    std::copy(std::begin(pieceBB), std::end(pieceBB), pieceBBcopy);
    const bool castleWhiteKingsideCopy = castleWhiteKingside;
    const bool castleWhiteQueensideCopy = castleWhiteQueenside;
    const bool castleBlackKingsideCopy = castleBlackKingside;
    const bool castleBlackQueensideCopy = castleBlackQueenside;
    const int enPassantSquareCopy = enPassantSquare;

    const u64 startMask = 1ULL << start;
    const u64 endMask = 1ULL << end;

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
        std::copy(std::begin(pieceBBcopy), std::end(pieceBBcopy), pieceBB);
        if (crossesCheck) {
            return -1;
        }
    }

    side enemy = (s == White) ? Black : White;

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

    enPassantSquare = -1;

    if (pieceBB[enemy] & endMask) {
        removePieceAt(end);
    }
    if (enPassantCapture) {
        removePieceAt(capturedPawnSquare);
    }
    if (p == Pawn && abs(start - end) == 16) {
        enPassantSquare = end;
    }


    if (p == King) {
        if (s == White) {
            castleWhiteKingside = castleWhiteQueenside = false;
        } else {
            castleBlackKingside = castleBlackQueenside = false;
        }
    }
    if (p == Rook) {
        if (start == 0) castleWhiteQueenside = false;
        if (start == 7) castleWhiteKingside = false;
        if (start == 56) castleBlackQueenside = false;
        if (start == 63) castleBlackKingside = false;
    }
    if (end == 0) castleWhiteQueenside = false;
    if (end == 7) castleWhiteKingside = false;
    if (end == 56) castleBlackQueenside = false;
    if (end == 63) castleBlackKingside = false;

    pieceBB[s] &= ~startMask;
    pieceBB[s] |= endMask;

    pieceBB[pieceIndex] &= ~startMask;
    pieceBB[pieceIndex] |= endMask;

    if (castling) {
        const int rookStart = kingsideCastle ? start + 3 : start - 4;
        const int rookEnd = kingsideCastle ? start + 1 : start - 1;
        const u64 rookStartMask = 1ULL << rookStart;
        const u64 rookEndMask = 1ULL << rookEnd;

        pieceBB[s] = (pieceBB[s] & ~rookStartMask) | rookEndMask;
        pieceBB[Rook + 2] = (pieceBB[Rook + 2] & ~rookStartMask) | rookEndMask;
    }

    if(kingAttacked(s))
    {
        std::copy(std::begin(pieceBBcopy), std::end(pieceBBcopy), pieceBB);
        castleWhiteKingside = castleWhiteKingsideCopy;
        castleWhiteQueenside = castleWhiteQueensideCopy;
        castleBlackKingside = castleBlackKingsideCopy;
        castleBlackQueenside = castleBlackQueensideCopy;
        enPassantSquare = enPassantSquareCopy;
        std::cout<<"Invalid Move"<<'\n';
        return -1;
    }
    return 0;
}

u64 Board::getPawnMoves(int idx, side turn) const
{
    u64 possibleMoves = 0;

    if (idx < 0 || idx >= 64) {
        return possibleMoves;
    }

    const u64 allPieces = getAllPieces();
    const u64 enemyPieces = getSidePieces(turn == White ? Black : White);
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

        // Pawns capture one square diagonally forward.
        for (const int captureCol : {col - 1, col + 1}) {
            if (captureCol >= 0 && captureCol < 8) {
                const int captureSquare = nextRow * 8 + captureCol;
                if (enemyPieces & (1ULL << captureSquare)) {
                    possibleMoves |= 1ULL << captureSquare;
                }

                // The en passant square stores the adjacent enemy pawn
                // that just advanced two squares. The landing square is
                // diagonally forward from this pawn and is empty.
                const int adjacentSquare = row * 8 + captureCol;
                if (enPassantSquare == adjacentSquare &&
                    (getPieces(turn == White ? Black : White, Pawn) &
                     (1ULL << adjacentSquare))) {
                    possibleMoves |= 1ULL << captureSquare;
                }
            }
        }
    }

    return possibleMoves;
}

u64 Board::getKnightMoves(int idx, side turn) const
{
    u64 possibleMoves = 0;
    if (idx < 0 || idx >= 64) {
        return possibleMoves;
    }

    const u64 ownPieces = getSidePieces(turn);
    const int row = idx / 8;
    const int col = idx % 8;
    constexpr int offsets[8][2] = {
        { 2,  1}, { 2, -1}, {-2,  1}, {-2, -1},
        { 1,  2}, { 1, -2}, {-1,  2}, {-1, -2}
    };

    for (const auto& offset : offsets) {
        const int targetRow = row + offset[0];
        const int targetCol = col + offset[1];
        if (targetRow >= 0 && targetRow < 8 && targetCol >= 0 && targetCol < 8) {
            const int destination = targetRow * 8 + targetCol;
            if ((ownPieces & (1ULL << destination)) == 0) {
                possibleMoves |= 1ULL << destination;
            }
        }
    }

    return possibleMoves;
}

u64 Board::getBishopMoves(int idx, side turn) const
{
    u64 possibleMoves = 0;

    if (idx < 0 || idx >= 64) {
        return possibleMoves;
    }

    const u64 ownPieces = getSidePieces(turn);
    const u64 allPieces = getAllPieces();

    const int startRow = idx / 8;
    const int startCol = idx % 8;

    // Arriba-izquierda, arriba-derecha,
    // abajo-izquierda, abajo-derecha.
    constexpr int directions[4][2] = {
        { 1, -1 },
        { 1,  1 },
        {-1, -1 },
        {-1,  1 }
    };

    for (const auto& direction : directions) {
        int row = startRow + direction[0];
        int col = startCol + direction[1];

        while (row >= 0 && row < 8 &&
               col >= 0 && col < 8) {

            int destination = row * 8 + col;
            u64 destinationMask = 1ULL << destination;

            // Una pieza propia bloquea la diagonal.
            if (ownPieces & destinationMask) {
                break;
            }

            possibleMoves |= 1ULL << destination;

            // Una pieza enemiga se puede capturar,
            // pero no se puede continuar detrás de ella.
            if (allPieces & destinationMask) {
                break;
            }

            row += direction[0];
            col += direction[1];
        }
    }

    return possibleMoves;
}

u64 Board::getRookMoves(int idx, side turn) const
{
    u64 possibleMoves = 0;

    if (idx < 0 || idx >= 64) {
        return possibleMoves;
    }

    const u64 ownPieces = getSidePieces(turn);
    const u64 allPieces = getAllPieces();

    const int startRow = idx / 8;
    const int startCol = idx % 8;
    
    constexpr int directions[4][2] = {
        { 1, 0 },
        {-1, 0 },
        { 0, -1},
        { 0, 1 }
    };

    for (const auto& direction : directions) {
        int row = startRow + direction[0];
        int col = startCol + direction[1];

        while (row >= 0 && row < 8 &&
               col >= 0 && col < 8) {

            int destination = row * 8 + col;
            u64 destinationMask = 1ULL << destination;

            // Una pieza propia bloquea la fila/columna.
            if (ownPieces & destinationMask) {
                break;
            }

            possibleMoves |= 1ULL << destination;

            // Una pieza enemiga se puede capturar,
            // pero no se puede continuar detrás de ella.
            if (allPieces & destinationMask) {
                break;
            }

            row += direction[0];
            col += direction[1];
        }
    }

    return possibleMoves;
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

    const u64 ownPieces = getSidePieces(turn);
    const int row = idx / 8;
    const int col = idx % 8;

    for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
        for (int colOffset = -1; colOffset <= 1; ++colOffset) {
            if (rowOffset == 0 && colOffset == 0) {
                continue;
            }

            const int targetRow = row + rowOffset;
            const int targetCol = col + colOffset;
            if (targetRow >= 0 && targetRow < 8 && targetCol >= 0 && targetCol < 8) {
                const int destination = targetRow * 8 + targetCol;
                if ((ownPieces & (1ULL << destination)) == 0) {
                    possibleMoves |= 1ULL << destination;
                }
            }
        }
    }

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

bool Board::kingAttacked(side s)
{
    side enemySide = static_cast<side>(!s);
    u64 kingPos = getPieces(s, King);

    u64 enemyKnights = getPieces(enemySide, Knight);
    u64 enemyBishops = getPieces(enemySide, Bishop);
    u64 enemyRooks = getPieces(enemySide, Rook);
    u64 enemyQueens = getPieces(enemySide, Queen);
    u64 enemyKing = getPieces(enemySide, King);
    u64 res;

    if (getPawnAttacks(enemySide) & kingPos) return true;

    for(int i = 0; i < 64; i++)
    {
        if(enemyKnights & (1ULL << i))
        {
            res = getKnightMoves(i, enemySide);
            if(res & kingPos) return true;
        }
    }
    for(int i = 0; i < 64; i++)
    {
        if(enemyBishops & (1ULL << i))
        {
            res = getBishopMoves(i, enemySide);
            if(res & kingPos) return true;
        }
    }
    for(int i = 0; i < 64; i++)
    {
        if(enemyRooks & (1ULL << i))
        {
            res = getRookMoves(i, enemySide);
            if(res & kingPos) return true;
        }
    }
    for(int i = 0; i < 64; i++)
    {
        if(enemyQueens & (1ULL << i))
        {
            res = getQueenMoves(i, enemySide);
            if(res & kingPos) return true;
        }
    }
    for (int i = 0; i < 64; ++i) {
        if ((enemyKing & (1ULL << i)) == 0) {
            continue;
        }

        const int kingRow = i / 8;
        const int kingCol = i % 8;
        for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
            for (int colOffset = -1; colOffset <= 1; ++colOffset) {
                if (rowOffset == 0 && colOffset == 0) {
                    continue;
                }

                const int targetRow = kingRow + rowOffset;
                const int targetCol = kingCol + colOffset;
                if (targetRow >= 0 && targetRow < 8 &&
                    targetCol >= 0 && targetCol < 8 &&
                    (kingPos & (1ULL << (targetRow * 8 + targetCol)))) {
                    return true;
                }
            }
        }
    }
    return false;
}
