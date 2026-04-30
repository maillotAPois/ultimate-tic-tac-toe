#include "main.h"
#include "src/GameState.h"
#include "src/RandomPlayer.h"

namespace {

// Joue une partie complete contre l'IA fournie via `game`.
// Met a jour notre etat interne en suivant les coups adverses
// renvoyes par `game.getMove(...)`.
void playOneGame(GameState& state, AIPlayer& ai) {
    state.reset();

    while (!game.isFinish()) {
        // 1) Recuperer le coup adverse (si disponible)
        GameMove opponentMove;
        game.getMove(opponentMove);
        if (opponentMove.row >= 0 && opponentMove.col >= 0) {
            state.applyMove(Move(opponentMove.row, opponentMove.col));
        }

        // 2) Calculer notre coup et l'appliquer localement
        Move ourMove = ai.chooseMove(state);
        state.applyMove(ourMove);

        // 3) Envoyer le coup au framework
        GameMove out = { ourMove.row, ourMove.col };
        game.setMove(out);
    }
}

} // namespace

int main() {
    // Initialisation: 100 parties en mode Arene contre VERY_HARD_2.
    // alwaysPlayFirst est ignore en mode Arene (alternance auto X/O).
    game.initialize(100, Level::VERY_HARD_2, Mode::ARENA, false, "RomThpt");

    GameState state;
    RandomPlayer ai;  // Sera remplace par MinimaxPlayer dans la prochaine PR

    while (!game.isAllGameFinish()) {
        playOneGame(state, ai);
    }

    return 0;
}
