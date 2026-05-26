#ifndef AI_PLAYER_H
#define AI_PLAYER_H

#include "GameState.h"
#include "Move.h"

// Interface d'un joueur automatise (notre IA, mais aussi des
// implementations basiques utilisees pour tester).
class AIPlayer {
public:
    virtual ~AIPlayer() {}

    // Selectionne un coup pour le joueur courant de `state`.
    // Doit toujours renvoyer un coup legal (le contrat suppose
    // que `state.legalMoves()` n'est pas vide).
    virtual Move chooseMove(const GameState& state) = 0;
};

#endif // AI_PLAYER_H
