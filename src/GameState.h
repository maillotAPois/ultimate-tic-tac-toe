#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "UltimateBoard.h"
#include "Move.h"
#include <vector>

// Etat complet d'une partie d'Ultimate Tic-Tac-Toe:
//  - le plateau 9x9
//  - le joueur courant
//  - la sous-grille dans laquelle le prochain coup doit etre joue
//    (-1 si le joueur est libre de choisir).
class GameState {
public:
    GameState();

    // Applique un coup et fait avancer le joueur courant.
    // Retourne false si le coup est illegal (cas inattendu en pratique
    // car l'IA genere toujours des coups legaux).
    bool applyMove(const Move& m);

    // Coups legaux pour le joueur courant en respectant les regles
    // de redirection UTTT.
    std::vector<Move> legalMoves() const;

    // Verifie si un coup donne est legal sans le jouer.
    bool isLegal(const Move& m) const;

    Cell currentPlayer() const { return currentPlayer_; }

    bool isFinished() const;
    Cell winner() const;

    int forcedSubRow() const { return forcedSubRow_; }
    int forcedSubCol() const { return forcedSubCol_; }

    int moveCount() const { return moveCount_; }

    const UltimateBoard& board() const { return board_; }

    // True si l'adversaire est libre de choisir sa sous-grille apres le
    // coup `m`. Utilise pour penaliser les coups qui rendent l'adversaire
    // libre (un avantage strategique pour lui).
    bool wouldGiveOpponentFreeChoice(const Move& m) const;

private:
    UltimateBoard board_;
    Cell          currentPlayer_;
    int           forcedSubRow_;
    int           forcedSubCol_;
    int           moveCount_;
};

#endif // GAME_STATE_H
