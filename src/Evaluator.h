#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "GameState.h"

// Fonction d'evaluation heuristique d'une position.
// L'evaluation est exprimee du point de vue du joueur courant
// du `GameState` (compatible negamax).
//
// Composantes:
//   1. Sous-grilles gagnees, ponderees par la valeur strategique de
//      leur position dans la meta-grille (centre > coins > bords).
//   2. Menaces de 2-en-ligne sur la meta-grille (sous-grilles alignees
//      avec une troisieme encore jouable).
//   3. Menaces de 2-en-ligne au sein des sous-grilles non terminees.
//   4. Penalite si l'adversaire est libre de choisir sa sous-grille
//      (ou s'il est envoye sur une sous-grille ou il a une menace).
//   5. Bonus pour la possession du centre (4,4) du plateau global.
class Evaluator {
public:
    static int evaluate(const GameState& state);

private:
    // Poids strategique d'une sous-grille a la position (br,bc) de la meta:
    //   centre = 4, coins = 3, bords = 2.
    static int subPositionWeight(int br, int bc);
};

#endif // EVALUATOR_H
