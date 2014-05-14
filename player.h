#include <stdlib.h>

typedef struct _player {
	int id;         // Currently is sockfd of client.
	int state;      // 0 = lobby, 1 = waiting, 2 = playing
	int lives;      // Lives left to make a guess
	char name[50];
}* Player;

Player newPlayer(int id, int state, int lives, char* name);
void freePlayer(Player player);