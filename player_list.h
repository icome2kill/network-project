#include "player.h"

typedef struct _playerList {
    Player * players;
    int cur;
    int size;
} * PlayerList;

PlayerList newPlayerList(int );
void freePlayerList(PlayerList );
void addPlayer(PlayerList , Player );
void removePlayer(PlayerList, Player );
void removePlayerById(PlayerList, int );
Player getPlayer(PlayerList, int );
Player getPlayerById(PlayerList, int );