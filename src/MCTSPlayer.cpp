#include "MCTSPlayer.h"

#include <cmath>
#include <limits>

MCTSPlayer::MCTSPlayer(int budgetMs, double cExplore)
    : budgetMs_(budgetMs)
    , cExplore_(cExplore)
    , rng_(0xC0FFEE)
    , hasTree_(false)
{}

int MCTSPlayer::expand(int nodeIdx, GameState& state) {
    Move m = nodes_[nodeIdx].untried.back();
    nodes_[nodeIdx].untried.pop_back();
    Cell mover = state.currentPlayer();
    state.applyMove(m);

    // Prior PUCT: 4.0 si capture de sous-grille, 0.3 si on envoie l'adversaire
    // dans une sub deja finie (lui donne libre choix), 1.0 sinon.
    double prior = 1.0;
    int br = m.row / 3, bc = m.col / 3;
    if (state.board().subWinner(br, bc) == mover) prior = 4.0;
    else {
        int fr = state.forcedSubRow(), fc = state.forcedSubCol();
        if (fr >= 0 && state.board().subFinished(fr, fc)) prior = 0.3;
    }

    Node child;
    child.move = m; child.parent = nodeIdx;
    child.toMove = state.currentPlayer();
    child.visits = 0; child.wins = 0.0; child.prior = prior;
    child.terminal = state.isFinished();
    if (!child.terminal) child.untried = state.legalMoves();
    int childIdx = static_cast<int>(nodes_.size());
    nodes_.push_back(std::move(child));
    nodes_[nodeIdx].children.push_back(childIdx);
    return childIdx;
}

int MCTSPlayer::select(int rootIdx, GameState& state) {
    int idx = rootIdx;
    while (true) {
        const Node& n = nodes_[idx];
        if (n.terminal) return idx;
        if (!n.untried.empty()) return expand(idx, state);
        if (n.children.empty()) return idx;

        // PUCT: argmax_c (Q(c) + c * P(c) * sqrt(N) / (1 + n_c)).
        // FPU=0.5 pour les enfants non visites.
        double sqrtN  = std::sqrt(static_cast<double>(n.visits));
        int    bestIdx = n.children[0];
        double bestVal = -std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < n.children.size(); ++i) {
            const Node& c = nodes_[n.children[i]];
            double winrate = (c.visits > 0) ? (c.wins / c.visits) : 0.5;
            double explore = cExplore_ * c.prior * sqrtN / (1.0 + c.visits);
            double v = winrate + explore;
            if (v > bestVal) { bestVal = v; bestIdx = n.children[i]; }
        }
        state.applyMove(nodes_[bestIdx].move);
        idx = bestIdx;
    }
}

double MCTSPlayer::rollout(GameState state) {
    // Politique: a chaque etape, prefere un coup qui gagne immediatement
    // la sous-grille (check 8 lignes O(1)). Sinon coup aleatoire uniforme.
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
        // Tie-breaker reglementaire: plus de sous-grilles gagnees l'emporte.
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
    int idx = leaf;
    while (idx >= 0) {
        Node& n = nodes_[idx];
        n.visits++;
        // Recompense du POV du joueur qui a JOUE le coup vers ce noeud
        // (= opponent(toMove)).
        Cell mover = n.toMove;
        double reward;
        if (rolloutEncoded == 0.5) {
            reward = 0.5;
        } else {
            Cell winner = (rolloutEncoded > 0.5) ? Cell::X : Cell::O;
            reward = (winner == opponent(mover)) ? 1.0 : 0.0;
        }
        n.wins += reward;
        idx = n.parent;
    }
}

int MCTSPlayer::bestChildIndex(int rootIdx) const {
    const Node& root = nodes_[rootIdx];
    int bestIdx    = -1;
    int bestVisits = -1;
    for (size_t i = 0; i < root.children.size(); ++i) {
        const Node& c = nodes_[root.children[i]];
        if (c.visits > bestVisits) {
            bestVisits = c.visits;
            bestIdx    = root.children[i];
        }
    }
    return bestIdx;
}

