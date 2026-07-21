#include "board.h"
#include <cstdlib>
#include <iostream>
#include <string>
#define INF INT32_MAX;

void printBoard(const Board& cboard);
int makeMove(std::string move, Board& cboard, Board::enumPiece p, Board::side color);
int decode(const std::string& pos);
int getMove(std::string move, Board& cboard, Board::side turn);
u64 getPossibleMoves(const Board& cboard, Board::enumPiece p, int index);



void printBoard(const Board& cboard) {
    char boardGraph[8][8];

    // Inicializar todas las casillas como vacías
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            boardGraph[row][col] = '.';
        }
    }

    auto placePieces = [&](u64 bitboard, char symbol) {
        for (int square = 0; square < 64; square++) {
            if (bitboard & (1ULL << square)) {
                int row = square / 8;
                int col = square % 8;

                boardGraph[row][col] = symbol;
            }
        }
    };

    // Blancas
    placePieces(cboard.getPieces(Board::White, Board::Pawn),   'P');
    placePieces(cboard.getPieces(Board::White, Board::Bishop), 'B');
    placePieces(cboard.getPieces(Board::White, Board::Knight), 'N');
    placePieces(cboard.getPieces(Board::White, Board::Rook),   'R');
    placePieces(cboard.getPieces(Board::White, Board::Queen),  'Q');
    placePieces(cboard.getPieces(Board::White, Board::King),   'K');

    // Negras
    placePieces(cboard.getPieces(Board::Black, Board::Pawn),   'p');
    placePieces(cboard.getPieces(Board::Black, Board::Bishop), 'b');
    placePieces(cboard.getPieces(Board::Black, Board::Knight), 'n');
    placePieces(cboard.getPieces(Board::Black, Board::Rook),   'r');
    placePieces(cboard.getPieces(Board::Black, Board::Queen),  'q');
    placePieces(cboard.getPieces(Board::Black, Board::King),   'k');

    for (int row = 7; row >= 0; row--) {
        std::cout << row + 1 << "  ";

        for (int col = 0; col < 8; col++) {
            std::cout << boardGraph[row][col] << ' ';
        }

        std::cout << '\n';
    }

    std::cout << "\n   a b c d e f g h\n";
}

int decode(const std::string& pos)
{
    if (pos.size() != 2 ||
        pos[0] < 'a' || pos[0] > 'h' ||
        pos[1] < '1' || pos[1] > '8') {
        return -1;
    }

    int row = pos[1] - '1';
    int col = pos[0] - 'a';

    return row * 8 + col;
}

u64 getPossibleMoves(const Board& cboard, Board::enumPiece p, int index)
{
    u64 possibleMoves = 0;
    Board::side turno = (cboard.getSidePieces(Board::White) & (1ULL << index))
        ? Board::White
        : Board::Black;
    
    switch(p)
    {
        case Board::Pawn:
        {
            possibleMoves = cboard.getPawnMoves(index, turno);
            break;
        }
        case Board::Knight:
        {
            possibleMoves = cboard.getKnightMoves(index, turno);
            break;
        }
        case Board::Bishop:
        {
            possibleMoves = cboard.getBishopMoves(index, turno);
            break;
        }
        case Board::Rook:
        {
            possibleMoves = cboard.getRookMoves(index, turno);
            break;
        }
        case Board::Queen:
        {
            possibleMoves = cboard.getQueenMoves(index, turno);
            break;
        }
        case Board::King:
        {
            possibleMoves = cboard.getKingMoves(index, turno);
            break;
        }

    }
    return possibleMoves;
}

int getMove(std::string move, Board& cboard, Board::side turn)
{
    Board::enumPiece p;
    int idxStart = decode(move.substr(0, 2));
    int idxEnd = decode(move.substr(2, 2));
    int pieceIdx = cboard.getPieceAt(idxStart);
    if(pieceIdx != -1)
    {
        p = static_cast<Board::enumPiece>(pieceIdx);
    }
    else
    {
        return -1;
    }
    
    // Si quiero mover una pieza blanca en el turno de las blancas
    if(((cboard.getSidePieces(Board::White) & (1ULL << idxStart)) && turn == Board::White) || ((cboard.getSidePieces(Board::Black) & (1ULL << idxStart)) && turn == Board::Black))
    {
        const u64 possibleMoves = getPossibleMoves(cboard, p, idxStart);

        // Chequear si el movimiento es valido (el destino esta en el bitboard).
        if (idxEnd >= 0 && idxEnd < 64 && (possibleMoves & (1ULL << idxEnd))) {
            int returnVal = makeMove(move, cboard, p, turn);
            return returnVal;
        }
        return -1;
    }
    else
    {
        return -1;
    }
    
};

