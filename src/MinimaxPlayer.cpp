#include "MinimaxPlayer.h"
#include "Evaluator.h"
#include <algorithm>
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

    // Pre-tri rapide pour orienter la 1re passe.
    std::vector<ScoredMove> scored;
    scored.reserve(moves.size());
    for (size_t i = 0; i < moves.size(); ++i) {
        ScoredMove sm;
        sm.move  = moves[i];
        sm.score = quickScore(state, moves[i]);
        scored.push_back(sm);
    }
    std::sort(scored.begin(), scored.end(),
              [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    if (!useID_) {
        return searchRoot(state, depth_, scored);
    }

    // Iterative deepening: on commence court et on approfondit
    // tant que le budget temps le permet.
    Move best = scored[0].move;
    g_deadline = Clock::now()
               + std::chrono::milliseconds(static_cast<long long>(timeBudgetMs_));
    g_useDeadline = true;

    try {
        for (int d = 2; d <= maxDepth_; ++d) {
            Move iterBest = searchRoot(state, d, scored);
            best = iterBest;  // recherche a profondeur d completee
            if (Clock::now() > g_deadline) break;
        }
    } catch (const TimeOut&) {
        // On garde `best`, calcule a la profondeur precedente.
    }

    g_useDeadline = false;
    return best;
}

int MinimaxPlayer::quickScore(const GameState& state, const Move& m) {
    int score = 0;
    int br = m.row / 3, bc = m.col / 3;
    int lr = m.row % 3, lc = m.col % 3;

    // Bonus si le coup gagne directement la sous-grille (tres prioritaire)
    Board sim = state.board().sub(br, bc);
    sim.set(lr, lc, state.currentPlayer());
    if (sim.winner() == state.currentPlayer()) score += 500;

    // Centre du plateau global: position de prestige
    if (m.row == 4 && m.col == 4) score += 80;
    // Centre d'une sous-grille
    if (lr == 1 && lc == 1) score += 30;
    // Coins d'une sous-grille
    if ((lr == 0 || lr == 2) && (lc == 0 || lc == 2)) score += 10;

    // Penalite si le coup envoie l'adversaire vers une sous-grille libre
    // (sous-grille deja finie => libre choice). Mauvais en general.
    int nbr = lr, nbc = lc;
    if (state.board().subFinished(nbr, nbc)) score -= 60;

    return score;
}

Move MinimaxPlayer::searchRoot(const GameState& state, int depth,
                               std::vector<ScoredMove>& moves) const {
    Move bestMove = moves[0].move;
    int  bestScore = -INF;
    int  alpha = -INF;
    const int beta = INF;

    for (size_t i = 0; i < moves.size(); ++i) {
        GameState next = state;
        next.applyMove(moves[i].move);
        int score = -negamax(next, depth - 1, -beta, -alpha);
        moves[i].score = score;
        if (score > bestScore) {
            bestScore = score;
            bestMove  = moves[i].move;
        }
        if (score > alpha) alpha = score;
    }
    // Tri pour la prochaine iteration de l'iterative deepening:
    // le coup le plus prometteur sera explore en premier ce qui
    // ameliore drastiquement les coupures alpha-beta.
    std::sort(moves.begin(), moves.end(),
              [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });
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
