#include "UltimateBoard.h"

UltimateBoard::UltimateBoard() {
    // Boards par defaut
}

const Board& UltimateBoard::sub(int br, int bc) const {
    return subs_[br][bc];
}

Board& UltimateBoard::sub(int br, int bc) {
    return subs_[br][bc];
}

Cell UltimateBoard::cellAt(int row, int col) const {
    return subs_[row / 3][col / 3].get(row % 3, col % 3);
}

void UltimateBoard::place(int row, int col, Cell player) {
    subs_[row / 3][col / 3].set(row % 3, col % 3, player);
}

Cell UltimateBoard::subWinner(int br, int bc) const {
    return subs_[br][bc].winner();
}

bool UltimateBoard::subFinished(int br, int bc) const {
    return subs_[br][bc].isFinished();
}
