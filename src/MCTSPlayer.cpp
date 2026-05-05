#include "MCTSPlayer.h"

#include <algorithm>
#include <cmath>
#include <limits>

MCTSPlayer::MCTSPlayer(int budgetMs, double cExplore)
    : budgetMs_(budgetMs)
    , cExplore_(cExplore)
    , rng_(0xC0FFEE) // graine fixe -> reproductible
{}

namespace {
    // Convertit un resultat de partie (winner) en recompense pour le
    // joueur qui DEVAIT jouer dans le noeud (= celui qui a effectue le
    // coup vers ce noeud, du POV du parent).
    //
    // mover: le joueur qui DOIT jouer dans le noeud cible (= adversaire
    // de celui qui a fait le coup pour y arriver).
    // winner: gagnant du rollout, ou EMPTY pour nul.
    inline double rewardFor(Cell mover, Cell winner) {
        if (winner == Cell::EMPTY)         return 0.5;
        // Le coup vers ce noeud a ete joue par opponent(mover).
        // On veut la recompense du POV du JOUEUR du coup parent.
        if (winner == opponent(mover)) return 1.0;
        return 0.0;
    }
}

int MCTSPlayer::expand(int nodeIdx, GameState& state) {
    Node& n = nodes_[nodeIdx];
    Move m = n.untried.back();
    n.untried.pop_back();

    state.applyMove(m);

    Node child;
    child.move     = m;
    child.parent   = nodeIdx;
    child.toMove   = state.currentPlayer();
    child.visits   = 0;
    child.wins     = 0.0;
    child.terminal = state.isFinished();
    if (!child.terminal) {
        child.untried = state.legalMoves();
    }
    int childIdx = static_cast<int>(nodes_.size());
    nodes_.push_back(child);
    nodes_[nodeIdx].children.push_back(childIdx);
    return childIdx;
}

int MCTSPlayer::select(int rootIdx, GameState& state) {
    int idx = rootIdx;
    while (true) {
        const Node& n = nodes_[idx];
        if (n.terminal) return idx;
        if (!n.untried.empty()) {
            // Expansion immediate: on cree un nouvel enfant et on le
            // retourne comme noeud a simuler.
            return expand(idx, state);
        }
        if (n.children.empty()) return idx; // securite

        // UCT: argmax_c (winrate + c * sqrt(ln(N) / n_c)).
        double logN = std::log(static_cast<double>(n.visits));
        int    bestIdx = n.children[0];
        double bestVal = -std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < n.children.size(); ++i) {
            const Node& c = nodes_[n.children[i]];
            double winrate = (c.visits > 0) ? (c.wins / c.visits) : 0.0;
            double explore = cExplore_ * std::sqrt(logN / std::max(1, c.visits));
            double v = winrate + explore;
            if (v > bestVal) { bestVal = v; bestIdx = n.children[i]; }
        }
        state.applyMove(nodes_[bestIdx].move);
        idx = bestIdx;
    }
}

