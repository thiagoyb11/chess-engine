#include "board.h"
#include "magic_bitboards.h"
#include "move_generation.h"
#include <iostream>
#include <vector>
#include <string>

using u64 = uint64_t;

int main()
{
    Board board;

    // TEST 1 //

    u64 testResults_1[6] = {48, 2039, 97862, 4085603, 193690690, 8031647685};
    bool testPassed_1 = true;
    std::string fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    board.loadFromFEN(fen);

    for(int i = 0; i < 6; i++)
    {
        u64 nodes = parallelPerft(board, i + 1);

        if(nodes != testResults_1[i]) testPassed_1 = false;
    }

    if(testPassed_1)
    {
        std::cout<<"Test 1 passed!"<<'\n';
    }
    else
    {
        std::cout<<"Test 1 failed!"<<'\n';
    }

    // TEST 2 //

    u64 testResults_2[8] = {14, 191, 2812, 43238, 674624, 11030083, 178633661, 3009794393};
    bool testPassed_2 = true;
    fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ";

    board.loadFromFEN(fen);

    for(int i = 0; i < 8; i++)
    {
        u64 nodes = parallelPerft(board, i + 1);

        if(nodes != testResults_2[i]) testPassed_2 = false;
    }

    if(testPassed_2)
    {
        std::cout<<"Test 2 passed!"<<'\n';
    }
    else
    {
        std::cout<<"Test 2 failed!"<<'\n';
    }

    // TEST 3 //

    u64 testResults_3[6] = {6, 264, 9467, 422333, 15833292, 706045033};
    bool testPassed_3 = true;
    fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";

    board.loadFromFEN(fen);

    for(int i = 0; i < 6; i++)
    {
        u64 nodes = parallelPerft(board, i + 1);

        if(nodes != testResults_3[i]) testPassed_3 = false;
    }

    if(testPassed_3)
    {
        std::cout<<"Test 3 passed!"<<'\n';
    }
    else
    {
        std::cout<<"Test 3 failed!"<<'\n';
    }

    // TEST 4 //

    u64 testResults_4[5] = {44, 1486, 62379, 2103487, 89941194};
    bool testPassed_4 = true;
    fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";

    board.loadFromFEN(fen);

    for(int i = 0; i < 5; i++)
    {
        u64 nodes = parallelPerft(board, i + 1);

        if(nodes != testResults_4[i]) testPassed_4 = false;
    }

    if(testPassed_4)
    {
        std::cout<<"Test 4 passed!"<<'\n';
    }
    else
    {
        std::cout<<"Test 4 failed!"<<'\n';
    }

    if(testPassed_1 && testPassed_2 && testPassed_3 && testPassed_4)
    {
        std::cout<<"ALL TESTS PASSED!"<<'\n';
    }

    return 0;
}