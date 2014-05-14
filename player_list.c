#include "player_list.h"

PlayerList newPlayerList(int size) {
    PlayerList playerList = (PlayerList) calloc(1, sizeof(struct _playerList));
    playerList->players = (Player *) calloc(size, sizeof(struct _player));
    playerList->cur = 0;
    playerList->size = size;
    return playerList;
}

void freePlayerList(PlayerList playerList) {
    int i = 0;
    for (i = 0; i < playerList->cur; i++) {
        freePlayer(playerList->players[i]);
    }
}

void addPlayer(PlayerList playerList, Player player) {
    if (playerList->cur == playerList->size) {
        return;
    }
    playerList->players[playerList->cur++] = player;
}

void removePlayer(PlayerList playerList, Player player) {
    int i = 0, j = 0;
    for (i = 0; i < playerList->cur; i++) {
        if (playerList->players[i]->id == player->id) {
            playerList->cur--;
            for (j = i; j < playerList->cur; j++) {
                playerList->players[j] = playerList->players[j+1];
            }
            playerList->players[j + 1] = NULL;
            return;
        }
    }
}

void removePlayerById(PlayerList playerList, int playerId) {
    Player player = getPlayerById(playerList, playerId);
    if (player != NULL) {
        removePlayer(playerList, player);
    }
}

Player getPlayer(PlayerList playerList, int index) {
    if (index >= playerList->cur) {
        // Array index out of bound
        exit(1);
    }
    return playerList->players[index];
}

Player getPlayerById(PlayerList playerList, int playerId) {
    int i = 0;
    for (i = 0; i < playerList->cur; i++) {
        if (playerList->players[i]->id == playerId) {
            return playerList->players[i];
        }
    }
    return NULL;
}