std::vector<std::string> generateAllMoves(Board& cboard)
{
    u64 sidePieces = cboard.getSidePieces(cboard.getTurn());
    std::vector<std::string> movesVector;

    for(int i = 0; i < 64; i++)
    {
        if(sidePieces & (1ULL << i))
        {
            int pieceIdx = cboard.getPieceAt(i);
            Board::enumPiece p = static_cast<Board::enumPiece>(pieceIdx);
            u64 moves = 0;

            switch(p)
            {
                case Board::enumPiece::Pawn:
                    moves = cboard.getPawnMoves(i, cboard.getTurn());
                    break;
                case Board::enumPiece::Knight:
                    moves = cboard.getKnightMoves(i, cboard.getTurn());
                    break;
                case Board::enumPiece::Bishop:
                    moves = cboard.getBishopMoves(i, cboard.getTurn());
                    break;
                case Board::enumPiece::Rook:
                    moves = cboard.getRookMoves(i, cboard.getTurn());
                    break;
                case Board::enumPiece::Queen:
                    moves = cboard.getQueenMoves(i, cboard.getTurn());
                    break;
                case Board::enumPiece::King:
                    moves = cboard.getKingMoves(i, cboard.getTurn());
                    break;
            }

            for(int j = 0; j < 64; j++)
            {
                if(moves & (1ULL << j))
                {
                    std::string move = "....";
                    move[0] = 'a' + (i % 8);
                    move[1] = '1' + (i / 8);
                    move[2] = 'a' + (j % 8);
                    move[3] = '1' + (j / 8);
                    movesVector.push_back(move);
                }
            }
        }
    }
    return movesVector;
}

u64 perft(Board& cboard, int depth)
{
    if(depth == 0) return 1;

    u64 nodes = 0;
    std::vector<std::string> moves = generateAllMoves(cboard);

    for(const std::string& move : moves)
    {
        Board::side turnColor = cboard.getTurn();
        int returnVal = getMove(move, cboard, turnColor);
        if(returnVal == 0)
        {
            nodes += perft(cboard, depth - 1);
            // Undo the move
            cboard.undoMove();
        }
    }

    return nodes;
}

int makeMove(std::string move, Board& cboard, Board::enumPiece p, Board::side color) {
    int idxStart = decode(move.substr(0, 2));
    int idxEnd = decode(move.substr(2, 2));
    
    int returnVal = cboard.updatePosition(idxStart, idxEnd, color, p);

    return returnVal;
}

int evalPosition(Board cboard, Board::side s)
{
    int score = 0;
    u64 allPieces = cboard.getAllPieces();
    u64 sidePieces = cboard.getSidePieces(s);
    
    for(int i = 0; i < 64; i++)
    {
        if(allPieces & (1ULL << i))
        {
            int pieceIdx = cboard.getPieceAt(i);
            Board::enumPiece p = static_cast<Board::enumPiece>(pieceIdx);
            int pieceValue = 0;

            switch(p)
            {
                case Board::Pawn:
                    pieceValue = 100;
                    break;
                case Board::Knight:
                    pieceValue = 300;
                    break;
                case Board::Bishop:
                    pieceValue = 300;
                    break;
                case Board::Rook:
                    pieceValue = 500;
                    break;
                case Board::Queen:
                    pieceValue = 900;
                    break;
                case Board::King:
                    pieceValue = 1000;
                    break;
            }

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

int negamax(Board cboard, int depth)
{
    if(depth == 0) return evalPosition(cboard, cboard.getTurn());
    int maxEval = -INF;

    std::vector<std::string> moves = generateAllMoves(cboard);

    for(auto x : moves)
    {
        int score = -negamax(cboard, depth - 1);
        if(score > maxEval) maxEval = score;
    }
    return maxEval;
}

#include <future>

u64 parallelPerft(Board& cboard, int depth) {
    if (depth == 0) return 1;

    const auto moves = generateAllMoves(cboard);
    std::vector<std::future<u64>> jobs;

    for (const auto& move : moves) {
        jobs.push_back(std::async(std::launch::async, [&cboard, move, depth] {
            Board local = cboard;  // independent board for this worker

            if (getMove(move, local, local.getTurn()) != 0) {
                return u64{0};     // pseudo-legal move rejected as illegal
            }

            return perft(local, depth - 1);
        }));
    }

    u64 nodes = 0;
    for (auto& job : jobs) {
        nodes += job.get();
    }
    return nodes;
}

int main() {
    Board cboard;
    bool turn = 0;

    std::cout<<parallelPerft(cboard, 6)<<'\n';
    
    /*while(true){
        std::string move;
        std::cin>>move;
        Board::side turnColor = turn == 0 ? Board::White : Board::Black;
        int returnVal = getMove(move, cboard, turnColor);
        if(returnVal == 0) turn = !turn;
    } */

    return 0;
}
