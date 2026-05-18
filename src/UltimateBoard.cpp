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

Cell UltimateBoard::metaWinner() const {
    // On construit une vue 3x3 des gagnants de chaque sous-grille
    // et on reutilise la detection d'alignement de Board.
    Board meta;
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            meta.set(br, bc, subs_[br][bc].winner());
        }
    }
    return meta.winner();
}

int UltimateBoard::countWonSubBoards(Cell player) const {
    int n = 0;
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            if (subs_[br][bc].winner() == player) ++n;
        }
    }
    return n;
}

bool UltimateBoard::allSubFinished() const {
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            if (!subs_[br][bc].isFinished()) return false;
        }
    }
    return true;
}
