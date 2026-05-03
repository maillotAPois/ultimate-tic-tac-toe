#include "main.h"
#include "src/GameState.h"
#include "src/MinimaxPlayer.h"

namespace {

// Joue une partie complete contre l'IA fournie via `game`.
// Met a jour notre etat interne en suivant les coups adverses
// renvoyes par `game.getMove(...)`.
//
// Sequencement par tour:
//   1) game.getMove(opp)   -> recuperer le coup adverse (vide au 1er tour
//                              si nous commencons)
//   2) state.applyMove     -> alterne le joueur courant vers nous
//   3) ai.chooseMove       -> calcule notre coup
//   4) state.applyMove     -> alterne le joueur courant vers l'adversaire
//   5) game.setMove        -> envoie au framework
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
    // Pour valider un niveau il faut au minimum 80% de victoires
    // (egalites non comptees). alwaysPlayFirst est ignore en mode Arene
    // (alternance automatique X/O sur les 100 parties).
    game.initialize(100, Level::VERY_HARD_2, Mode::ARENA, false, "RomThpt");

    GameState     state;
    MinimaxPlayer ai(/*fallback depth=*/5);
    // Budget temps en accord avec les timeouts indicatifs du sujet:
    //  - VERY_HARD: 200 ms par coup, 1000 ms pour le tout 1er coup.
    // On garde une marge de securite (~150 ms) pour ne pas frole le
    // timeout en cas de variabilite d'execution.
    ai.enableIterativeDeepening(/*timeBudgetMs=*/150.0, /*maxDepth=*/12);

    while (!game.isAllGameFinish()) {
        playOneGame(state, ai);
    }

    return 0;
}