// Compaction: garde uniquement le sous-arbre de newRoot, le place en
// nodes_[0], remappe parent/children. Evite la croissance memoire monotone.
void MCTSPlayer::reroot(int newRoot) {
    std::vector<int>  remap(nodes_.size(), -1);
    std::vector<Node> compact;
    compact.reserve(nodes_.size());

    std::vector<int> stack;
    stack.push_back(newRoot);
    while (!stack.empty()) {
        int idx = stack.back();
        stack.pop_back();
        if (remap[idx] >= 0) continue;
        remap[idx] = static_cast<int>(compact.size());
        compact.push_back(nodes_[idx]);
        const std::vector<int>& ch = nodes_[idx].children;
        for (size_t k = 0; k < ch.size(); ++k) stack.push_back(ch[k]);
    }

    for (size_t i = 0; i < compact.size(); ++i) {
        Node& n = compact[i];
        if (i == 0) n.parent = -1;
        else        n.parent = remap[n.parent];
        for (size_t k = 0; k < n.children.size(); ++k) {
            n.children[k] = remap[n.children[k]];
        }
    }

    nodes_.swap(compact);
}

Move MCTSPlayer::findOppMove(const GameState& prev, const GameState& curr) const {
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            Cell pv = prev.board().cellAt(r, c);
            Cell cv = curr.board().cellAt(r, c);
            if (pv == Cell::EMPTY && cv != Cell::EMPTY) {
                return Move(r, c);
            }
        }
    }
    return Move();
}

Move MCTSPlayer::chooseMove(const GameState& state) {
    std::vector<Move> moves = state.legalMoves();
    if (moves.empty()) return Move();
    if (moves.size() == 1) {
        hasTree_ = false;
        return moves[0];
    }

    // Coup d'ouverture connu optimal: centre du centre.
    if (state.moveCount() == 0) {
        hasTree_ = false;
        return Move(4, 4);
    }

    // Raccourci tactique: gain immediat de la partie.
    Cell me = state.currentPlayer();
    for (size_t i = 0; i < moves.size(); ++i) {
        GameState next = state;
        next.applyMove(moves[i]);
        if (next.winner() == me) {
            hasTree_ = false;
            return moves[i];
        }
    }

    // Tree reuse: identifier le coup adverse et reroot sur le grandchild.
    bool reused = false;
    if (hasTree_ && !nodes_.empty()) {
        Move oppMv = findOppMove(rootState_, state);
        if (oppMv.isValid()) {
            const std::vector<int>& kids = nodes_[0].children;
            int gc = -1;
            for (size_t k = 0; k < kids.size(); ++k) {
                const Node& c = nodes_[kids[k]];
                if (c.move.row == oppMv.row && c.move.col == oppMv.col) {
                    gc = kids[k]; break;
                }
            }
            if (gc >= 0) {
                reroot(gc);
                rootState_ = state;
                reused = true;
            }
        }
    }
    if (!reused) {
        nodes_.clear();
        nodes_.reserve(50000);
        Node root;
        root.move     = Move();
        root.parent   = -1;
        root.toMove   = state.currentPlayer();
        root.visits   = 0;
        root.wins     = 0.0;
        root.prior    = 1.0;
        root.terminal = state.isFinished();
        root.untried  = moves;
        nodes_.push_back(root);
        rootState_ = state;
        hasTree_   = true;
    }

    TimePoint deadline = Clock::now() + std::chrono::milliseconds(budgetMs_);
    while (Clock::now() < deadline) {
        for (int b = 0; b < 64 && Clock::now() < deadline; ++b) {
            GameState s = rootState_;
            int leaf = select(0, s);
            double encoded = rollout(s);
            backprop(leaf, encoded);
        }
    }

    int bestIdx = bestChildIndex(0);
    if (bestIdx < 0) {
        hasTree_ = false;
        return moves[0];
    }
    Move chosen = nodes_[bestIdx].move;
    reroot(bestIdx);
    rootState_.applyMove(chosen);
    return chosen;
}
