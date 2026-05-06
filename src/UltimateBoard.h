#ifndef ULTIMATE_BOARD_H
#define ULTIMATE_BOARD_H

#include "Board.h"
#include <array>

// Plateau 9x9 = meta-grille 3x3 de Board. Indices globaux (row, col)
// dans [0..8]; sous-grille = (row/3, col/3); case locale = (row%3, col%3).
class UltimateBoard {
public:
    UltimateBoard() = default;

    const Board& sub(int br, int bc) const { return subs_[br][bc]; }
    Board&       sub(int br, int bc)       { return subs_[br][bc]; }

    Cell cellAt(int row, int col) const;
    void place(int row, int col, Cell player);

    Cell subWinner(int br, int bc) const;
    bool subFinished(int br, int bc) const;

    Cell metaWinner() const;
    int  countWonSubBoards(Cell player) const;
    bool allSubFinished() const;

private:
    std::array<std::array<Board, 3>, 3> subs_;
};

#endif // ULTIMATE_BOARD_H
