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
