#include "Board.h"

namespace {
    // Masques 9 bits des 8 lignes (3 H, 3 V, 2 diagonales).
    constexpr std::uint16_t LINE_MASKS[8] = {
        0x007u, 0x038u, 0x1C0u, 0x049u, 0x092u, 0x124u, 0x111u, 0x054u,
    };
    constexpr std::uint16_t FULL_MASK = 0x1FFu;
}

Board::Board() : bitsX_(0), bitsO_(0) {}

Cell Board::get(int row, int col) const {
    std::uint16_t bit = 1u << (row * 3 + col);
    if (bitsX_ & bit) return Cell::X;
    if (bitsO_ & bit) return Cell::O;
    return Cell::EMPTY;
}

void Board::set(int row, int col, Cell value) {
    std::uint16_t bit = 1u << (row * 3 + col);
    bitsX_ &= ~bit;
    bitsO_ &= ~bit;
    if (value == Cell::X) bitsX_ |= bit;
    else if (value == Cell::O) bitsO_ |= bit;
}

bool Board::isEmpty(int row, int col) const {
    std::uint16_t bit = 1u << (row * 3 + col);
    return ((bitsX_ | bitsO_) & bit) == 0;
}

bool Board::isFull() const {
    return ((bitsX_ | bitsO_) & FULL_MASK) == FULL_MASK;
}

Cell Board::winner() const {
    for (int i = 0; i < 8; ++i) {
        std::uint16_t m = LINE_MASKS[i];
        if ((bitsX_ & m) == m) return Cell::X;
        if ((bitsO_ & m) == m) return Cell::O;
    }
    return Cell::EMPTY;
}

bool Board::isFinished() const {
    return winner() != Cell::EMPTY || isFull();
}
