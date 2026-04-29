#include "GameState.h"

GameState::GameState()
    : board_()
    , currentPlayer_(Cell::X)  // X commence par convention
    , forcedSubRow_(-1)
    , forcedSubCol_(-1)
    , moveCount_(0)
{}

void GameState::reset() {
    *this = GameState();
}

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

bool GameState::wouldGiveOpponentFreeChoice(const Move& m) const {
    if (!m.isValid()) return false;
    int nextBr = m.row % 3;
    int nextBc = m.col % 3;
    // Apres le coup, la sous-grille cible serait-elle finie ?
    // On regarde le statut actuel: si elle est deja finie, oui.
    // Si elle ne l'est pas, on verifie si le coup la termine.
    if (board_.subFinished(nextBr, nextBc)) return true;
    // Le coup pourrait aussi terminer la sous-grille cible si on joue dedans
    // (cas: forcedSub == nextSub et le coup gagne ou remplit).
    if (m.row / 3 == nextBr && m.col / 3 == nextBc) {
        Board sim = board_.sub(nextBr, nextBc);
        sim.set(m.row % 3, m.col % 3, currentPlayer_);
        if (sim.isFinished()) return true;
    }
    return false;
}

std::vector<Move> GameState::legalMoves() const {
    std::vector<Move> moves;
    moves.reserve(20);  // borne raisonnable, evite des reallocations

    if (isFinished()) return moves;

    if (forcedSubRow_ >= 0) {
        // Joueur force de jouer dans une sous-grille precise
        int br = forcedSubRow_;
        int bc = forcedSubCol_;
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                if (board_.sub(br, bc).isEmpty(r, c)) {
                    moves.push_back(Move(br * 3 + r, bc * 3 + c));
                }
            }
        }
        return moves;
    }

    // Libre: parcourir toutes les sous-grilles non terminees
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            if (board_.subFinished(br, bc)) continue;
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    if (board_.sub(br, bc).isEmpty(r, c)) {
                        moves.push_back(Move(br * 3 + r, bc * 3 + c));
                    }
                }
            }
        }
    }
    return moves;
}
