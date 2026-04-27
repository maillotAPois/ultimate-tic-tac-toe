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
    return Cell::EMPTY; // implementation a venir
}

bool Board::isFinished() const {
    return winner() != Cell::EMPTY || isFull();
}
