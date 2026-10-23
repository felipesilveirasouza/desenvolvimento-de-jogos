#include "NPC.hpp"

// NPC is fully defined in its header: it is an Entity plus a public
// sf::Sprite that the game loop positions and draws. This translation unit
// exists so the class has its own .cpp like every other one, and so the
// header is checked to compile on its own (it needs SFML's Graphics module).
