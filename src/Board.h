#ifndef BOARD_H
#define BOARD_H

#include <array>

// Symbole present dans une case du plateau
enum class Cell : char {
    EMPTY = 0,
    X     = 1,
    O     = 2,
};

inline Cell opponent(Cell c) {
    return (c == Cell::X) ? Cell::O : Cell::X;
}

// Sous-grille 3x3 du Ultimate Tic-Tac-Toe.
// Une partie complete d'UTTT est composee de 9 instances de Board
// reparties dans une meta-grille 3x3 (cf. UltimateBoard).
class Board {
public:
    Board();

    Cell get(int row, int col) const;
    void set(int row, int col, Cell value);
    bool isEmpty(int row, int col) const;

    bool isFull() const;
    Cell winner() const;       // EMPTY si aucun gagnant
    bool isFinished() const;   // gagne ou plein

    // Compte le nombre de lignes (3 cases alignees) ou ce joueur a
    // exactement `count` symboles et le reste vide. Sert a evaluer
    // les menaces (count=2) ou les positions naissantes (count=1).
    int countAlignments(Cell player, int count) const;

private:
    std::array<std::array<Cell, 3>, 3> cells_;
};

#endif // BOARD_H
