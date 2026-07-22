#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

using u64 = std::uint64_t;

class Board {

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
    void loadFromFEN(const std::string& fen);
    side getTurn() const { return turn; }
    void undoMove();

private:
    // Complete pre-move state. Keeping this compact binary snapshot makes
    // undo constant-time and avoids serializing/parsing text during search.
    struct MoveState {
        u64 pieceBB[8];
        bool castleWhiteKingside;
        bool castleWhiteQueenside;
        bool castleBlackKingside;
        bool castleBlackQueenside;
        int enPassantSquare;
        side turn;
        int moveCount;
        int halfmoveClock;
    };

    void saveBoardState(const MoveState& state);

    u64 pieceBB[8];
    bool castleWhiteKingside = true;
    bool castleWhiteQueenside = true;
    bool castleBlackKingside = true;
    bool castleBlackQueenside = true;
    int enPassantSquare = -1;
    side turn = White;
    int moveCount;
    std::vector<MoveState> moveHistory;
    int halfmoveClock;
};
