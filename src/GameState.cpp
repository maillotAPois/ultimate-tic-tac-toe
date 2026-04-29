#include "GameState.h"

GameState::GameState()
    : board_()
    , currentPlayer_(Cell::X)  // X commence par convention
    , forcedSubRow_(-1)
    , forcedSubCol_(-1)
    , moveCount_(0)
{}

bool GameState::isFinished() const {
    return board_.metaWinner() != Cell::EMPTY || board_.allSubFinished();
}

Cell GameState::winner() const {
    return board_.metaWinner();
}

bool GameState::isLegal(const Move& m) const {
    if (!m.isValid() || m.row > 8 || m.col > 8) return false;

    // La case doit etre vide
    if (board_.cellAt(m.row, m.col) != Cell::EMPTY) return false;

    int br = m.row / 3;
    int bc = m.col / 3;

    // La sous-grille cible ne doit pas etre deja terminee
    if (board_.subFinished(br, bc)) return false;

    // Si une sous-grille est imposee, on doit jouer dedans
    if (forcedSubRow_ >= 0) {
        return br == forcedSubRow_ && bc == forcedSubCol_;
    }
    return true;
}

bool GameState::applyMove(const Move& m) {
    if (!isLegal(m)) return false;

    board_.place(m.row, m.col, currentPlayer_);
    ++moveCount_;

    // La case jouee determine la sous-grille du prochain joueur
    int nextBr = m.row % 3;
    int nextBc = m.col % 3;
    if (board_.subFinished(nextBr, nextBc)) {
        // Sous-grille deja resolue/pleine: le joueur suivant est libre
        forcedSubRow_ = -1;
        forcedSubCol_ = -1;
    } else {
        forcedSubRow_ = nextBr;
        forcedSubCol_ = nextBc;
    }

    currentPlayer_ = opponent(currentPlayer_);
    return true;
}