double MCTSPlayer::rollout(GameState state) {
    // Rollout heuristique rapide: a chaque etape on cherche un coup qui
    // gagne immediatement la sous-grille via un check O(8 lignes) sans
    // copie. Sinon coup aleatoire. Reduit le bruit sans payer le cout
    // du Board::winner() complet sur chaque candidat.
    //
    // Lignes 3x3 (indices locaux) — 3 horizontales, 3 verticales, 2 diagonales.
    static const int LINES[8][3][2] = {
        {{0,0},{0,1},{0,2}}, {{1,0},{1,1},{1,2}}, {{2,0},{2,1},{2,2}},
        {{0,0},{1,0},{2,0}}, {{0,1},{1,1},{2,1}}, {{0,2},{1,2},{2,2}},
        {{0,0},{1,1},{2,2}}, {{0,2},{1,1},{2,0}},
    };

    while (!state.isFinished()) {
        std::vector<Move> ms = state.legalMoves();
        if (ms.empty()) break;

        Cell turn    = state.currentPlayer();
        int  pickIdx = -1;
        for (size_t i = 0; i < ms.size(); ++i) {
            int br = ms[i].row / 3, bc = ms[i].col / 3;
            int lr = ms[i].row % 3, lc = ms[i].col % 3;
            const Board& sub = state.board().sub(br, bc);
            // Check inline: existe-t-il une ligne contenant (lr,lc) ou
            // les 2 autres cases ont deja `turn`? Si oui, jouer ce coup
            // gagne la sous-grille.
            bool wins = false;
            for (int li = 0; li < 8 && !wins; ++li) {
                bool inLine = false;
                int  cnt = 0;
                for (int k = 0; k < 3; ++k) {
                    int r = LINES[li][k][0], c = LINES[li][k][1];
                    if (r == lr && c == lc) { inLine = true; }
                    else if (sub.get(r, c) == turn) ++cnt;
                }
                if (inLine && cnt == 2) wins = true;
            }
            if (wins) { pickIdx = static_cast<int>(i); break; }
        }
        if (pickIdx < 0) {
            std::uniform_int_distribution<size_t> dist(0, ms.size() - 1);
            pickIdx = static_cast<int>(dist(rng_));
        }
        state.applyMove(ms[pickIdx]);
    }

    Cell winner = state.winner();
    if (winner == Cell::EMPTY) {
        // Match nul reel: le framework departage aux sous-grilles
        // gagnees. On reproduit la regle pour que la recompense reflete
        // le verdict reel.
        int xCount = state.board().countWonSubBoards(Cell::X);
        int oCount = state.board().countWonSubBoards(Cell::O);
        if (xCount > oCount)      winner = Cell::X;
        else if (oCount > xCount) winner = Cell::O;
    }
    if (winner == Cell::X) return 1.0;
    if (winner == Cell::O) return 0.0;
    return 0.5;
}

void MCTSPlayer::backprop(int leaf, double rolloutEncoded) {
    // rolloutEncoded: 1.0 X gagne, 0.0 O gagne, 0.5 nul.
    int idx = leaf;
    while (idx >= 0) {
        Node& n = nodes_[idx];
        n.visits++;
        // Recompense du POV du joueur qui a JOUE le coup vers ce noeud
        // (= opposite de toMove). Pour la racine (parent==-1), pas
        // utilise mais la maj est neutre.
        Cell mover = n.toMove; // qui doit jouer ici
        double reward;
        if (rolloutEncoded == 0.5) {
            reward = 0.5;
        } else {
            Cell winner = (rolloutEncoded > 0.5) ? Cell::X : Cell::O;
            // Le coup parent->n a ete fait par opponent(mover).
            reward = (winner == opponent(mover)) ? 1.0 : 0.0;
        }
        n.wins += reward;
        idx = n.parent;
    }
}

Move MCTSPlayer::bestChildMove(int rootIdx) const {
    const Node& root = nodes_[rootIdx];
    int    bestIdx   = -1;
    int    bestVisits = -1;
    for (size_t i = 0; i < root.children.size(); ++i) {
        const Node& c = nodes_[root.children[i]];
        if (c.visits > bestVisits) {
            bestVisits = c.visits;
            bestIdx    = root.children[i];
        }
    }
    if (bestIdx < 0) return Move();
    return nodes_[bestIdx].move;
}

Move MCTSPlayer::chooseMove(const GameState& state) {
    std::vector<Move> moves = state.legalMoves();
    if (moves.empty())     return Move();
    if (moves.size() == 1) return moves[0];

    // Coup d'ouverture connu: centre du centre.
    if (state.moveCount() == 0) return Move(4, 4);

    // Raccourci tactique: gain immediat.
    Cell me = state.currentPlayer();
    for (size_t i = 0; i < moves.size(); ++i) {
        GameState next = state;
        next.applyMove(moves[i]);
        if (next.winner() == me) return moves[i];
    }

    // Initialise l'arbre avec la position actuelle comme racine.
    nodes_.clear();
    nodes_.reserve(50000);
    Node root;
    root.move     = Move();
    root.parent   = -1;
    root.toMove   = state.currentPlayer();
    root.visits   = 0;
    root.wins     = 0.0;
    root.terminal = state.isFinished();
    root.untried  = moves;
    nodes_.push_back(root);

    auto deadline = Clock::now() + std::chrono::milliseconds(budgetMs_);
    int  iters = 0;
    while (Clock::now() < deadline) {
        // Verification temps tous les 64 iters pour limiter l'overhead.
        for (int b = 0; b < 64 && Clock::now() < deadline; ++b) {
            GameState s = state;
            int leaf = select(0, s);
            double encoded = rollout(s);
            backprop(leaf, encoded);
            ++iters;
        }
    }
    (void)iters;

    return bestChildMove(0);
}
