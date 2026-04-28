#include "Board.h"

Board::Board() {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            cells_[r][c] = Cell::EMPTY;
        }
    }
}

Cell Board::get(int row, int col) const {
    return cells_[row][col];
}

void Board::set(int row, int col, Cell value) {
    cells_[row][col] = value;
}

bool Board::isEmpty(int row, int col) const {
    return cells_[row][col] == Cell::EMPTY;
}

bool Board::isFull() const {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (cells_[r][c] == Cell::EMPTY) return false;
        }
    }
    return true;
}

Cell Board::winner() const {
    // Lignes
    for (int r = 0; r < 3; ++r) {
        Cell c0 = cells_[r][0];
        if (c0 != Cell::EMPTY && c0 == cells_[r][1] && c0 == cells_[r][2]) {
            return c0;
        }
    }
    // Colonnes
    for (int c = 0; c < 3; ++c) {
        Cell c0 = cells_[0][c];
        if (c0 != Cell::EMPTY && c0 == cells_[1][c] && c0 == cells_[2][c]) {
            return c0;
        }
    }
    // Diagonales
    Cell d0 = cells_[0][0];
    if (d0 != Cell::EMPTY && d0 == cells_[1][1] && d0 == cells_[2][2]) {
        return d0;
    }
    Cell d1 = cells_[0][2];
    if (d1 != Cell::EMPTY && d1 == cells_[1][1] && d1 == cells_[2][0]) {
        return d1;
    }
    return Cell::EMPTY;
}

bool Board::isFinished() const {
    return winner() != Cell::EMPTY || isFull();
}

namespace {
    struct Line { int r0, c0, r1, c1, r2, c2; };
    static const Line kLines[8] = {
        {0,0, 0,1, 0,2}, {1,0, 1,1, 1,2}, {2,0, 2,1, 2,2}, // lignes
        {0,0, 1,0, 2,0}, {0,1, 1,1, 2,1}, {0,2, 1,2, 2,2}, // colonnes
        {0,0, 1,1, 2,2}, {0,2, 1,1, 2,0}                   // diagonales
    };
}

int Board::countAlignments(Cell player, int count) const {
    int total = 0;
    for (int i = 0; i < 8; ++i) {
        const Line& L = kLines[i];
        Cell cs[3] = {
            cells_[L.r0][L.c0],
            cells_[L.r1][L.c1],
            cells_[L.r2][L.c2]
        };
        int playerCount = 0;
        int emptyCount  = 0;
        for (int k = 0; k < 3; ++k) {
            if (cs[k] == player)      ++playerCount;
            else if (cs[k] == Cell::EMPTY) ++emptyCount;
        }
        if (playerCount == count && playerCount + emptyCount == 3) {
            ++total;
        }
    }
    return total;
}
