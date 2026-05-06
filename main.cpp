#include "main.h"
#include "src/GameState.h"
#include "src/MCTSPlayer.h"

#include <cstdio>

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
    game.initialize(100, Level::VERY_HARD_2, Mode::ARENA, false, "RomThpt");
    MCTSPlayer ai(/*budgetMs=*/140, /*cExplore=*/1.4);
    int W = 0, L = 0, D = 0;
    while (!game.isAllGameFinish()) {
        playOneGame(ai);
        Winner w = game.getWinner();
        if      (w == IA)     ++W;
        else if (w == PLAYER) ++L;
        else                  ++D;
    }
    std::fprintf(stderr, "FINAL W=%d L=%d D=%d  rate=%.1f%%\n",
        W, L, D, (W+L) > 0 ? 100.0 * W / (W + L) : 0.0);
    return 0;
}
