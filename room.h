#include "player_list.h"

typedef struct _room {
    int id;
    char name[50];
    PlayerList playerList;
    int state; // 0 = waiting, 1 = playing
    int currentPlayerIndex;
    int timeLeft; // Time left for current player to make a guess
    int target; // Target number
} * Room;

Room newRoom(int id, char* name, int size);
void freeRoom(Room room);
void addPlayerToRoom(Room room, Player player);
void removePlayerFromRoom(Room room, Player player);
void removePlayerFromRoomById(Room room, int playerId);
Player getCurrentPlayerFromRoom(Room );