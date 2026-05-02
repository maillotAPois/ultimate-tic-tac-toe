#include "Evaluator.h"

namespace {
    static const int WIN_VAL          = 100000;
    static const int SUB_BASE_VAL     = 1000;
    static const int META_THREAT_VAL  = 5000;
    static const int SUB_THREAT_VAL   = 100;
    static const int CENTER_BONUS_VAL = 50;
    static const int FREE_CHOICE_PEN  = 80;  // penalite si adv est libre
}

int Evaluator::subPositionWeight(int br, int bc) {
    // Centre du meta-plateau: position la plus puissante (intervient
    // dans 4 alignements). Coins: 3 alignements. Bords: 2.
    if (br == 1 && bc == 1) return 4;
    if ((br == 0 || br == 2) && (bc == 0 || bc == 2)) return 3;
    return 2;
}

int Evaluator::evaluate(const GameState& state) {
    Cell me   = state.currentPlayer();
    Cell them = opponent(me);

    // 1) Cas terminaux: priorite absolue
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
    const UltimateBoard& b = state.board();

    // 2) Sous-grilles gagnees ponderees par leur position
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            Cell w2 = b.subWinner(br, bc);
            int weight = subPositionWeight(br, bc);
            if (w2 == me)   score += SUB_BASE_VAL * weight;
            if (w2 == them) score -= SUB_BASE_VAL * weight;
        }
    }

    // 3) Menaces sur la meta-grille: lignes/colonnes/diagonales ou un
    //    joueur a 2 sous-grilles gagnees et la 3e n'est pas perdue.
    Board metaView   = b.metaView();
    int   meThreats   = metaView.countAlignments(me, 2);
    int   themThreats = metaView.countAlignments(them, 2);
    score += META_THREAT_VAL * meThreats;
    score -= META_THREAT_VAL * themThreats;

    // Fork: avoir 2+ menaces meta simultanees est quasi gagnant car
    // l'adversaire ne peut pas toutes les bloquer en un seul coup.
    if (meThreats   >= 2) score += META_THREAT_VAL * 2;
    if (themThreats >= 2) score -= META_THREAT_VAL * 2;

    // 4) Menaces internes aux sous-grilles non terminees.
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            if (b.subFinished(br, bc)) continue;
            score += SUB_THREAT_VAL * b.sub(br, bc).countAlignments(me, 2);
            score -= SUB_THREAT_VAL * b.sub(br, bc).countAlignments(them, 2);
        }
    }

    // 5) Bonus pour la case centrale globale (4,4) - centre du centre.
    Cell center = b.cellAt(4, 4);
    if (center == me)   score += CENTER_BONUS_VAL;
    if (center == them) score -= CENTER_BONUS_VAL;

    // 6) Penalite si la sous-grille forcee est libre pour le prochain
    //    coup (l'adversaire serait libre car notre joueur courant est
    //    nous-memes a la racine, mais ici state.currentPlayer = me
    //    et c'est `me` qui est sur le point de jouer, donc la liberte
    //    est en notre faveur si pas forcee). On code cette nuance:
    if (state.forcedSubRow() < 0) {
        // Liberte de choix: avantage pour le joueur courant.
        score += FREE_CHOICE_PEN;
    }

    return score;
}
