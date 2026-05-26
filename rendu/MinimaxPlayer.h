#ifndef MINIMAX_PLAYER_H
#define MINIMAX_PLAYER_H

#include "AIPlayer.h"

#include <chrono>
#include <cstdint>
#include <vector>

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
        std::uint64_t key   = 0;   // 0 = creneau vide
        std::int32_t  score = 0;
        std::int16_t  depth = 0;
        TTFlag        flag  = TT_EXACT;
        Move          best;
    };
    // Table de transposition en tableau fixe direct-mapped (index =
    // hash & TT_MASK). Bien plus rapide qu'un unordered_map: pas
    // d'allocation ni de chainage par noeud. La cle complete est stockee
    // pour distinguer les collisions d'index.
    static constexpr std::size_t   TT_SIZE = std::size_t(1) << 22; // 4M
    static constexpr std::uint64_t TT_MASK = TT_SIZE - 1;
    mutable std::vector<TTEntry> tt_;

    // Killer moves: pour chaque ply (distance depuis la racine), on retient
    // les 2 derniers coups qui ont produit une coupure beta. Ces coups
    // (sans capture, pas dans la TT) sont essayes en priorite dans des
    // positions analogues -> meilleur move ordering -> plus de coupures.
    static constexpr int KILLERS_MAX_PLY = 32;
    Move killers_[KILLERS_MAX_PLY][2];

    // Heuristique d'historique: pour chaque (cote au trait, case cible
    // 0..80), score cumule des coupures beta produites par ce coup. Un
    // coup qui coupe souvent ailleurs est essaye plus tot -> meilleur
    // move ordering -> recherche plus profonde a budget egal.
    int history_[2][81];

    // Buffers de coups pre-alloues, un par profondeur de recherche.
    // Evite l'allocation d'un std::vector a chaque appel de negamax/
    // quiescence (chemin chaud).
    static constexpr int MOVE_BUFS_MAX = 64;
    std::vector<std::vector<Move>> moveBufs_;

    void recordKiller(int ply, const Move& m);
    bool isKiller(int ply, const Move& m) const;

    std::uint64_t hashState(const GameState& state) const;

    int  negamax(GameState& state, int depth, int ply, int alpha, int beta, SearchCtx& ctx);
    int  quiescence(GameState& state, int alpha, int beta, int qdepth, SearchCtx& ctx);
    int  evaluate(const GameState& state) const;
    bool timedOut(SearchCtx& ctx) const;
};

#endif // MINIMAX_PLAYER_H
