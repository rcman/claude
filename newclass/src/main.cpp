#include "Game.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    Game game;
    printf("About to do game.init\n");
    if (!game.init("Zombie Survival", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, false)) {
        std::cerr << "Game initialization failed!" << std::endl;
        return 1;
    }
    printf("Game is about to be running\n");
    while (game.isRunning()) {
        Uint32 frameStart = SDL_GetTicks();

        game.handleEvents();
        printf("HandleEvents\n");
        game.update();
        printf("About to run game render\n");
        game.render();

        game.capFrameRate(frameStart);
    }

    return 0;
}
