#ifndef MCTS_PLAYER_H
#define MCTS_PLAYER_H

#include "AIPlayer.h"
#include "GameState.h"

#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

// Monte Carlo Tree Search avec PUCT (priors heuristiques) et tree reuse
// entre coups. Plus efficace que minimax sur UTTT (branching variable
// jusqu'a 81, eval heuristique difficile a calibrer).
class MCTSPlayer : public AIPlayer {
public:
    explicit MCTSPlayer(int budgetMs = 200, double cExplore = 1.4);

    Move chooseMove(const GameState& state);

private:
    int    budgetMs_;
    double cExplore_;
    std::mt19937 rng_;

    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    struct Node {
        Move           move;
        int            parent;
        std::vector<int>  children;
        std::vector<Move> untried;
        Cell           toMove;
        std::int32_t   visits;
        double         wins;
        double         prior;
        bool           terminal;
    };

    std::vector<Node> nodes_;

    // rootState_ correspond toujours au noeud racine actuel (= apres
    // notre dernier coup OU apres un reroot grandchild). Apres
    // reroot/rebuild la racine est en nodes_[0].
    GameState rootState_;
    bool      hasTree_;

    int   expand(int nodeIdx, GameState& state);
    int   select(int rootIdx, GameState& state);
    double rollout(GameState state);
    void  backprop(int leaf, double reward);
    int   bestChildIndex(int rootIdx) const;
    void  reroot(int newRoot);
    Move  findOppMove(const GameState& prev, const GameState& curr) const;
};

#endif // MCTS_PLAYER_H
