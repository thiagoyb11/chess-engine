#include "board.h"
#include "move_generation.h"
#include "search.h"
#include <chrono>
#include <iomanip>
#include <iostream>

int main() {
    Board board;
    Search search;

    Search::SearchResult res = search.findBestMove(board, 7);
    std::cout << "score: " << res.score << '\n';
    if (res.hasMove) {
        std::cout << "move: " << static_cast<int>(res.move.start)
                  << " -> " << static_cast<int>(res.move.end) << '\n';
    }

    const auto start = std::chrono::high_resolution_clock::now();
    const long long nodes = parallelPerft(board, 7);
    std::cout << nodes << '\n';

    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start);
    const float durationSeconds = duration.count() / 1000.0f;
    std::cout << "Execution time: " << durationSeconds << "s\n";

    const float nodeRate = nodes / durationSeconds;
    std::cout << std::fixed << std::setprecision(2)
              << nodeRate << " Nodes/s\n";
    return 0;
}
