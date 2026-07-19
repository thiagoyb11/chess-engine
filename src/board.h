#pragma once

#include <cstdint>

using u64 = std::uint64_t;

class Board {
    u64 pieceBB[8];
    bool castleWhiteKingside = true;
    bool castleWhiteQueenside = true;
    bool castleBlackKingside = true;
    bool castleBlackQueenside = true;
    int enPassantSquare = -1;

public:
    enum side { White, Black };
    enum enumPiece { Pawn, Bishop, Knight, Rook, Queen, King };

    Board();

    u64 getPieces(side s, enumPiece p) const;
    u64 getSidePieces(side s) const;
    u64 getAllPieces() const;
    u64 getPawnAttacks(side color) const;
    void removePieceAt(int idx);
    int getPieceAt(int idx);
    int updatePosition(int start, int end, side s, enumPiece p);
    u64 getPawnMoves(int idx, side turn) const;
    u64 getKnightMoves(int idx, side turn) const;
    u64 getBishopMoves(int idx, side turn) const;
    u64 getRookMoves(int idx, side turn) const;
    u64 getQueenMoves(int idx, side turn) const;
    u64 getKingMoves(int idx, side turn) const;
    bool kingAttacked(side s);
};
