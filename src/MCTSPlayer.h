#ifndef MCTS_PLAYER_H
#define MCTS_PLAYER_H

#include "AIPlayer.h"
#include "GameState.h"

#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

// Joueur Monte Carlo Tree Search avec UCT (Upper Confidence Bounds for
// Trees). Plus adapte que minimax sur UTTT car:
//  - le facteur de branchement est tres variable (1..81),
//  - la fonction d'evaluation heuristique est difficile a calibrer,
//  - les rollouts aleatoires capturent naturellement l'espace tactique.
//
// Boucle: pendant le budget temps imparti par coup, on repete:
//   1) Selection: descendre l'arbre via UCT jusqu'a un noeud non
//      completement etendu ou terminal.
//   2) Expansion: ajouter un enfant pour un coup non encore joue.
//   3) Simulation: rollout aleatoire jusqu'a un etat terminal.
//   4) Retropropagation: propager le resultat (win/draw/loss) vers la
//      racine.
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

    // Noeud de l'arbre MCTS. Stocke le coup qui a mene a cet etat depuis
    // le parent (ou Move() invalide pour la racine), les statistiques
    // de visites, le joueur qui doit jouer dans cet etat, et les
    // enfants/coups non encore explores.
    struct Node {
        Move           move;
        int            parent;
        std::vector<int>  children;
        std::vector<Move> untried;
        Cell           toMove;
        std::int32_t   visits;
        double         wins;       // wins du POV du joueur qui DOIT
                                   // jouer dans le PARENT (= celui qui
                                   // a fait le coup vers ici).
        double         prior;      // PUCT prior (1.0 neutre, >1 favorise,
                                   // <1 defavorise). Calcule au moment
                                   // de l'expansion en regardant le
                                   // resultat statique du coup.
        bool           terminal;
    };

    std::vector<Node> nodes_;

    // Tree reuse: on garde l'arbre entre deux appels a chooseMove.
    // - rootState_  : etat correspondant au noeud racine actuel (=
    //   apres notre dernier coup, ou apres reroot grandchild).
    // - hasTree_    : false au tout premier appel ou si le re-root
    //   echoue (coup adverse non explore par MCTS au tour precedent).
    // Apres reroot/rebuild, la racine est toujours nodes_[0].
    GameState rootState_;
    bool      hasTree_;
    long long mctsIters_;     // [BENCH] iterations du dernier appel

    int   expand(int nodeIdx, GameState& state);
    int   select(int rootIdx, GameState& state);
    double rollout(GameState state);
    void  backprop(int leaf, double reward);
    int   bestChildIndex(int rootIdx) const;

    // Compaction de l'arbre: garde uniquement le sous-arbre enracine
    // en `newRoot`, le replace en index 0, et remappe parent/children.
    void  reroot(int newRoot);
    // Retrouve le coup adverse en comparant deux etats (cellule
    // devenue occupee). Retourne un Move invalide si pas trouve.
    Move  findOppMove(const GameState& prev, const GameState& curr) const;
};

#endif // MCTS_PLAYER_H
