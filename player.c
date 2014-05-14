#include "player.h"

Player newPlayer(int id, int state, int lives, char* name) {
    Player player = (Player) calloc(1, sizeof (struct _player));
    player->id = id;
    player->state = state;
    player->lives = lives;
    strcpy(player->name, name);
    return player;
}

void freePlayer(Player player) {
    free(player);
}