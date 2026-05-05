#include "MinimaxPlayer.h"

#include <algorithm>
#include <array>

namespace {
    // INF >> tout score heuristique pour que les terminaux dominent.
    constexpr int INF             = 1000000;
    constexpr int WIN_VAL         = 100000;

    // Win-distance: on attenue legerement WIN_VAL avec la profondeur de
    // recherche pour preferer les victoires rapides et reculer les
    // defaites. Sans ca le minimax est indifferent entre "perdre dans 1
    // coup" et "perdre dans 5", ce qui produit des renoncements precoces.
    constexpr int WIN_DEPTH_PENALTY = 100;

    // Poids positionnels (TTT classique): centre > coins > cotes.
    constexpr int POS_W[3][3] = {
        {3, 2, 3},
        {2, 4, 2},
        {3, 2, 3},
    };

    constexpr int SUB_VAL         = 200;   // sous-grille gagnee, * poids positionnel
    constexpr int META_THREAT_VAL = 500;   // 2-en-ligne au niveau meta
    constexpr int LOCAL_THREAT    = 2;     // 2-en-ligne dans sous-grille * POS
    constexpr int FREE_CHOICE_VAL = 30;    // bonus si on peut jouer ou on veut

    struct Scored {
        Move move;
        int  score;
    };

    // Table Zobrist: 81 cases x 2 joueurs + 9 sous-grilles cibles + 1 bit
    // de joueur courant. Generee de maniere deterministe (xorshift) pour
    // que le binaire reste reproductible.
    struct ZobristTable {
        std::uint64_t cells[81][2];
        std::uint64_t forced[10]; // 0..8 + 9 = libre
        std::uint64_t side;
        ZobristTable() {
            std::uint64_t s = 0x9E3779B97F4A7C15ULL; // graine
            auto next = [&]() {
                s ^= s << 13;
                s ^= s >> 7;
                s ^= s << 17;
                return s;
            };
            for (int i = 0; i < 81; ++i) {
                cells[i][0] = next();
                cells[i][1] = next();
            }
            for (int i = 0; i < 10; ++i) forced[i] = next();
            side = next();
        }
    };
    const ZobristTable kZob;
}

MinimaxPlayer::MinimaxPlayer(int depth, int maxDepth, int budgetMs)
    : depth_(depth)
    , maxDepth_(maxDepth)
    , budgetMs_(budgetMs)
{
    for (int i = 0; i < KILLERS_MAX_PLY; ++i) {
        killers_[i][0] = Move();
        killers_[i][1] = Move();
    }
}

void MinimaxPlayer::recordKiller(int ply, const Move& m) {
    if (ply < 0 || ply >= KILLERS_MAX_PLY) return;
    // Si le coup est deja killer1, ne rien faire. Sinon decaler.
    if (killers_[ply][0].row == m.row && killers_[ply][0].col == m.col) return;
    killers_[ply][1] = killers_[ply][0];
    killers_[ply][0] = m;
}

bool MinimaxPlayer::isKiller(int ply, const Move& m) const {
    if (ply < 0 || ply >= KILLERS_MAX_PLY) return false;
    if (killers_[ply][0].row == m.row && killers_[ply][0].col == m.col) return true;
    if (killers_[ply][1].row == m.row && killers_[ply][1].col == m.col) return true;
    return false;
}

std::uint64_t MinimaxPlayer::hashState(const GameState& state) const {
    std::uint64_t h = 0;
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            Cell v = state.board().cellAt(r, c);
            if (v == Cell::EMPTY) continue;
            int idx = r * 9 + c;
            int who = (v == Cell::X) ? 0 : 1;
            h ^= kZob.cells[idx][who];
        }
    }
    int f = 9; // libre par defaut
    if (state.forcedSubRow() >= 0) {
        f = state.forcedSubRow() * 3 + state.forcedSubCol();
    }
    h ^= kZob.forced[f];
    if (state.currentPlayer() == Cell::O) h ^= kZob.side;
    return h;
}

namespace {
    // Score d'une ligne de 3 cases du point de vue d'un joueur, en
    // fonction du nombre de symboles (player, adversaire) presents:
    //   (0,0): 0          ligne vide -> aucun signal
    //   (1,0): +1         debut d'alignement
    //   (2,0): +10        2-en-ligne, prochain coup gagne
    //   (0,1): -1
    //   (0,2): -10
    //   melange: 0        ligne morte
    inline int lineScore(int p, int o) {
        if (p == 0 && o == 0) return 0;
        if (o == 0) {
            if (p == 1) return 1;
            if (p == 2) return 10;
            return 50; // 3-en-ligne: gagne (cas terminal en theorie)
        }
        if (p == 0) {
            if (o == 1) return -1;
            if (o == 2) return -10;
            return -50;
        }
        return 0;
    }

