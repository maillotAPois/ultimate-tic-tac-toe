#ifndef MINIMAX_PLAYER_H
#define MINIMAX_PLAYER_H

#include "AIPlayer.h"

// Joueur base sur l'algorithme negamax (variante symetrique du minimax)
// avec elagage alpha-beta. Profondeur fixe.
//
// L'evaluation est exprimee du point de vue du joueur courant a chaque
// noeud:
//   +VALEUR_GAIN si le joueur courant a gagne,
//   -VALEUR_GAIN s'il a perdu,
//   sinon: difference de sous-grilles gagnees, ponderee.
class MinimaxPlayer : public AIPlayer {
public:
    explicit MinimaxPlayer(int depth = 4);

    Move chooseMove(const GameState& state);

private:
    int depth_;

    int negamax(GameState state, int depth, int alpha, int beta) const;
    int evaluate(const GameState& state) const;
};

#endif // MINIMAX_PLAYER_H
