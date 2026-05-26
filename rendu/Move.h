#ifndef MOVE_H
#define MOVE_H

// Coup dans le repere global du plateau 9x9.
// row, col sont dans [0..8]. row=-1 indique un coup invalide
// (par convention: aucun coup, premier coup d'une partie, etc.).
struct Move {
    int row;
    int col;

    Move() : row(-1), col(-1) {}
    Move(int r, int c) : row(r), col(c) {}

    bool isValid() const { return row >= 0 && col >= 0; }
};

#endif // MOVE_H
