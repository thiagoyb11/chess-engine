#ifdef _WIN32
#include <process.h>
#include <windows.h>
#endif

#include "board.h"
#include <cstdlib>
#include <iostream>
#include <string>
#define INF INT32_MAX;

void printBoard(const Board& cboard);
int makeMove(u64 idxStart, u64 idxEnd, Board& cboard, Board::enumPiece p, Board::side color);
int decode(const std::string& pos);
int getMove(u64 idxStart, u64 idxEnd, Board& cboard, Board::side turn);
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

int getMove(u64 idxStart, u64 idxEnd, Board& cboard, Board::side turn)
{
    Board::enumPiece p;
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
            int returnVal = makeMove(idxStart, idxEnd, cboard, p, turn);
            return returnVal;
        }
        return -1;
    }
    else
    {
        return -1;
    }
    
};

std::vector<u64> generateAllMoves(Board& cboard)
{
    Board::side turn = cboard.getTurn();
    u64 sidePieces = cboard.getSidePieces(turn);
    std::vector<u64> movesVector;

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
                    moves = cboard.getPawnMoves(i, turn);
                    break;
                case Board::enumPiece::Knight:
                    moves = cboard.getKnightMoves(i, turn);
                    break;
                case Board::enumPiece::Bishop:
                    moves = cboard.getBishopMoves(i, turn);
                    break;
                case Board::enumPiece::Rook:
                    moves = cboard.getRookMoves(i, turn);
                    break;
                case Board::enumPiece::Queen:
                    moves = cboard.getQueenMoves(i, turn);
                    break;
                case Board::enumPiece::King:
                    moves = cboard.getKingMoves(i, turn);
                    break;
            }
            movesVector.push_back(i);
            movesVector.push_back(moves);
        }
    }
    return movesVector;
}

u64 perft(Board& cboard, int depth)
{
    if(depth == 0) return 1;

    u64 nodes = 0;
    std::vector<u64> moves = generateAllMoves(cboard);

    for(int i = 0; i < moves.size(); i += 2)
    {
        Board::side turnColor = cboard.getTurn();
        for(int j = 0; j < 64; j++)
        {
            if(1ULL << j & moves[i + 1])
            {
                int returnVal = getMove(moves[i], j, cboard, turnColor);
                if(returnVal == 0)
                {
                    nodes += perft(cboard, depth - 1);
                    // Undo the move
                    cboard.undoMove();
                }
            }
        }
    }

    return nodes;
}

int makeMove(u64 startIdx, u64 endIdx, Board& cboard, Board::enumPiece p, Board::side color) {
    int returnVal = cboard.updatePosition(startIdx, endIdx, color, p);

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

/*int negamax(Board cboard, int depth)
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
}*/

#ifdef _WIN32
struct ParallelPerftJob {
    Board board;
    std::string move;
    int depth;
    u64* result;
};

unsigned __stdcall runParallelPerftJob(void* rawJob) {
    auto* job = static_cast<ParallelPerftJob*>(rawJob);
    if (getMove(job->move, job->board, job->board.getTurn()) == 0) {
        *job->result = perft(job->board, job->depth - 1);
    }
    delete job;
    return 0;
}
#endif

/*u64 parallelPerft(Board& cboard, int depth) {
    if (depth == 0) return 1;

    // Take one snapshot before launching workers.  Each worker must operate
    // on its own board, and should not capture the caller's board by
    // reference while other workers are running.
    Board root = cboard;
    const auto moves = generateAllMoves(root);
    std::vector<u64> results(moves.size(), 0);

#ifdef _WIN32
    std::vector<HANDLE> jobs;
    for (std::size_t i = 0; i < moves.size(); ++i) {
        auto* job = new ParallelPerftJob{root, moves[i], depth, &results[i]};
        HANDLE handle = reinterpret_cast<HANDLE>(_beginthreadex(
            nullptr, 0, runParallelPerftJob, job, 0, nullptr));
        if (handle != nullptr) {
            jobs.push_back(handle);
        } else {
            delete job;
            // Preserve correctness if the OS refuses to create a worker.
            Board local = root;
            if (getMove(moves[i], local, local.getTurn()) == 0) {
                results[i] = perft(local, depth - 1);
            }
        }
    }

    for (HANDLE job : jobs) {
        WaitForSingleObject(job, INFINITE);
        CloseHandle(job);
    }
#else
    // Keep a portable synchronous fallback for non-Windows builds.  The
    // Windows build uses native workers because the bundled MinGW runtime
    // does not provide std::thread/std::future.
    for (std::size_t i = 0; i < moves.size(); ++i) {
        Board local = root;
        if (getMove(moves[i], local, local.getTurn()) == 0) {
            results[i] = perft(local, depth - 1);
        }
    }
#endif

    u64 nodes = 0;
    for (const u64 result : results) {
        nodes += result;
    }
    return nodes;
}*/

int main() {
    Board cboard;

    bool turn = 0;

    std::cout<<perft(cboard, 6)<<'\n';
    
    /*while(true){
        std::string move;
        std::cin>>move;
        Board::side turnColor = turn == 0 ? Board::White : Board::Black;
        int returnVal = getMove(move, cboard, turnColor);
        if(returnVal == 0) turn = !turn;
    } */

    return 0;
}
