#include "GameState.h"

GameState::GameState()
    : board_()
    , currentPlayer_(Cell::X)
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
    if (board_.cellAt(m.row, m.col) != Cell::EMPTY) return false;
    int br = m.row / 3, bc = m.col / 3;
    if (board_.subFinished(br, bc)) return false;
    if (forcedSubRow_ >= 0) return br == forcedSubRow_ && bc == forcedSubCol_;
    return true;
}

void GameState::undoMove(const Move& m, int prevForcedRow, int prevForcedCol) {
    currentPlayer_ = opponent(currentPlayer_);
    board_.place(m.row, m.col, Cell::EMPTY);
    --moveCount_;
    forcedSubRow_ = prevForcedRow;
    forcedSubCol_ = prevForcedCol;
}

bool GameState::applyMove(const Move& m) {
    if (!isLegal(m)) return false;
    board_.place(m.row, m.col, currentPlayer_);
    ++moveCount_;
    int nextBr = m.row % 3, nextBc = m.col % 3;
    if (board_.subFinished(nextBr, nextBc)) {
        forcedSubRow_ = -1;
        forcedSubCol_ = -1;
    } else {
        forcedSubRow_ = nextBr;
        forcedSubCol_ = nextBc;
    }
    currentPlayer_ = opponent(currentPlayer_);
    return true;
}

std::vector<Move> GameState::legalMoves() const {
    std::vector<Move> moves;
    moves.reserve(20);
    if (isFinished()) return moves;

    auto pushSubMoves = [&](int br, int bc) {
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                if (board_.sub(br, bc).isEmpty(r, c))
                    moves.push_back(Move(br * 3 + r, bc * 3 + c));
    };

    if (forcedSubRow_ >= 0) {
        pushSubMoves(forcedSubRow_, forcedSubCol_);
    } else {
        for (int br = 0; br < 3; ++br)
            for (int bc = 0; bc < 3; ++bc)
                if (!board_.subFinished(br, bc)) pushSubMoves(br, bc);
    }
    return moves;
}
