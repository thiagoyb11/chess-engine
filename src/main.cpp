#include "board.h"
#include "move_generation.h"
#include "search.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

const char files[8] = {'a', 'b', 'c', 'd', 'e', 'f' ,'g' ,'h'};
constexpr const char* StartPositionFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/"
    "RNBQKBNR w KQkq - 0 1";
const std::unordered_map<Board::enumPiece, char> promotion = {
    {Board::Queen, 'q'},
    {Board::Rook, 'r'},
    {Board::Knight, 'n'},
    {Board::Bishop, 'b'}
};

std::string moveToUci(const Board::Move& move)
{
    auto squareToUci = [](int square) {
        std::string result;
        result += static_cast<char>('a' + square % 8);
        result += static_cast<char>('1' + square / 8);
        return result;
    };

    std::string res = squareToUci(move.start) + squareToUci(move.end);

    if (move.promotion != Board::Pawn) {
        res += promotion.at(move.promotion);
    }

    return res;
}

bool promotionFromUci(char character, Board::enumPiece& promotionPiece)
{
    switch (character) {
    case 'q': promotionPiece = Board::Queen;  return true;
    case 'r': promotionPiece = Board::Rook;   return true;
    case 'b': promotionPiece = Board::Bishop; return true;
    case 'n': promotionPiece = Board::Knight; return true;
    default: return false;
    }
}

bool applyUciMove(Board& board, const std::string& uciMove)
{
    if (uciMove.size() != 4 && uciMove.size() != 5) {
        return false;
    }

    const int start = decode(uciMove.substr(0, 2));
    const int end = decode(uciMove.substr(2, 2));
    if (start < 0 || end < 0) {
        return false;
    }

    Board::enumPiece promotionPiece = Board::Pawn;
    if (uciMove.size() == 5 &&
        !promotionFromUci(uciMove[4], promotionPiece)) {
        return false;
    }

    for (const Board::Move& move : generateAllMoves(board)) {
        if (move.start != start || move.end != end ||
            move.promotion != promotionPiece) {
            continue;
        }

        return getMove(move, board) == 0;
    }

    return false;
}

bool setPosition(Board& board, std::istringstream& input)
{
    std::string token;
    if (!(input >> token)) {
        return false;
    }

    if (token == "startpos") {
        board.loadFromFEN(StartPositionFen);
    } else if (token == "fen") {
        std::string fen;
        for (int field = 0; field < 6; ++field) {
            std::string fenField;
            if (!(input >> fenField)) {
                return false;
            }
            if (!fen.empty()) {
                fen += ' ';
            }
            fen += fenField;
        }
        board.loadFromFEN(fen);
    } else {
        return false;
    }

    if (!(input >> token)) {
        return true;
    }
    if (token != "moves") {
        return false;
    }

    while (input >> token) {
        if (!applyUciMove(board, token)) {
            return false;
        }
    }

    return true;
}

int main()
{
    Board board;
    Search search;
    std::string line;

    while (std::getline(std::cin, line)) {
        std::istringstream input(line);
        std::string command;
        input >> command;

        if (command == "uci") {
            std::cout << "id name ThiagoChess\n";
            std::cout << "id author Thiago\n";
            std::cout << "uciok\n" << std::flush;
        }
        else if (command == "isready") {
            std::cout << "readyok\n" << std::flush;
        }
        else if (command == "ucinewgame") {
            board.loadFromFEN(StartPositionFen);
            search.newGame();
        }
        else if (command == "position") {
            if (!setPosition(board, input)) {
                std::cerr << "Invalid UCI position command: " << line << '\n';
            }
        }
        else if (command == "go") {
            int depth = 8;
            std::string option;

            while (input >> option) {
                if (option == "depth") {
                    int requestedDepth = 0;
                    if (input >> requestedDepth && requestedDepth > 0) {
                        depth = requestedDepth;
                    }
                }
            }

            const auto result = search.findBestMove(board, depth);

            std::cout << "bestmove "
                      << (result.hasMove ? moveToUci(result.move) : "0000")
                      << '\n' << std::flush;
        }
        else if (command == "quit") {
            break;
        }
    }
}
