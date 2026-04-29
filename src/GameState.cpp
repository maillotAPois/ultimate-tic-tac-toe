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
