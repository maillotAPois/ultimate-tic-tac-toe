#ifndef BOARD_H
#define BOARD_H

#include <cstdint>

// Symbole present dans une case du plateau
enum class Cell : char {
    EMPTY = 0,
    X     = 1,
    O     = 2,
};

inline Cell opponent(Cell c) {
    return (c == Cell::X) ? Cell::O : Cell::X;
}

// Sous-grille 3x3 du Ultimate Tic-Tac-Toe, representee par bitboards
// (un mask 9 bits par joueur). Les operations critiques (winner, isFull,
// countAlignments) deviennent O(1) via des masques de lignes precalcules
// et des operations bitwise. Gain ~5-10x sur les hot path par rapport a
// l'implementation tableau.
//
// Encodage: cellule (row, col) -> bit (row*3 + col), positions 0..8.
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

    // Acces brut au mask d'un joueur (pour les optimisations
    // d'evaluation appelees a haute frequence).
    std::uint16_t maskFor(Cell player) const {
        return (player == Cell::X) ? bitsX_ : bitsO_;
    }

private:
    // bit i (0..8) = cellule (i/3, i%3). Au plus l'un des deux mask a le
    // bit a 1 pour une position donnee.
    std::uint16_t bitsX_;
    std::uint16_t bitsO_;
};

#endif // BOARD_H
