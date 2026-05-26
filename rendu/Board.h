#ifndef BOARD_H
#define BOARD_H

#include <cstdint>

enum class Cell : char {
    EMPTY = 0,
    X     = 1,
    O     = 2,
};

inline Cell opponent(Cell c) {
    return (c == Cell::X) ? Cell::O : Cell::X;
}

// Sous-grille 3x3 du Ultimate Tic-Tac-Toe, representee par 2 bitboards
// 9-bits (un par joueur). Cellule (row, col) -> bit (row*3 + col).
class Board {
public:
    Board();

    Cell get(int row, int col) const;
    void set(int row, int col, Cell value);
    bool isEmpty(int row, int col) const;
    bool isFull() const;
    Cell winner() const;
    bool isFinished() const;

private:
    std::uint16_t bitsX_;
    std::uint16_t bitsO_;
};

#endif // BOARD_H
