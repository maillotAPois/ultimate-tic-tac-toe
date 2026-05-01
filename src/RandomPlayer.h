#ifndef RANDOM_PLAYER_H
#define RANDOM_PLAYER_H

#include "AIPlayer.h"

// Joueur de reference qui pioche un coup legal au hasard.
// Sert de baseline pour la non-regression des autres IA.
class RandomPlayer : public AIPlayer {
public:
    explicit RandomPlayer(unsigned int seed = 42u);
    Move chooseMove(const GameState& state);

private:
    unsigned int seed_;
};

#endif // RANDOM_PLAYER_H
