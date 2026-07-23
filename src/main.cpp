#include "board.h"
#include "move_generation.h"
#include "search.h"
#include <chrono>
#include <iomanip>
#include <iostream>

int main() {
    Board board;
    Search search;
    board.loadFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    /*
    const long long nodes = parallelPerft(board, 5);
    std::cout << nodes << std::endl;

    std::cout << "Execution time: " << durationSeconds << "s\n";

    const float nodeRate = nodes / durationSeconds;
    std::cout << std::fixed << std::setprecision(2)
              << nodeRate << " Nodes/s\n"; */

    const auto start = std::chrono::high_resolution_clock::now();
    Search::SearchResult res = search.findBestMove(board, 7);
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start);
    const float durationSeconds = duration.count() / 1000.0f;
    std::cout << "score: " << res.score << '\n';
    if (res.hasMove) {
        std::cout << "move: " << static_cast<int>(res.move.start)
                  << " -> " << static_cast<int>(res.move.end) << '\n';
    }
    std::cout << "Execution time: " << durationSeconds << "s\n";
    return 0;
}
