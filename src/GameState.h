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

    // Annule un coup precedemment applique. Necessite les valeurs de
    // forcedSubRow/Col d'AVANT le coup (stockees par l'appelant).
    // Permet une recherche minimax sans copies de GameState (gain
    // significatif sur la profondeur effective a budget temps fixe).
    void undoMove(const Move& m, int prevForcedRow, int prevForcedCol);

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

private:
    UltimateBoard board_;
    Cell          currentPlayer_;
    int           forcedSubRow_;
    int           forcedSubCol_;
    int           moveCount_;
};

#endif // GAME_STATE_H
