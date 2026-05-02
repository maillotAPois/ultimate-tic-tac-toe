#include "MinimaxPlayer.h"
#include "Evaluator.h"

namespace {
    // INF >> tout score heuristique pour que les terminaux dominent.
    static const int INF = 1000000;
}

MinimaxPlayer::MinimaxPlayer(int depth) : depth_(depth) {}

int MinimaxPlayer::evaluate(const GameState& state) const {
    return Evaluator::evaluate(state);
}

Move MinimaxPlayer::chooseMove(const GameState& state) {
    std::vector<Move> moves = state.legalMoves();
    if (moves.empty()) return Move();

    // Cas degenere: une seule option, pas besoin de recherche.
    if (moves.size() == 1) return moves[0];

    // Premier coup de la partie: 81 possibilites. La recherche serait
    // tres lente et le centre de l'echiquier est connu pour etre fort.
    if (state.moveCount() == 0) return Move(4, 4);

    // Profondeur effective: on approfondit quand le branching factor
    // diminue (fin de partie). Permet de garder des temps raisonnables
    // tout en ayant une meilleure vision en endgame.
    int effectiveDepth = depth_;
    if (state.moveCount() > 30) effectiveDepth = depth_ + 1;
    if (state.moveCount() > 50) effectiveDepth = depth_ + 2;

    Move bestMove = moves[0];
    int  bestScore = -INF;
    int  alpha = -INF;
    const int beta = INF;

    for (size_t i = 0; i < moves.size(); ++i) {
        GameState next = state;
        next.applyMove(moves[i]);
        // -negamax: on inverse car le score retourne est du point de vue
        // de l'adversaire (joueur courant apres notre coup).
        int score = -negamax(next, effectiveDepth - 1, -beta, -alpha);
        if (score > bestScore) {
            bestScore = score;
            bestMove  = moves[i];
        }
        if (score > alpha) alpha = score;
    }
    return bestMove;
}

int MinimaxPlayer::negamax(GameState state, int depth, int alpha, int beta) const {
    // Negamax = formulation symetrique du minimax: on multiplie la valeur
    // par -1 a chaque appel recursif ce qui evite de distinguer les
    // niveaux MIN et MAX. L'evaluation est donc toujours faite du point
    // de vue du joueur courant a la feuille (cf. evaluate).
    if (depth == 0 || state.isFinished()) {
        return evaluate(state);
    }

    std::vector<Move> moves = state.legalMoves();
    if (moves.empty()) return evaluate(state);

    int best = -INF;
    for (size_t i = 0; i < moves.size(); ++i) {
        GameState next = state;
        next.applyMove(moves[i]);
        int score = -negamax(next, depth - 1, -beta, -alpha);
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;  // coupure beta
    }
    return best;
}
