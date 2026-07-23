#pragma once

#include "board.h"
#include <string>
#include <vector>

void printBoard(const Board& board);
int decode(const std::string& position);
u64 getPossibleMoves(const Board& board, Board::enumPiece piece, int index);
int getMove(u64 start, u64 end, Board& board, Board::side turn);
int getMove(const Board::Move& move, Board& board);
std::vector<Board::Move> generateAllMoves(Board& board);
u64 perft(Board& board, int depth);
u64 parallelPerft(Board& board, int depth);
