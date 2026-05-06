#ifndef MINIMAX_PLAYER_H
#define MINIMAX_PLAYER_H

#include "AIPlayer.h"

#include <chrono>
#include <cstdint>
#include <unordered_map>

// Joueur base sur l'algorithme negamax (variante symetrique du minimax)
// avec elagage alpha-beta, iterative deepening et table de transposition
// indexee par hash Zobrist.
//
// Iterative deepening: on fait une recherche complete a profondeur 1, puis
// 2, etc. en gardant le meilleur coup tant qu'il reste du budget temps.
//
// Transposition table: cache (key -> {depth, value, flag, bestMove}). Une
// position re-rencontree par transposition peut etre coupee tot si on l'a
// deja recherchee a une profondeur suffisante. La best move retenue
// alimente le move ordering, ce qui amplifie les coupures alpha-beta.
class MinimaxPlayer : public AIPlayer {
public:
    explicit MinimaxPlayer(int depth = 4, int maxDepth = 8, int budgetMs = 90);

    Move chooseMove(const GameState& state);

private:
    int depth_;
    int maxDepth_;
    int budgetMs_;

    using Clock      = std::chrono::steady_clock;
    using TimePoint  = Clock::time_point;

    struct SearchCtx {
        TimePoint deadline;
        bool      stop;
        bool      timed;
    };

    enum TTFlag : char { TT_EXACT = 0, TT_LOWER = 1, TT_UPPER = 2 };
    struct TTEntry {
        std::int32_t score;
        std::int16_t depth;
        TTFlag       flag;
        Move         best;
    };
    mutable std::unordered_map<std::uint64_t, TTEntry> tt_;

    // Killer moves: pour chaque ply (distance depuis la racine), on retient
    // les 2 derniers coups qui ont produit une coupure beta. Ces coups
    // (sans capture, pas dans la TT) sont essayes en priorite dans des
    // positions analogues -> meilleur move ordering -> plus de coupures.
    static constexpr int KILLERS_MAX_PLY = 32;
    Move killers_[KILLERS_MAX_PLY][2];

    void recordKiller(int ply, const Move& m);
    bool isKiller(int ply, const Move& m) const;

    // Instrumentation (benchmark only): comptes par appel a chooseMove,
    // logges sur stderr. Aucun impact fonctionnel sur le choix du coup.
    std::uint64_t nodes_;
    std::uint64_t qnodes_;
    std::uint64_t tt_lookups_;
    std::uint64_t tt_hits_;
    int           iter_depth_reached_;

    std::uint64_t hashState(const GameState& state) const;

    int  negamax(GameState& state, int depth, int ply, int alpha, int beta, SearchCtx& ctx);
    int  quiescence(GameState& state, int alpha, int beta, int qdepth, SearchCtx& ctx);
    int  evaluate(const GameState& state) const;
    bool timedOut(SearchCtx& ctx) const;
};

#endif // MINIMAX_PLAYER_H
