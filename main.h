#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <string>

enum class Level {
    EASY_1,
    EASY_2,
    MEDIUM_1,
    MEDIUM_2,
    HARD_1,
    HARD_2,
    VERY_HARD_1,
    VERY_HARD_2,
};

enum class Mode {
    DEBUG,
    ARENA,
};

enum Winner {
    NO_WINNER,
    IA,
    PLAYER,
    IA_AND_PLAYER,
};

struct GameMove {
    int row;
    int col;
};

struct IGame {
    virtual ~IGame() = default;
    virtual Winner getWinner() const = 0;
    virtual bool isFinish() = 0;
    virtual bool isAllGameFinish() const = 0;
    virtual bool getMove(GameMove& outMove) = 0;
    virtual void setMove(const GameMove& move) = 0;
    virtual void initialize(unsigned int nbGame, Level level, Mode mode, bool alwaysPlayFirst, const std::string& alias = "Player") = 0;
};

extern IGame& game;

#endif // MAIN_H_INCLUDED