    // Score d'une sous-grille pour un joueur: somme de lineScore sur
    // les 8 lignes possibles, ponderee par les positions des cases.
    int evalSubBoard(const Board& b, Cell me) {
        if (b.winner() == me)            return 100;
        if (b.winner() == opponent(me))  return -100;
        if (b.isFull())                  return 0;

        Cell them = opponent(me);
        int score = 0;
        // 8 lignes: 3 lignes, 3 colonnes, 2 diagonales
        int lines[8][3][2] = {
            {{0,0},{0,1},{0,2}},
            {{1,0},{1,1},{1,2}},
            {{2,0},{2,1},{2,2}},
            {{0,0},{1,0},{2,0}},
            {{0,1},{1,1},{2,1}},
            {{0,2},{1,2},{2,2}},
            {{0,0},{1,1},{2,2}},
            {{0,2},{1,1},{2,0}},
        };
        for (int li = 0; li < 8; ++li) {
            int p = 0, o = 0;
            for (int k = 0; k < 3; ++k) {
                Cell c = b.get(lines[li][k][0], lines[li][k][1]);
                if (c == me)        ++p;
                else if (c == them) ++o;
            }
            score += lineScore(p, o);
        }
        return score;
    }
}

int MinimaxPlayer::evaluate(const GameState& state) const {
    Cell me   = state.currentPlayer();
    Cell them = opponent(me);

    Cell w = state.winner();
    if (w == me)   return  WIN_VAL;
    if (w == them) return -WIN_VAL;

    if (state.isFinished()) {
        int diff = state.board().countWonSubBoards(me)
                 - state.board().countWonSubBoards(them);
        if (diff > 0) return  WIN_VAL;
        if (diff < 0) return -WIN_VAL;
        return 0;
    }

    int score = 0;

    // 1) Score positionnel par sous-grille, pondere par l'importance de
    //    la sous-grille dans la meta. La fonction evalSubBoard donne
    //    un score riche (-100..+100) qui capture le degre de controle,
    //    pas seulement le binaire gagnee/non.
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            score += POS_W[br][bc] * evalSubBoard(state.board().sub(br, bc), me);
        }
    }

    // 2) Score de la meta-grille (sous-grilles gagnees), ponderation
    //    forte car decisive pour la victoire globale.
    Board metaView = state.board().metaView();
    int lines[8][3][2] = {
        {{0,0},{0,1},{0,2}},
        {{1,0},{1,1},{1,2}},
        {{2,0},{2,1},{2,2}},
        {{0,0},{1,0},{2,0}},
        {{0,1},{1,1},{2,1}},
        {{0,2},{1,2},{2,2}},
        {{0,0},{1,1},{2,2}},
        {{0,2},{1,1},{2,0}},
    };
    for (int li = 0; li < 8; ++li) {
        int p = 0, o = 0;
        for (int k = 0; k < 3; ++k) {
            Cell c = metaView.get(lines[li][k][0], lines[li][k][1]);
            if (c == me)        ++p;
            else if (c == them) ++o;
        }
        // Coefficient meta = 400x: une 2-en-ligne meta vaut ~ une sous-grille
        // gagnee au centre, ce qui reflete son poids strategique reel
        // (deux sous-grilles capturees alignees ~= 1 coup du gain global).
        score += 400 * lineScore(p, o);
    }

    if (state.forcedSubRow() < 0) {
        score += FREE_CHOICE_VAL;
    }

    return score;
}

bool MinimaxPlayer::timedOut(SearchCtx& ctx) const {
    if (!ctx.timed) return false;
    if (ctx.stop) return true;
    if (Clock::now() >= ctx.deadline) {
        ctx.stop = true;
        return true;
    }
    return false;
}

