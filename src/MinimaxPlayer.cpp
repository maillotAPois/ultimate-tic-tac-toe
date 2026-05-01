#include "MinimaxPlayer.h"
#include <climits>

namespace {
    // Constantes d'evaluation. INF >> tout score heuristique pour
    // garantir que les terminaux dominent l'eval intermediaire.
    static const int INF      = 1000000;
    static const int WIN_VAL  = 100000;
    static const int SUB_VAL  = 1000;
}

MinimaxPlayer::MinimaxPlayer(int depth) : depth_(depth) {}

int MinimaxPlayer::evaluate(const GameState& state) const {
    Cell me   = state.currentPlayer();
    Cell them = opponent(me);

    Cell w = state.winner();
    if (w == me)   return WIN_VAL;
    if (w == them) return -WIN_VAL;

    // Egalite par decompte des sous-grilles si match termine
    if (state.isFinished()) {
        int diff = state.board().countWonSubBoards(me)
                 - state.board().countWonSubBoards(them);
        if (diff > 0) return WIN_VAL;
        if (diff < 0) return -WIN_VAL;
        return 0;
    }

    int score = 0;
    score += SUB_VAL * (state.board().countWonSubBoards(me)
                      - state.board().countWonSubBoards(them));
    return score;
}

Move MinimaxPlayer::chooseMove(const GameState& state) {
    std::vector<Move> moves = state.legalMoves();
    if (moves.empty()) return Move();

    Move bestMove = moves[0];
    int  bestScore = -INF;
    int  alpha = -INF;
    const int beta = INF;

    for (size_t i = 0; i < moves.size(); ++i) {
        GameState next = state;
        next.applyMove(moves[i]);
        // -negamax: on inverse car le score retourne est du point de vue
        // de l'adversaire (joueur courant apres notre coup).
        int score = -negamax(next, depth_ - 1, -beta, -alpha);
        if (score > bestScore) {
            bestScore = score;
            bestMove  = moves[i];
        }
        if (score > alpha) alpha = score;
    }
    return bestMove;
}

int MinimaxPlayer::negamax(GameState state, int depth, int alpha, int beta) const {
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
