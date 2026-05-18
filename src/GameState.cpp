#include "GameState.h"

namespace {
    // Table Zobrist: 81 cases * 2 joueurs + 10 sous-grilles cibles (0..8
    // forcees + 9 "libre") + 1 bit cote au trait. Initialisee une fois
    // par xorshift deterministe pour rester reproductible.
    struct ZobristTable {
        std::uint64_t cells[81][2];
        std::uint64_t forced[10];
        std::uint64_t side;
        ZobristTable() {
            std::uint64_t s = 0x9E3779B97F4A7C15ULL;
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

    inline int forcedIdx(int fr, int fc) {
        return (fr < 0) ? 9 : (fr * 3 + fc);
    }
    inline int cellIdx(int row, int col) { return row * 9 + col; }
    inline int playerIdx(Cell c) { return (c == Cell::X) ? 0 : 1; }
}

GameState::GameState()
    : board_()
    , currentPlayer_(Cell::X)
    , forcedSubRow_(-1)
    , forcedSubCol_(-1)
    , moveCount_(0)
    , hash_(kZob.forced[9]) // forced=libre, X au trait, plateau vide
{}

bool GameState::isFinished() const {
    return board_.metaWinner() != Cell::EMPTY || board_.allSubFinished();
}

Cell GameState::winner() const {
    return board_.metaWinner();
}

bool GameState::isLegal(const Move& m) const {
    if (!m.isValid() || m.row > 8 || m.col > 8) return false;
    if (board_.cellAt(m.row, m.col) != Cell::EMPTY) return false;
    int br = m.row / 3, bc = m.col / 3;
    if (board_.subFinished(br, bc)) return false;
    if (forcedSubRow_ >= 0) return br == forcedSubRow_ && bc == forcedSubCol_;
    return true;
}

void GameState::undoMove(const Move& m, int prevForcedRow, int prevForcedCol) {
    // XOR self-inverse: on annule cell+forced+side dans l'ordre inverse
    // de applyMove.
    currentPlayer_ = opponent(currentPlayer_);
    hash_ ^= kZob.side;
    hash_ ^= kZob.forced[forcedIdx(forcedSubRow_, forcedSubCol_)];
    hash_ ^= kZob.forced[forcedIdx(prevForcedRow, prevForcedCol)];
    hash_ ^= kZob.cells[cellIdx(m.row, m.col)][playerIdx(currentPlayer_)];
    board_.place(m.row, m.col, Cell::EMPTY);
    --moveCount_;
    forcedSubRow_ = prevForcedRow;
    forcedSubCol_ = prevForcedCol;
}

bool GameState::applyMove(const Move& m) {
    if (!isLegal(m)) return false;
    hash_ ^= kZob.cells[cellIdx(m.row, m.col)][playerIdx(currentPlayer_)];
    board_.place(m.row, m.col, currentPlayer_);
    ++moveCount_;
    int nextBr = m.row % 3, nextBc = m.col % 3;
    hash_ ^= kZob.forced[forcedIdx(forcedSubRow_, forcedSubCol_)];
    if (board_.subFinished(nextBr, nextBc)) {
        forcedSubRow_ = -1;
        forcedSubCol_ = -1;
    } else {
        forcedSubRow_ = nextBr;
        forcedSubCol_ = nextBc;
    }
    hash_ ^= kZob.forced[forcedIdx(forcedSubRow_, forcedSubCol_)];
    hash_ ^= kZob.side;
    currentPlayer_ = opponent(currentPlayer_);
    return true;
}

void GameState::legalMoves(std::vector<Move>& out) const {
    out.clear();
    if (isFinished()) return;

    auto pushSubMoves = [&](int br, int bc) {
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                if (board_.sub(br, bc).isEmpty(r, c))
                    out.push_back(Move(br * 3 + r, bc * 3 + c));
    };

    if (forcedSubRow_ >= 0) {
        pushSubMoves(forcedSubRow_, forcedSubCol_);
    } else {
        for (int br = 0; br < 3; ++br)
            for (int bc = 0; bc < 3; ++bc)
                if (!board_.subFinished(br, bc)) pushSubMoves(br, bc);
    }
}

std::vector<Move> GameState::legalMoves() const {
    std::vector<Move> moves;
    moves.reserve(20);
    legalMoves(moves);
    return moves;
}
