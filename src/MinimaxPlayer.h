#ifndef MINIMAX_PLAYER_H
#define MINIMAX_PLAYER_H

#include "AIPlayer.h"

// Joueur base sur l'algorithme negamax (variante symetrique du minimax)
// avec elagage alpha-beta.
//
// Deux modes:
//  - profondeur fixe: utile pour le debug / la reproduction.
//  - iterative deepening: on recherche a 1, 2, 3, ... plis tant qu'il
//    reste du budget temps. Plus robuste car on a toujours un meilleur
//    coup deja calcule a la profondeur precedente.
class MinimaxPlayer : public AIPlayer {
public:
    explicit MinimaxPlayer(int depth = 5);

    // Active la recherche a profondeur croissante avec budget en ms.
    void enableIterativeDeepening(double timeBudgetMs, int maxDepth = 12);

    Move chooseMove(const GameState& state);

    void setDepth(int depth) { depth_ = depth; }
    int  depth() const { return depth_; }

private:
    int    depth_;
    bool   useID_;
    double timeBudgetMs_;
    int    maxDepth_;

    // Recherche a profondeur fixe et renvoie le meilleur coup racine.
    Move searchRoot(const GameState& state, int depth) const;

    // Renvoie le score du noeud du point de vue du joueur courant.
    int negamax(GameState state, int depth, int alpha, int beta) const;

    // Evaluation statique (heuristique) du point de vue du joueur courant.
    int evaluate(const GameState& state) const;
};

#endif // MINIMAX_PLAYER_H