Move MinimaxPlayer::chooseMove(const GameState& constState) {
    // Local mutable copy: la recherche en apply/undo modifie l'etat
    // mais le restaure a chaque retour. Une seule copie a la racine.
    GameState state = constState;
    std::vector<Move> moves = state.legalMoves();
    if (moves.empty())     return Move();
    if (moves.size() == 1) return moves[0];

    // Coup d'ouverture: centre du centre.
    if (state.moveCount() == 0) {
        return Move(4, 4);
    }

    // Raccourci tactique: gain immediat sans recherche.
    Cell me = state.currentPlayer();
    for (size_t i = 0; i < moves.size(); ++i) {
        int pf_r = state.forcedSubRow(), pf_c = state.forcedSubCol();
        state.applyMove(moves[i]);
        bool win = (state.winner() == me);
        state.undoMove(moves[i], pf_r, pf_c);
        if (win) return moves[i];
    }

    tt_.clear();
    // Reset killers entre coups (positions changent, killers peuvent etre
    // obsoletes meme s'ils aident au sein d'un meme arbre de recherche).
    for (int i = 0; i < KILLERS_MAX_PLY; ++i) {
        killers_[i][0] = Move();
        killers_[i][1] = Move();
    }

    // Move ordering racine via apply/undo.
    std::vector<Scored> ordered;
    ordered.reserve(moves.size());
    for (size_t i = 0; i < moves.size(); ++i) {
        int pf_r = state.forcedSubRow(), pf_c = state.forcedSubCol();
        state.applyMove(moves[i]);
        int sc = -evaluate(state);
        state.undoMove(moves[i], pf_r, pf_c);
        ordered.push_back({moves[i], sc});
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const Scored& a, const Scored& b) { return a.score > b.score; });

    SearchCtx ctx;
    ctx.timed = (budgetMs_ > 0);
    ctx.stop  = false;
    if (ctx.timed) {
        ctx.deadline = Clock::now() + std::chrono::milliseconds(budgetMs_);
    }

    Move bestOverall = ordered[0].move;

    for (int d = depth_; d <= maxDepth_; ++d) {
        Move bestThisIter  = ordered[0].move;
        int  bestScore     = -INF;
        int  alpha         = -INF;
        const int beta     = INF;
        bool aborted       = false;

        for (size_t i = 0; i < ordered.size(); ++i) {
            int pf_r = state.forcedSubRow(), pf_c = state.forcedSubCol();
            state.applyMove(ordered[i].move);
            int score = -negamax(state, d - 1, /*ply=*/1, -beta, -alpha, ctx);
            state.undoMove(ordered[i].move, pf_r, pf_c);
            if (ctx.stop) { aborted = true; break; }
            if (score > bestScore) {
                bestScore = score;
                bestThisIter = ordered[i].move;
            }
            if (score > alpha) alpha = score;
        }

        if (!aborted) {
            bestOverall = bestThisIter;
            // Mat trouve: inutile d'aller plus profond.
            if (bestScore >= WIN_VAL - 1000) break;
            if (bestScore <= -WIN_VAL + 1000) break;

            // Re-ordonnement: la PV trouvee doit etre testee en premier
            // a la prochaine iteration. On re-trie par eval pour les autres.
            for (size_t i = 0; i < ordered.size(); ++i) {
                if (ordered[i].move.row == bestThisIter.row &&
                    ordered[i].move.col == bestThisIter.col) {
                    if (i > 0) std::swap(ordered[0], ordered[i]);
                    break;
                }
            }
        }
        if (ctx.stop) break;
    }

    return bestOverall;
}

