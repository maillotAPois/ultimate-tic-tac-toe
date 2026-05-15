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

    // Eval asymetrique (pessimiste): le progres de l'adversaire pese plus
    // lourd que le notre. L'IA perd par sur-evaluation -- elle fonce dans
    // des positions "+1700" en realite perdues. En gonflant les menaces
    // adverses et en escomptant les notres, elle devient mefiante et
    // refuse d'entrer dans ces pieges.
    constexpr int META_MINE = 300;   // ligne meta en notre faveur
    constexpr int META_OPP  = 480;   // ligne meta en faveur adverse
    constexpr int FORK_MINE = 1500;  // >= 2 menaces meta a nous
    constexpr int FORK_OPP  = 2600;  // >= 2 menaces meta adverses
    constexpr int FREE_CHOICE_VAL  = 120;  // malus de controle (libre choix donne)
    constexpr int FORCED_MY_OPEN   = 200;  // 2-en-ligne ouverte a nous dans la sub forcee
    constexpr int FORCED_THEM_OPEN = 350;  // 2-en-ligne ouverte adverse (on doit bloquer)

    struct Scored {
        Move move;
        int  score;
    };

}

MinimaxPlayer::MinimaxPlayer(int depth, int maxDepth, int budgetMs)
    : depth_(depth)
    , maxDepth_(maxDepth)
    , budgetMs_(budgetMs)
    , tt_(TT_SIZE)
    , moveBufs_(MOVE_BUFS_MAX)
{
    for (int i = 0; i < KILLERS_MAX_PLY; ++i) {
        killers_[i][0] = Move();
        killers_[i][1] = Move();
    }
    for (int s = 0; s < 2; ++s)
        for (int c = 0; c < 81; ++c)
            history_[s][c] = 0;
    for (int i = 0; i < MOVE_BUFS_MAX; ++i) {
        moveBufs_[i].reserve(20);
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
    // Le hash est desormais maintenu incrementalement par GameState
    // (apply/undoMove). Cette fonction devient O(1).
    return state.hash();
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

    // Nombre de 2-en-ligne "ouvertes" (2 a soi, 0 adverse) d'un joueur
    // dans une sous-grille: un coup les transforme en victoire de sub.
    int countOpenTwos(const Board& b, Cell me) {
        Cell them = opponent(me);
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
        int n = 0;
        for (int li = 0; li < 8; ++li) {
            int p = 0, o = 0;
            for (int k = 0; k < 3; ++k) {
                Cell c = b.get(lines[li][k][0], lines[li][k][1]);
                if (c == me)        ++p;
                else if (c == them) ++o;
            }
            if (p == 2 && o == 0) ++n;
        }
        return n;
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

    // 1) Score positionnel par sous-grille. Sub forcee = 3x: ce qui s'y
    //    passe est imminent (l'adversaire y joue obligatoirement).
    int fr = state.forcedSubRow(), fc = state.forcedSubCol();
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            int w = POS_W[br][bc];
            if (br == fr && bc == fc) w *= 3;
            score += w * evalSubBoard(state.board().sub(br, bc), me);
        }
    }

    // 2) Score de la meta-grille, asymetrique et avec dead-sub blocking.
    //    Une sous-grille pleine sans gagnant neutralise la ligne meta
    //    (valeur fantome). Le progres adverse est pondere plus lourd.
    const UltimateBoard& ub = state.board();
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
    int myMetaThreats = 0, oppMetaThreats = 0;
    for (int li = 0; li < 8; ++li) {
        int p = 0, o = 0;
        bool deadLine = false;
        for (int k = 0; k < 3; ++k) {
            int br = lines[li][k][0], bc = lines[li][k][1];
            Cell sw = ub.subWinner(br, bc);
            if (sw == me)        ++p;
            else if (sw == them) ++o;
            else if (ub.subFinished(br, bc)) { deadLine = true; break; }
        }
        if (deadLine) continue;
        int ls = lineScore(p, o);
        score += (ls >= 0 ? META_MINE : META_OPP) * ls;
        if (p == 2 && o == 0) ++myMetaThreats;
        if (o == 2 && p == 0) ++oppMetaThreats;
    }
    if (myMetaThreats  >= 2) score += FORK_MINE;
    if (oppMetaThreats >= 2) score -= FORK_OPP;

    // 3) Controle du flux. Le joueur au trait doit jouer dans la sous-
    //    grille forcee: y completer une 2-en-ligne est un avantage,
    //    devoir bloquer celle de l'adversaire un malus (pondere plus
    //    lourd). En libre choix, c'est lui qui a l'avantage de placement.
    if (fr < 0) {
        score += FREE_CHOICE_VAL;
    } else {
        const Board& fs = ub.sub(fr, fc);
        score += FORCED_MY_OPEN   * countOpenTwos(fs, me);
        score -= FORCED_THEM_OPEN * countOpenTwos(fs, them);
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

    // TT en tableau fixe: persiste entre coups, pas de purge necessaire
    // (les creneaux sont ecrases, les collisions detectees par la cle).
    for (int i = 0; i < KILLERS_MAX_PLY; ++i) {
        killers_[i][0] = Move();
        killers_[i][1] = Move();
    }
    for (int s = 0; s < 2; ++s)
        for (int c = 0; c < 81; ++c)
            history_[s][c] = 0;

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
    const TTEntry& probe = tt_[key & TT_MASK];
    if (probe.key == key) {
        if (probe.depth >= depth) {
            if (probe.flag == TT_EXACT) return probe.score;
            if (probe.flag == TT_LOWER && probe.score > alpha) alpha = probe.score;
            else if (probe.flag == TT_UPPER && probe.score < beta) beta = probe.score;
            if (alpha >= beta) return probe.score;
        }
        // Profondeur insuffisante ou bornes: on garde la best move pour
        // le move ordering.
        ttBest = probe.best;
    }

    if (state.isFinished()) {
        return evaluate(state);
    }
    if (depth == 0) {
        return quiescence(state, alpha, beta, 0, ctx);
    }

    std::vector<Move>& moves = moveBufs_[ply < MOVE_BUFS_MAX ? ply : MOVE_BUFS_MAX - 1];
    state.legalMoves(moves);
    if (moves.empty()) return evaluate(state);

    const Cell mover = state.currentPlayer();
    const int  si    = (mover == Cell::X) ? 0 : 1;

    // Move ordering: PV move (TT best) d'abord, puis tri par eval si la
    // profondeur restante le justifie. Seuil 4: a profondeur 1-3 le tri
    // par eval coute autant que la recherche elle-meme.
    std::vector<Scored> ordered;
    ordered.reserve(moves.size());
    // Bonus prioritaires (plus grands que tout score eval ou historique):
    //   ttBest = +2_000_000 (PV move connue)
    //   killer = +1_000_000 (coup ayant produit une coupure au meme ply)
    //   sinon  = score d'historique (coupures beta cumulees de ce coup)
    auto orderBonus = [&](const Move& m) -> int {
        if (ttBest.isValid() && m.row == ttBest.row && m.col == ttBest.col)
            return 2000000;
        if (isKiller(ply, m))
            return 1000000;
        return history_[si][m.row * 9 + m.col];
    };
    if (depth >= 4) {
        for (size_t i = 0; i < moves.size(); ++i) {
            int pf_r = state.forcedSubRow(), pf_c = state.forcedSubCol();
            state.applyMove(moves[i]);
            int s = -evaluate(state);
            state.undoMove(moves[i], pf_r, pf_c);
            ordered.push_back({moves[i], s + orderBonus(moves[i])});
        }
    } else {
        for (size_t i = 0; i < moves.size(); ++i) {
            ordered.push_back({moves[i], orderBonus(moves[i])});
        }
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const Scored& a, const Scored& b) { return a.score > b.score; });

    // PVS (Principal Variation Search): le premier coup (PV) est cherche
    // en fenetre complete; les suivants en fenetre nulle [alpha, alpha+1]
    // pour prouver rapidement qu'ils sont inferieurs au PV. Si un coup
    // null-window passe au-dessus d'alpha (mais sous beta), on re-cherche
    // en fenetre complete. Speedup ~2x sur les bonnes orderings.
    int  best     = -INF;
    Move bestMove = ordered[0].move;
    for (size_t i = 0; i < ordered.size(); ++i) {
        int pf_r = state.forcedSubRow(), pf_c = state.forcedSubCol();
        const Move& mv = ordered[i].move;
        state.applyMove(mv);
        int score;
        if (i == 0) {
            score = -negamax(state, depth - 1, ply + 1, -beta, -alpha, ctx);
        } else {
            // LMR: les coups tardifs, non tactiques et hors killers sont
            // d'abord cherches a profondeur reduite (fenetre nulle). S'ils
            // depassent alpha, re-recherche a profondeur pleine. Les coups
            // tactiques (gain de sous-grille ou global) ne sont pas reduits.
            bool tactical = (state.winner() != Cell::EMPTY) ||
                            (state.board().subWinner(mv.row / 3, mv.col / 3) == mover);
            int reduction = 0;
            if (depth >= 3 && i >= 3 && !tactical && !isKiller(ply, mv)) {
                reduction = (i >= 6 && depth >= 6) ? 2 : 1;
            }
            score = -negamax(state, depth - 1 - reduction, ply + 1,
                             -alpha - 1, -alpha, ctx);
            if (!ctx.stop && reduction > 0 && score > alpha) {
                score = -negamax(state, depth - 1, ply + 1,
                                 -alpha - 1, -alpha, ctx);
            }
            if (!ctx.stop && score > alpha && score < beta) {
                score = -negamax(state, depth - 1, ply + 1, -beta, -alpha, ctx);
            }
        }
        state.undoMove(mv, pf_r, pf_c);
        if (ctx.stop) return 0;
        if (score > WIN_VAL - 1000)        score -= WIN_DEPTH_PENALTY;
        else if (score < -WIN_VAL + 1000)  score += WIN_DEPTH_PENALTY;
        if (score > best) {
            best     = score;
            bestMove = mv;
        }
        if (best > alpha)  alpha = best;
        if (alpha >= beta) {
            // Coupure beta: ce coup devient killer pour ce ply et son
            // score d'historique est renforce (pondere par la profondeur).
            recordKiller(ply, mv);
            history_[si][mv.row * 9 + mv.col] += depth * depth;
            break;
        }
    }

    // Stockage TT: remplacement preferant la profondeur. On ecrase si le
    // creneau est vide, occupe par une autre position, ou par une entree
    // moins profonde.
    TTEntry& slot = tt_[key & TT_MASK];
    if (slot.key != key || depth >= slot.depth) {
        slot.key   = key;
        slot.score = best;
        slot.depth = static_cast<std::int16_t>(depth);
        slot.best  = bestMove;
        if (best <= alphaOrig)      slot.flag = TT_UPPER;
        else if (best >= beta)      slot.flag = TT_LOWER;
        else                        slot.flag = TT_EXACT;
    }

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
    // Buffers reserves au-dela de KILLERS_MAX_PLY pour la quiescence (qdepth 0..5).
    int qIdx = KILLERS_MAX_PLY + qdepth;
    if (qIdx >= MOVE_BUFS_MAX) qIdx = MOVE_BUFS_MAX - 1;
    std::vector<Move>& moves = moveBufs_[qIdx];
    state.legalMoves(moves);
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
