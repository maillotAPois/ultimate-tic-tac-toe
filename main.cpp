#include "main.h"
#include "src/GameState.h"
#include "src/MCTSPlayer.h"

#include <cstdio>

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
void playOneGame(AIPlayer& ai) {
    GameState state;

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
    // [BENCH] 20 parties en mode Arene contre MEDIUM_2 pour mesurer.
    // (En production: 100 parties pour valider le niveau a 80%.)
    game.initialize(20, Level::HARD_1, Mode::ARENA, false, "RomThpt");

    // MCTS-PUCT: budget 140ms/coup. cExplore=1.4. Tree reuse + PUCT
    // priors actifs (capture-sub favorisee, envoi-vers-sub-finie penalise).
    MCTSPlayer ai(/*budgetMs=*/140, /*cExplore=*/1.4);

    int wins = 0, losses = 0, draws = 0, total = 0;
    while (!game.isAllGameFinish()) {
        playOneGame(ai);
        Winner w = game.getWinner();
        ++total;
        if (w == IA)              ++wins;
        else if (w == PLAYER)     ++losses;
        else                      ++draws;
        std::fprintf(stderr,
            "GAME #%d done: winner=%d  running W=%d L=%d D=%d\n",
            total, (int)w, wins, losses, draws);
        std::fflush(stderr);
    }
    std::fprintf(stderr,
        "FINAL W=%d L=%d D=%d  win_rate(no_draws)=%.1f%%\n",
        wins, losses, draws,
        (wins + losses) > 0 ? 100.0 * wins / (wins + losses) : 0.0);
    std::fflush(stderr);

    return 0;
}