int MinimaxPlayer::negamax(GameState& state, int depth, int ply, int alpha, int beta, SearchCtx& ctx) {
    if (timedOut(ctx)) return 0;

    int alphaOrig = alpha;
    std::uint64_t key = hashState(state);
    Move ttBest;  // invalide par defaut
    auto it = tt_.find(key);
    if (it != tt_.end() && it->second.depth >= depth) {
        const TTEntry& e = it->second;
        if (e.flag == TT_EXACT) return e.score;
        if (e.flag == TT_LOWER && e.score > alpha) alpha = e.score;
        else if (e.flag == TT_UPPER && e.score < beta) beta = e.score;
        if (alpha >= beta) return e.score;
        ttBest = e.best;
    } else if (it != tt_.end()) {
        // Profondeur insuffisante: on garde quand meme la best move pour
        // le move ordering.
        ttBest = it->second.best;
    }

    if (state.isFinished()) {
        return evaluate(state);
    }
    if (depth == 0) {
        return quiescence(state, alpha, beta, 0, ctx);
    }

    std::vector<Move> moves = state.legalMoves();
    if (moves.empty()) return evaluate(state);

    // Move ordering: PV move (TT best) d'abord, puis tri par eval si la
    // profondeur restante le justifie. Seuil 4: a profondeur 1-3 le tri
    // par eval coute autant que la recherche elle-meme.
    std::vector<Scored> ordered;
    ordered.reserve(moves.size());
    // Bonus prioritaires (plus grands que tout score eval):
    //   ttBest = +2_000_000 (PV move connue)
    //   killer = +1_000_000 (coup ayant produit une coupure ailleurs au meme ply)
    if (depth >= 4) {
        for (size_t i = 0; i < moves.size(); ++i) {
            int pf_r = state.forcedSubRow(), pf_c = state.forcedSubCol();
            state.applyMove(moves[i]);
            int s = -evaluate(state);
            state.undoMove(moves[i], pf_r, pf_c);
            if (ttBest.isValid() &&
                moves[i].row == ttBest.row && moves[i].col == ttBest.col) {
                s += 2000000;
            } else if (isKiller(ply, moves[i])) {
                s += 1000000;
            }
            ordered.push_back({moves[i], s});
        }
        std::sort(ordered.begin(), ordered.end(),
                  [](const Scored& a, const Scored& b) { return a.score > b.score; });
    } else {
        // Profondeur faible: bonus statique pour ttBest puis killers.
        for (size_t i = 0; i < moves.size(); ++i) {
            int s = 0;
            if (ttBest.isValid() &&
                moves[i].row == ttBest.row && moves[i].col == ttBest.col) {
                s = 2000000;
            } else if (isKiller(ply, moves[i])) {
                s = 1000000;
            }
            ordered.push_back({moves[i], s});
        }
        std::sort(ordered.begin(), ordered.end(),
                  [](const Scored& a, const Scored& b) { return a.score > b.score; });
    }

    int  best     = -INF;
    Move bestMove = ordered[0].move;
    for (size_t i = 0; i < ordered.size(); ++i) {
        int pf_r = state.forcedSubRow(), pf_c = state.forcedSubCol();
        state.applyMove(ordered[i].move);
        int score = -negamax(state, depth - 1, ply + 1, -beta, -alpha, ctx);
        state.undoMove(ordered[i].move, pf_r, pf_c);
        if (ctx.stop) return 0;
        if (score > WIN_VAL - 1000)        score -= WIN_DEPTH_PENALTY;
        else if (score < -WIN_VAL + 1000)  score += WIN_DEPTH_PENALTY;
        if (score > best) {
            best     = score;
            bestMove = ordered[i].move;
        }
        if (best > alpha)  alpha = best;
        if (alpha >= beta) {
            // Coupure beta: mémoriser ce coup comme killer pour ce ply.
            recordKiller(ply, ordered[i].move);
            break;
        }
    }

    // Stockage TT
    TTEntry e;
    e.score = best;
    e.depth = static_cast<std::int16_t>(depth);
    e.best  = bestMove;
    if (best <= alphaOrig)      e.flag = TT_UPPER;
    else if (best >= beta)      e.flag = TT_LOWER;
    else                        e.flag = TT_EXACT;
    tt_[key] = e;

    return best;
}

// Quiescence search: a la frontiere d'horizon, on n'arrete que sur des
// positions "calmes". On etend uniquement les coups tactiques (gain
// d'une sous-grille ou gain global) pour eviter l'effet d'horizon.
int MinimaxPlayer::quiescence(GameState& state, int alpha, int beta, int qdepth, SearchCtx& ctx) {
    if (timedOut(ctx)) return 0;

    int standPat = evaluate(state);
    if (qdepth >= 6)        return standPat;
    if (state.isFinished()) return standPat;
    if (standPat >= beta)   return beta;
    if (standPat > alpha)   alpha = standPat;

    Cell mover = state.currentPlayer();
    std::vector<Move> moves = state.legalMoves();
    for (size_t i = 0; i < moves.size(); ++i) {
        const Move& m = moves[i];
        int pf_r = state.forcedSubRow(), pf_c = state.forcedSubCol();
        state.applyMove(m);
        bool tactical = (state.winner() != Cell::EMPTY) ||
                        (state.board().subWinner(m.row / 3, m.col / 3) == mover);
        if (!tactical) {
            state.undoMove(m, pf_r, pf_c);
            continue;
        }
        int score = -quiescence(state, -beta, -alpha, qdepth + 1, ctx);
        state.undoMove(m, pf_r, pf_c);
        if (ctx.stop) return 0;
        if (score >= beta)  return beta;
        if (score > alpha)  alpha = score;
    }
    return alpha;
}
