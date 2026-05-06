#include "main.h"
#include "src/GameState.h"
#include "src/MCTSPlayer.h"

namespace {

void playOneGame(AIPlayer& ai) {
    GameState state;
    while (!game.isFinish()) {
        GameMove opponentMove;
        game.getMove(opponentMove);
        if (opponentMove.row >= 0 && opponentMove.col >= 0) {
            state.applyMove(Move(opponentMove.row, opponentMove.col));
        }
        Move ourMove = ai.chooseMove(state);
        state.applyMove(ourMove);
        GameMove out = { ourMove.row, ourMove.col };
        game.setMove(out);
    }
}

} // namespace

int main() {
    game.initialize(100, Level::VERY_HARD_1, Mode::ARENA, false, "RomThpt");
    MCTSPlayer ai(/*budgetMs=*/140, /*cExplore=*/1.4);
    while (!game.isAllGameFinish()) {
        playOneGame(ai);
    }
    return 0;
}
