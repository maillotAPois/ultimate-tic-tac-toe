// Harnais de self-play: moteur A contre moteur B, sans framework.
// Deterministe (aucun alea) => comparaison A/B a bruit nul.
// Compilation native: voir cible `selfplay` du Makefile.

#include "src/GameState.h"
#include "src/MinimaxPlayer.h"

#include <cstdio>
#include <cstdlib>

namespace {

// Joue une partie complete. `aIsX` indique si le moteur A joue les X.
// Retourne +1 si A gagne, -1 si B gagne, 0 si nulle.
int playGame(MinimaxPlayer& a, MinimaxPlayer& b, bool aIsX, const Move& opening) {
    GameState st;
    st.applyMove(opening);

    for (int ply = 0; ply < 81 && !st.isFinished(); ++ply) {
        bool xToMove = (st.currentPlayer() == Cell::X);
        MinimaxPlayer& cur = (xToMove == aIsX) ? a : b;
        Move mv = cur.chooseMove(st);
        if (!st.applyMove(mv)) break;
    }

    Cell w = st.winner();
    if (w == Cell::EMPTY) return 0;
    bool aWon = ((w == Cell::X) == aIsX);
    return aWon ? 1 : -1;
}

} // namespace

int main(int argc, char** argv) {
    // argv: depthA depthB
    int depthA = (argc > 1) ? std::atoi(argv[1]) : 11;
    int depthB = (argc > 2) ? std::atoi(argv[2]) : 9;

    // Profondeur fixe (budget 0 = illimite => iterative deepening ne fait
    // qu'une iteration a depth_ == maxDepth_).
    MinimaxPlayer a(depthA, depthA, 0);
    MinimaxPlayer b(depthB, depthB, 0);

    int aWins = 0, bWins = 0, draws = 0;

    // Une partie par premier coup (81), pour chaque assignation de couleur.
    for (int cell = 0; cell < 81; ++cell) {
        Move opening(cell / 9, cell % 9);
        for (int side = 0; side < 2; ++side) {
            bool aIsX = (side == 0);
            int r = playGame(a, b, aIsX, opening);
            if (r > 0)      ++aWins;
            else if (r < 0) ++bWins;
            else            ++draws;
        }
    }

    int decisive = aWins + bWins;
    std::printf("A(depth=%d) vs B(depth=%d)\n", depthA, depthB);
    std::printf("A wins: %d   B wins: %d   draws: %d\n", aWins, bWins, draws);
    std::printf("A no-draw winrate: %.1f%%\n",
                decisive ? 100.0 * aWins / decisive : 0.0);
    return 0;
}
