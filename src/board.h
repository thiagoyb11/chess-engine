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
    struct Move {
        std::uint8_t start;
        std::uint8_t end;
        // Pawn means no promotion; a promoting move uses Bishop/Knight/Rook/Queen.
        enumPiece promotion;
    };
    const int pieceValues[6] = {100, 325, 300, 500, 900, 1000};

    Board();

    u64 getPieces(side s, enumPiece p) const;
    u64 getSidePieces(side s) const;
    u64 getAllPieces() const;
    u64 getPawnAttacks(side color) const;
    void removePieceAt(int idx);
    int getPieceAt(int idx) const;
    int updatePosition(int start, int end, side s, enumPiece p, enumPiece promoted = Pawn);
    u64 getPawnMoves(int idx, side turn) const;
    u64 getKnightMoves(int idx, side turn) const;
    u64 getBishopMoves(int idx, side turn) const;
    u64 getRookMoves(int idx, side turn) const;
    u64 getQueenMoves(int idx, side turn) const;
    u64 getKingMoves(int idx, side turn) const;
    bool kingAttacked(side s);
    int evalPosition(side s);
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
        bool pieceCaptured;
        enumPiece capturedPiece;
        int whiteEval;
    };

    const int rookPSQT[64] = {
        500, 500, 500, 510, 510, 505, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        520, 520, 520, 520, 520, 520, 520, 520,
        500, 500, 500, 500, 500, 500, 500, 500
    };

    const int knightPSQT[64] = {
        290, 310, 300, 300, 300, 300, 310, 290,
        300, 305, 305, 305, 305, 305, 305, 300,
        300, 305, 320, 325, 325, 325, 305, 300,
        300, 305, 325, 325, 325, 325, 305, 300,
        300, 305, 325, 325, 325, 325, 305, 300,
        300, 305, 325, 325, 325, 325, 305, 300,
        300, 305, 305, 305, 305, 305, 305, 300,
        290, 300, 300, 300, 300, 300, 300, 300
    };

    // Tables are indexed from a1 to h8 and are oriented for White.  Black
    // should use the vertically mirrored square when these are applied.
    const int pawnPSQT[64] = {
        100, 100, 100, 100, 100, 100, 100, 100,
        105, 105, 105, 105, 105, 105, 105, 105,
        110, 110, 120, 130, 130, 120, 110, 110,
        115, 115, 120, 135, 135, 120, 115, 115,
        120, 120, 125, 140, 140, 125, 120, 120,
        125, 115, 110, 120, 120, 110, 115, 125,
        135, 140, 140, 115, 115, 140, 140, 135,
        100, 100, 100, 100, 100, 100, 100, 100
    };

    const int bishopPSQT[64] = {
        305, 315, 315, 315, 315, 315, 315, 305,
        315, 325, 325, 325, 325, 325, 325, 315,
        315, 325, 335, 335, 335, 335, 325, 315,
        315, 330, 330, 335, 335, 330, 330, 315,
        315, 325, 330, 335, 335, 330, 325, 315,
        315, 330, 330, 330, 330, 330, 330, 315,
        315, 325, 325, 325, 325, 325, 325, 315,
        305, 315, 315, 315, 315, 315, 315, 305
    };

    const int queenPSQT[64] = {
        890, 895, 895, 900, 900, 895, 895, 890,
        895, 900, 900, 905, 905, 900, 900, 895,
        895, 900, 905, 905, 905, 905, 900, 895,
        900, 905, 905, 910, 910, 905, 905, 900,
        900, 905, 905, 910, 910, 905, 905, 900,
        895, 900, 905, 905, 905, 905, 900, 895,
        895, 900, 900, 905, 905, 900, 900, 895,
        890, 895, 895, 900, 900, 895, 895, 890
    };

    const int kingPSQT[64] = {
        980, 990, 990, 970, 970, 990, 990, 980,
        985, 990, 990, 980, 980, 990, 990, 985,
        980, 985, 985, 975, 975, 985, 985, 980,
        975, 980, 980, 970, 970, 980, 980, 975,
        970, 975, 975, 965, 965, 975, 975, 970,
        965, 970, 970, 960, 960, 970, 970, 965,
        960, 965, 965, 955, 955, 965, 965, 960,
        950, 955, 955, 945, 945, 955, 955, 950
    };

    void saveBoardState(const MoveState& state);
    int calculateEval(side s) const;

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
    int whiteEval;
};
