#include <iostream>
#include "main.h"

int main()
{
    // Game initialization
    game.initialize(10, Level::EASY_1, Mode::DEBUG, false, "Pseudo");

    while (!game.isAllGameFinish())
    {
        GameMove myMove{0, 0};

        while (!game.isFinish())
        {
            // Get IA move
            GameMove gameMove;
            game.getMove(gameMove);
            std::cerr << "IA move " << gameMove.row << " " << gameMove.col << std::endl;

            // Send your move
            std::cerr << "Send move " << myMove.row << " " << myMove.col << std::endl;
            game.setMove(myMove);
        }
    }

    return 0;
}
