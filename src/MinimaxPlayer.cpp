#include "MinimaxPlayer.h"
#include "Evaluator.h"
#include <chrono>

namespace {
    // INF >> tout score heuristique pour que les terminaux dominent.
    static const int INF = 1000000;

    // Sentinelle levee quand le budget temps de l'iterative deepening
    // est epuise. La recherche en cours est abandonnee et l'on garde
    // le meilleur coup trouve a la profondeur precedente.
    struct TimeOut {};

    using Clock = std::chrono::steady_clock;
    static Clock::time_point g_deadline;
    static bool              g_useDeadline = false;

    inline void checkTime() {
        if (g_useDeadline && Clock::now() > g_deadline) throw TimeOut();
    }
}

MinimaxPlayer::MinimaxPlayer(int depth)
    : depth_(depth), useID_(false), timeBudgetMs_(150.0), maxDepth_(12)
{}

void MinimaxPlayer::enableIterativeDeepening(double timeBudgetMs, int maxDepth) {
    useID_        = true;
    timeBudgetMs_ = timeBudgetMs;
    maxDepth_     = maxDepth;
}

int MinimaxPlayer::evaluate(const GameState& state) const {
    return Evaluator::evaluate(state);
}

Move MinimaxPlayer::chooseMove(const GameState& state) {
    std::vector<Move> moves = state.legalMoves();
    if (moves.empty()) return Move();

    // Cas degeneres et coup d'ouverture
    if (moves.size() == 1)         return moves[0];
    if (state.moveCount() == 0)    return Move(4, 4);

    if (!useID_) {
        return searchRoot(state, depth_);
    }

    // Iterative deepening: on commence court et on approfondit
    // tant que le budget temps le permet.
    Move best = moves[0];
    g_deadline = Clock::now()
               + std::chrono::milliseconds(static_cast<long long>(timeBudgetMs_));
    g_useDeadline = true;

    try {
        for (int d = 2; d <= maxDepth_; ++d) {
            Move iterBest = searchRoot(state, d);
            best = iterBest;  // recherche a profondeur d completee
            // On verifie le temps avant de relancer une profondeur plus
            // grande qui couterait nettement plus cher.
            if (Clock::now() > g_deadline) break;
        }
    } catch (const TimeOut&) {
        // On garde `best`, calcule a la profondeur precedente.
    }

    g_useDeadline = false;
    return best;
}

Move MinimaxPlayer::searchRoot(const GameState& state, int depth) const {
    std::vector<Move> moves = state.legalMoves();
    Move bestMove = moves[0];
    int  bestScore = -INF;
    int  alpha = -INF;
    const int beta = INF;

    for (size_t i = 0; i < moves.size(); ++i) {
        GameState next = state;
        next.applyMove(moves[i]);
        int score = -negamax(next, depth - 1, -beta, -alpha);
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
    checkTime();

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
