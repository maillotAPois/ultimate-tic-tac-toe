#ifndef ULTIMATE_BOARD_H
#define ULTIMATE_BOARD_H

#include "Board.h"
#include <array>

// Plateau complet 9x9 d'Ultimate Tic-Tac-Toe = meta-grille 3x3 de Board.
// Conventions: tout indice (row, col) global est dans [0..8].
// La sous-grille correspondante est (row/3, col/3) et la case
// locale est (row%3, col%3).
class UltimateBoard {
public:
    UltimateBoard();

    // Acces aux sous-grilles (coords meta 0..2)
    const Board& sub(int br, int bc) const;
    Board&       sub(int br, int bc);

    // Acces a une case via coords globales (0..8)
    Cell cellAt(int row, int col) const;
    void place(int row, int col, Cell player);

    // Statut d'une sous-grille (dispo, gagnee ou pleine)
    Cell subWinner(int br, int bc) const;
    bool subFinished(int br, int bc) const;

    // Statut global
    Cell metaWinner() const;
    int  countWonSubBoards(Cell player) const;
    bool allSubFinished() const;

private:
    std::array<std::array<Board, 3>, 3> subs_;
};

#endif // ULTIMATE_BOARD_H
