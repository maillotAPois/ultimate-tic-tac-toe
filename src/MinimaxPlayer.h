#ifndef MINIMAX_PLAYER_H
#define MINIMAX_PLAYER_H

#include "AIPlayer.h"

// Joueur base sur l'algorithme negamax (variante symetrique du minimax)
// avec elagage alpha-beta. Profondeur fixe pour cette premiere version.
//
// Le score est evalue du point de vue du joueur courant a chaque noeud:
//   +VALEUR_GAIN si le joueur courant a gagne,
//   -VALEUR_GAIN s'il a perdu,
//   sinon: difference de sous-grilles gagnees, ponderee.
class MinimaxPlayer : public AIPlayer {
public:
    explicit MinimaxPlayer(int depth = 5);

    Move chooseMove(const GameState& state);

    void setDepth(int depth) { depth_ = depth; }
    int  depth() const { return depth_; }

private:
    int depth_;

    // Renvoie le score du noeud du point de vue du joueur courant.
    int negamax(GameState state, int depth, int alpha, int beta) const;

    // Evaluation statique (heuristique) du point de vue du joueur courant.
    int evaluate(const GameState& state) const;
};

#endif // MINIMAX_PLAYER_H
