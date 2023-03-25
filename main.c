#include "pico/stdlib.h"
#include "system.h"
#include "game.h"

#include "music.h"
#include "resource.h"

int main() {
   system_init();  // Initialize the System resources (LCD/Framebuffer/...etc)
   game_init();    // Initialize the game engine
   game_loop();    // enter the game loop
}


