#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <future>
#endif

#include "move_generation.h"
#include <iostream>

namespace {

int makeMove(u64 start, u64 end, Board& board,
             Board::enumPiece piece, Board::side color) {
    const int endRank = static_cast<int>(end) / 8;
    const bool promotion = piece == Board::Pawn &&
        ((color == Board::White && endRank == 7) ||
         (color == Board::Black && endRank == 0));
    return board.updatePosition(
        start, end, color, piece, promotion ? Board::Queen : Board::Pawn);
}

} // namespace

void printBoard(const Board& board) {
    char boardGraph[8][8];
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            boardGraph[row][col] = '.';
        }
    }

    auto placePieces = [&](u64 bitboard, char symbol) {
        for (int square = 0; square < 64; ++square) {
            if (bitboard & (1ULL << square)) {
                boardGraph[square / 8][square % 8] = symbol;
            }
        }
    };

    placePieces(board.getPieces(Board::White, Board::Pawn), 'P');
    placePieces(board.getPieces(Board::White, Board::Bishop), 'B');
    placePieces(board.getPieces(Board::White, Board::Knight), 'N');
    placePieces(board.getPieces(Board::White, Board::Rook), 'R');
    placePieces(board.getPieces(Board::White, Board::Queen), 'Q');
    placePieces(board.getPieces(Board::White, Board::King), 'K');
    placePieces(board.getPieces(Board::Black, Board::Pawn), 'p');
    placePieces(board.getPieces(Board::Black, Board::Bishop), 'b');
    placePieces(board.getPieces(Board::Black, Board::Knight), 'n');
    placePieces(board.getPieces(Board::Black, Board::Rook), 'r');
    placePieces(board.getPieces(Board::Black, Board::Queen), 'q');
    placePieces(board.getPieces(Board::Black, Board::King), 'k');

    for (int row = 7; row >= 0; --row) {
        std::cout << row + 1 << "  ";
        for (int col = 0; col < 8; ++col) {
            std::cout << boardGraph[row][col] << ' ';
        }
        std::cout << '\n';
    }
    std::cout << "\n   a b c d e f g h\n";
}

int decode(const std::string& position) {
    if (position.size() != 2 || position[0] < 'a' || position[0] > 'h' ||
        position[1] < '1' || position[1] > '8') {
        return -1;
    }
    return (position[1] - '1') * 8 + (position[0] - 'a');
}

u64 getPossibleMoves(const Board& board, Board::enumPiece piece, int index) {
    if (index < 0 || index >= 64) {
        return 0;
    }
    const Board::side side =
        (board.getSidePieces(Board::White) & (1ULL << index))
            ? Board::White : Board::Black;

    switch (piece) {
    case Board::Pawn:   return board.getPawnMoves(index, side);
    case Board::Knight: return board.getKnightMoves(index, side);
    case Board::Bishop: return board.getBishopMoves(index, side);
    case Board::Rook:   return board.getRookMoves(index, side);
    case Board::Queen:  return board.getQueenMoves(index, side);
    case Board::King:   return board.getKingMoves(index, side);
    }
    return 0;
}

int getMove(u64 start, u64 end, Board& board, Board::side turn) {
    if (start >= 64 || end >= 64 ||
        !(board.getSidePieces(turn) & (1ULL << start))) {
        return -1;
    }

    const int pieceIndex = board.getPieceAt(static_cast<int>(start));
    if (pieceIndex < 0) {
        return -1;
    }
    const auto piece = static_cast<Board::enumPiece>(pieceIndex);
    if (!(getPossibleMoves(board, piece, static_cast<int>(start)) &
          (1ULL << end))) {
        return -1;
    }
    return makeMove(start, end, board, piece, turn);
}

int getMove(const Board::Move& move, Board& board) {
    const Board::side turn = board.getTurn();
    const int pieceIndex = board.getPieceAt(move.start);
    if (pieceIndex < 0 ||
        !(board.getSidePieces(turn) & (1ULL << move.start))) {
        return -1;
    }
    const auto piece = static_cast<Board::enumPiece>(pieceIndex);
    return board.updatePosition(move.start, move.end, turn, piece,
                                move.promotion);
}

std::vector<Board::Move> generateAllMoves(Board& board) {
    const Board::side turn = board.getTurn();
    u64 sidePieces = board.getSidePieces(turn);
    std::vector<Board::Move> moves;
    moves.reserve(64);

    while (sidePieces) {
        const int start = __builtin_ctzll(sidePieces);
        sidePieces &= sidePieces - 1;
        const auto piece = static_cast<Board::enumPiece>(board.getPieceAt(start));
        u64 destinations = getPossibleMoves(board, piece, start);

        while (destinations) {
            const int end = __builtin_ctzll(destinations);
            destinations &= destinations - 1;
            const int endRank = end / 8;
            const bool promotion = piece == Board::Pawn &&
                ((turn == Board::White && endRank == 7) ||
                 (turn == Board::Black && endRank == 0));

            if (promotion) {
                moves.push_back({static_cast<std::uint8_t>(start),
                                 static_cast<std::uint8_t>(end), Board::Queen});
                moves.push_back({static_cast<std::uint8_t>(start),
                                 static_cast<std::uint8_t>(end), Board::Rook});
                moves.push_back({static_cast<std::uint8_t>(start),
                                 static_cast<std::uint8_t>(end), Board::Bishop});
                moves.push_back({static_cast<std::uint8_t>(start),
                                 static_cast<std::uint8_t>(end), Board::Knight});
            } else {
                moves.push_back({static_cast<std::uint8_t>(start),
                                 static_cast<std::uint8_t>(end), Board::Pawn});
            }
        }
    }
    return moves;
}

u64 perft(Board& board, int depth) {
    if (depth == 0) {
        return 1;
    }

    u64 nodes = 0;
    const auto moves = generateAllMoves(board);
    for (const Board::Move& move : moves) {
        if (getMove(move, board) == 0) {
            nodes += perft(board, depth - 1);
            board.undoMove();
        }
    }
    return nodes;
}

#ifdef _WIN32
struct ParallelPerftJob {
    Board board;
    Board::Move move;
    int depth;
    u64* result;
};

unsigned __stdcall runParallelPerftJob(void* rawJob) {
    auto* job = static_cast<ParallelPerftJob*>(rawJob);
    if (getMove(job->move, job->board) == 0) {
        *job->result = perft(job->board, job->depth - 1);
    }
    delete job;
    return 0;
}
#endif

u64 parallelPerft(Board& board, int depth) {
    if (depth == 0) {
        return 1;
    }

    Board root = board;
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
            Board local = root;
            if (getMove(moves[i], local) == 0) {
                results[i] = perft(local, depth - 1);
            }
        }
    }
    for (HANDLE job : jobs) {
        WaitForSingleObject(job, INFINITE);
        CloseHandle(job);
    }
#else
    std::vector<std::future<u64>> futures;
    futures.reserve(moves.size());
    for (const Board::Move& move : moves) {
        futures.emplace_back(std::async(
            std::launch::async,
            [root, move, depth]() mutable -> u64 {
                if (getMove(move, root) == 0) {
                    return perft(root, depth - 1);
                }
                return 0;
            }));
    }
    for (std::size_t i = 0; i < futures.size(); ++i) {
        results[i] = futures[i].get();
    }
#endif

    u64 nodes = 0;
    for (const u64 result : results) {
        nodes += result;
    }
    return nodes;
}
