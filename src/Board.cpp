#include "Board.h"

namespace {
    // Masques 9 bits des 8 lignes possibles (3 horizontales, 3 verticales,
    // 2 diagonales). Indice de bit = row*3 + col.
    constexpr std::uint16_t LINE_MASKS[8] = {
        0x007u, // ligne 0 : (0,0) (0,1) (0,2)
        0x038u, // ligne 1 : (1,0) (1,1) (1,2)
        0x1C0u, // ligne 2 : (2,0) (2,1) (2,2)
        0x049u, // col  0  : (0,0) (1,0) (2,0)
        0x092u, // col  1  : (0,1) (1,1) (2,1)
        0x124u, // col  2  : (0,2) (1,2) (2,2)
        0x111u, // diag NW-SE : (0,0) (1,1) (2,2)
        0x054u, // diag NE-SW : (0,2) (1,1) (2,0)
    };
    constexpr std::uint16_t FULL_MASK = 0x1FFu; // 9 bits a 1

    // popcount portable pour un mot 16-bits (compile en POPCNT sur x86).
    inline int popcount9(std::uint16_t x) {
        int c = 0;
        while (x) { x &= (x - 1); ++c; }
        return c;
    }
}

Board::Board()
    : bitsX_(0)
    , bitsO_(0)
{}

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

int Board::countAlignments(Cell player, int count) const {
    std::uint16_t pl = (player == Cell::X) ? bitsX_ : bitsO_;
    std::uint16_t op = (player == Cell::X) ? bitsO_ : bitsX_;
    int total = 0;
    for (int i = 0; i < 8; ++i) {
        std::uint16_t m = LINE_MASKS[i];
        if ((op & m) != 0) continue;          // ligne morte (adv present)
        if (popcount9(static_cast<std::uint16_t>(pl & m)) == count) ++total;
    }
    return total;
}
