#include "RandomPlayer.h"
#include <cstdlib>

RandomPlayer::RandomPlayer(unsigned int seed) : seed_(seed) {
    std::srand(seed_);
}

Move RandomPlayer::chooseMove(const GameState& state) {
    std::vector<Move> moves = state.legalMoves();
    if (moves.empty()) return Move();
    int idx = std::rand() % static_cast<int>(moves.size());
    return moves[idx];
}
