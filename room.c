#include "room.h"

Room newRoom(int id, char* name, int size) {
    Room room = (Room) calloc(1, sizeof (struct _room));
    room->id = id;
    strcpy(room->name, name);
    room->state = 0;
    room->playerList = newPlayerList(size);
    room->target = -1;
    room->currentPlayerIndex = -1;
    return room;
}

void freeRoom(Room room) {
    freePlayerList(room->playerList);
    free(room);
}

void addPlayerToRoom(Room room, Player player) {
    addPlayer(room->playerList, player);
}

void removePlayerFromRoom(Room room, Player player) {
    removePlayer(room->playerList, player);
}

void removePlayerFromRoomById(Room room, int playerId) {
    removePlayerById(room->playerList, playerId);
    
    if (room->currentPlayerIndex > room->playerList->cur - 1) {
        room->currentPlayerIndex = room->playerList->cur - 1;
    }
}

Player getCurrentPlayerFromRoom(Room room) {
    if (room->currentPlayerIndex >= 0 && room->currentPlayerIndex < room->playerList->cur) {
        return room->playerList->players[room->currentPlayerIndex];
    }
    return NULL;
}