#include	"unp.h"
#include	"constant.h"
#include        "room.h"
#include        "utils.h"

#define MAX_PLAYER FD_SETSIZE
#define CHUNK_SIZE 1024         // Maximum message size limits. 1024 = 1MB
#define MAX_ROOM 10

char * preparePlayerListMessage(PlayerList, int *);

void processRoomLogic(Room);

void processMessage(char*, int, int);
void processLoginMessage(char *, int, int);
void processLogoutMessage(int);
void processListPlayerMessage(int);
void processListRoomMessage(int);
void processJoinRoomMessage(int, char *, int);
void processGuessNumberMessage(int, char *, int);
void processLeaveRoomMessage(int, char *, int);

void sendPlayerJoinedRoomMessage(Player, Room);
void sendPlayerLeaveRoomMessage(Player, Room);
void sendChangeTurnMessage(Room room);
void sendGameEndMessage(Room, int);

Room roomList[MAX_ROOM];
PlayerList playerList;
fd_set allset;
int client[FD_SETSIZE];

int main() {
    printf("Starting server\n");
    int listenfd, connfd, sockfd;
    int maxi, maxfd, nready;
    int i = 0, j = 0;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    fd_set rset;
    ssize_t n;
    char buf[MAXLINE];
    int byte_sent[FD_SETSIZE], byte_recv[FD_SETSIZE], number[FD_SETSIZE];

    FD_ZERO(&allset);

    printf("Initializing\n");
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    Bind(listenfd, (SA *) & servaddr, sizeof (servaddr));

    Listen(listenfd, LISTENQ);

    FD_SET(listenfd, &allset);

    for (i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;
        byte_sent[i] = 0;
        byte_recv[i] = 0;
        number[i] = 0;
    }

    char roomName[50];
    for (i = 0; i < MAX_ROOM; i++) {
        sprintf(roomName, "Room no %d", i + 1);
        roomList[i] = newRoom(i + 1, roomName, 3);
    }

    maxfd = listenfd;
    maxi = -1;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    playerList = newPlayerList(MAX_PLAYER);

    printf("Initialization completed\n");

    for (;;) {
        rset = allset;
        nready = Select(maxfd + 1, &rset, NULL, NULL, &timeout);

        if (FD_ISSET(listenfd, &rset)) {
            // Client connected
            clilen = sizeof (cliaddr);
            connfd = Accept(listenfd, (SA *) & cliaddr, &clilen);

            for (j = 0; j < FD_SETSIZE; j++) {
                if (client[j] < 0) {
                    client[j] = connfd;
                    break;
                }
            }

            if (j == FD_SETSIZE) {
                err_quit("Too many client :(");
            }

            FD_SET(connfd, &allset);

            if (connfd > maxfd) {
                maxfd = connfd;
            }
            if (j > maxi) {
                // max index in client array client[]
                maxi = j;
            }
            if (--nready <= 0) {
                // No more descriptors to read
                continue;
            }
        }

        // Check all client for data
        for (j = 0; j <= maxi; j++) {
            if ((sockfd = client[j]) < 0) {
                continue;
            }
            if (FD_ISSET(sockfd, &rset)) {
                if ((n = read(sockfd, buf, MAXLINE)) == 0 || strcmp(buf, "\n") == 0) {
                    // Connection closed by client
                    // remove from room
                    for (i = 0; i < MAX_ROOM; i++) {
                        removePlayerFromRoomById(roomList[i], sockfd);
                    }
                    // Log the client out.
                    processLogoutMessage(sockfd);
                    Close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[j] = -1;
                } else {
                    processMessage(buf, n, sockfd);
                }
                if (--nready <= 0) {
                    // No more descriptors to read
                    continue;
                }
            }
        }

        // Check all room to process game logic
        if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
            // 1 second has passed
            printf("1 sec\n");
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            for (j = 0; j < MAX_ROOM; j++) {
                processRoomLogic(roomList[j]);
            }
        }
    }
}

/**
 * Control game logic
 * @param room
 */
void processRoomLogic(Room room) {
    int i = 0;
    char message[9];

    if (room->state == 0) {
        if (room->playerList->cur == room->playerList->size) {
            if (room->timeLeft > 0) {
                // Wait time before start the game
                room->timeLeft -= 1;
                return;
            }
            // Start the game as the room is full
            room->target = 512; // For testing purpose
            room->state = 1;
            room->currentPlayerIndex = 0;
            room->timeLeft = 30;

            message[0] = ID_GAME_START;
            memcpy(message + 1, &room->currentPlayerIndex, 4);
            memcpy(message + 5, &room->timeLeft, 4);

            // Broadcast the game start to all players in room.
            for (i = 0; i < room->playerList->cur; i++) {
                room->playerList->players[i]->state = 2;
                Writen(room->playerList->players[i]->id, message, 9);
            }
        }
    } else if (room->state == 1) {
        //Check for number of player in room
        if (room->playerList->cur == 1) {
            // Only 1 player left. Make him the winner.
            room->timeLeft = 0;
            sendGameEndMessage(room, room->playerList->players[0]->id);
            return;
        } else if (room->playerList->cur == 0) {
            // Unlikely to happens but who knows...
            room->state = 0;
            return;
        }

        room->timeLeft -= 1;

        if (room->timeLeft == 0) {
            // Change player turn
            room->timeLeft = 30;
            room->currentPlayerIndex = (room->currentPlayerIndex + 1) % room->playerList->cur;

            // Broadcast player turn changed to all players in room
            sendChangeTurnMessage(room);
        }
    }
}

/**
 * Return a list of player.
 * Format: 
 *              bytes   Description   
 *               0-3      playerId
 *               4-7      state
 *               8-11     lives
 *               12-61    name
 *               ...      Another player.  
 * @param sockfd
 * @param *playerInfoSize size of player info
 */
char * preparePlayerListMessage(PlayerList playerList, int * playerInfoSize) {
    *playerInfoSize = 62;
    char * message = (char *) calloc(playerList->cur, (*playerInfoSize) * playerList->cur);

    Player player = NULL;

    int i = 0;
    for (i = 0; i < playerList->cur; i++) {
        player = getPlayer(playerList, i);

        memcpy(message + (*playerInfoSize) * i, &(player->id), 4);
        memcpy(message + 4 + (*playerInfoSize) * i, &(player->state), 4);
        memcpy(message + 8 + (*playerInfoSize) * i, &(player->lives), 4);
        memcpy(message + 12 + (*playerInfoSize) * i, player-> name, 50);

        printf("%s\n", message);
    }
    return message;
}

/**
 * All received message run through this function.
 * @param message
 * @param len
 * @param sockfd
 */
void processMessage(char* message, int len, int sockfd) {
    if (message[0] == ID_LOGIN) {
        char name[51];
        stripMessageId(message, len, name);
        processLoginMessage(name, 51, sockfd);
        return;
    } else if (message[0] == ID_LOGOUT) {
        processLogoutMessage(sockfd);
        return;
    } else if (message[0] == ID_LIST_PLAYER) {
        processListPlayerMessage(sockfd);
        return;
    } else if (message[0] == ID_LIST_ROOM) {
        processListRoomMessage(sockfd);
        return;
    } else if (message[0] == ID_JOIN_ROOM) {
        processJoinRoomMessage(sockfd, message + 1, len - 1);
        return;
    } else if (message[0] == ID_GUESS_NUMBER) {
        processGuessNumberMessage(sockfd, message + 1, len - 1);
        return;
    } else if (message[0] == ID_LEAVE_ROOM) {
        processLeaveRoomMessage(sockfd, message + 1, len - 1);
        return;
    }
    printf("This should not be happenning...\n");
}

/**
 * Will return message to client to indicate their login status.
 * Format:
 *              bytes   description
 *               0       ID_LOGIN
 *               1       status(1 or 0)
 * Success = 1. Fail = 0.
 * @param name
 * @param len
 * @param sockfd
 */
void processLoginMessage(char * name, int len, int sockfd) {
    char response[6];

    response[0] = ID_LOGIN;

    if (playerList->cur == playerList->size) {
        response[1] = 0;
        Writen(sockfd, response, 2);
        return;
    }
    response[1] = 1;
    memcpy(response + 2, &sockfd, 4);

    Writen(sockfd, response, 6);

    addPlayer(playerList, newPlayer(sockfd, 0, 0, name));
    printf("id = %d, %s logged in. Current player: %d\n", sockfd, name, playerList->cur);
}

/**
 * Process client's request to logout
 * @param sockfd
 */
void processLogoutMessage(int sockfd) {
    printf("%d Logged out\n", sockfd);
    Player player = getPlayerById(playerList, sockfd);
    if (player != NULL) {
        removePlayer(playerList, player);
        freePlayer(player);

        FD_CLR(sockfd, &allset);
        int i = 0;
        for (i = 0; i < FD_SETSIZE; i++) {
            if (client[i] == sockfd) {
                client[i] = -1;
                break;
            }
        }
        return;
    }
    // WTF? Replay attack?
}

/**
 * Return a list of room 
 * Format:
 *              bytes   Description
 *                0      ID_LIST_ROOM
 *               1-4     room info size
 *               5-8     roomId
 *               9-12    size
 *               13-16   count (current no of player)
 *               17-20   state (0 = waiting, 1 = playing)
 *               21-60   name
 *               ...     Another room
 * @param sockfd
 */
void processListRoomMessage(int sockfd) {
    int roomInfoSize = 66;

    char message[1 + 4 + 66 * MAX_ROOM];
    char roomInfo[66];


    message[0] = ID_LIST_ROOM;
    memcpy(message + 1, &roomInfoSize, 4);
    Room room = NULL;
    int i = 0;
    for (i = 0; i < MAX_ROOM; i++) {
        room = roomList[i];
        memcpy(roomInfo, &(room->id), 4);
        memcpy(roomInfo + 4, &(room->playerList->cur), 4);
        memcpy(roomInfo + 8, &(room->playerList->size), 4);
        memcpy(roomInfo + 12, &(room->state), 4);
        memcpy(roomInfo + 16, room->name, 50);
        memcpy(message + 5 + i * roomInfoSize, roomInfo, 66);
    }

    Writen(sockfd, message, 5 + MAX_ROOM * roomInfoSize);
}

/**
 * Return a list of player.
 * @param sockfd
 */
void processListPlayerMessage(int sockfd) {
    int playerSize = 0;

    char * playerListMes = preparePlayerListMessage(playerList, &playerSize);
    char * message = (char *) calloc(playerList->cur + 1, playerSize * playerList->cur);

    message[0] = ID_LIST_PLAYER;

    memcpy(message + 1, &playerSize, 4);
    memcpy(message + 5, playerListMes, playerSize * playerList->cur);

    Writen(sockfd, message, playerSize * playerList->cur + 5);

    free(message);
    free(playerListMes);
}

/**
 * Process join room message from client
 * Return message
 * Format:
 *                      bytes   Description
 *                        0      ID_JOIN_ROOM
 *                        1      Status(0 = failed(Unknown error), 1 = Playing room, 2 = Room full, 3 = Success)
 *                       2-5     roomId
 *                       6-9     size
 *                       10-59   name
 *                        ...    player list of current room 
 * Broadcast message to players in room upon success
 * Format:
 *                      bytes   Description
 *                        0      ID_PLAYER_JOINED_ROOM
 *                       1-4     playerId
 *                       5-8     state
 *                       9-12    lives
 *                       13-62   name   
 * @param sockfd
 * @param message
 */
void processJoinRoomMessage(int sockfd, char * message, int n) {
    int playerInfoSize = 0;
    int roomId = 0;
    int len = 0;

    memcpy(&roomId, message, 4);

    if (roomId > MAX_ROOM || roomId < 0) {
        char returnMes[2];
        returnMes[0] = ID_JOIN_ROOM;
        returnMes[1] = 0;
        Writen(sockfd, returnMes, 2);
        return;
    }

    Player player = getPlayerById(playerList, sockfd);
    Room room = roomList[roomId - 1];

    if (room->state == 0 && (room->playerList->cur < room->playerList->size)) {
        player->state = 1;

        sendPlayerJoinedRoomMessage(player, room);

        addPlayerToRoom(room, player);

        char * playerListMes = preparePlayerListMessage(room->playerList, &playerInfoSize);

        len = room->playerList->cur * playerInfoSize + 64;
        char * returnMes = (char *) calloc(len, 1);

        returnMes[0] = ID_JOIN_ROOM;
        returnMes[1] = 3;
        memcpy(returnMes + 2, &room->id, 4);
        memcpy(returnMes + 6, &room->playerList->size, 4);
        memcpy(returnMes + 10, room->name, 50);
        memcpy(returnMes + 60, &playerInfoSize, 4);
        memcpy(returnMes + 64, playerListMes, playerInfoSize * room->playerList->cur);

        Writen(sockfd, returnMes, len);

        free(returnMes);
        free(playerListMes);
    } else {
        char returnMes[2];
        if (room->playerList->cur < room->playerList->size) {
            returnMes[1] = 2;
            len = 2;
        } else {
            returnMes[1] = 1;
            len = 2;
        }
        Writen(sockfd, returnMes, len);
    }
}

/**
 * Process guess number message from client
 * Return Format (Broadcast to room):
 *                      bytes   Description
 *                        0      ID_GUESS_NUMBER
 *                        1      0(smaller), 1(correct), 2(wrong)
 *                       2-5     playerId
 *                       6-9     guessed number
 * @param sockfd
 * @param message
 * @param len
 */
void processGuessNumberMessage(int sockfd, char * message, int len) {
    int roomId = 0;
    int number = 0;
    int i = 0;
    char response[10];

    memcpy(&roomId, message, 4);
    memcpy(&number, message + 4, 4);

    Room room = roomList[roomId - 1];

    response[0] = ID_GUESS_NUMBER;
    memcpy(response + 2, &sockfd, 4);
    memcpy(response + 6, &number, 4);

    if (room->state != 1) {
        // Not playing. What r u trying to guess here?
    }

    if (getCurrentPlayerFromRoom(room)->id == sockfd) {
        room->currentPlayerIndex = (room->currentPlayerIndex + 1) % room->playerList->cur;
        if (number < room->target) {
            response[1] = 0;
        } else if (number > room->target) {
            response[1] = 2;
        } else {
            response[1] = 1;
        }
        for (i = 0; i < room->playerList->cur; i++) {
            Writen(room->playerList->players[i]->id, response, 10);
        }

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200;

        Select(0, NULL, NULL, NULL, &timeout);
        if (response[1] == 1) {
            // Send end game message
            room->timeLeft = 5;
            sendGameEndMessage(room, sockfd);
        } else {
            room->timeLeft = 30;
            sendChangeTurnMessage(room);
        }

    }
}

/**
 * 
 * @param sockfd
 * @param message
 * @param n
 */
void processLeaveRoomMessage(int sockfd, char * message, int n) {
    int roomId = 0;

    memcpy(&roomId, message, 4);

    Room room = roomList[roomId - 1];

    Player player = getPlayerById(room->playerList, sockfd);
    Player currentPlayer = getCurrentPlayerFromRoom(room);

    if (player != NULL) {
        sendPlayerLeaveRoomMessage(player, room);
        
        removePlayerFromRoomById(room, sockfd);

        // If the player left the room is currently in his turn. Change turn.
        if (currentPlayer->id == player->id) {
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 200;

            Select(0, NULL, NULL, NULL, &timeout);

            sendChangeTurnMessage(room);
        }
    }
}

/**
 * Broadcast player left room to all players in room
 * Format:
 *                      bytes   Description
 *                        0      ID_LEAVE_ROOM
 *                       1-4     playerId
 * @param sockfd
 * @param player
 * @param room
 */
void sendPlayerLeaveRoomMessage(Player player, Room room) {
    int i = 0;

    char message[5];
    message[0] = ID_LEAVE_ROOM;
    memcpy(message + 1, &player->id, 4);

    for (i = 0; i < room->playerList->cur; i++) {
        Writen(room->playerList->players[i]->id, message, 5);
    }
}

/**
 * Send player joined room message to all players in a room
 * Format
 *                      bytes   Description
 *                        0      ID_GAME_CHANGE_TURN
 *                       1-4     new player index
 * @param sockfd
 * @param player
 * @param room
 */
void sendPlayerJoinedRoomMessage(Player player, Room room) {
    int i = 0;
    char playerInfoMes[63];

    playerInfoMes[0] = ID_PLAYER_JOINED_ROOM;
    memcpy(playerInfoMes + 1, &player->id, 4);
    memcpy(playerInfoMes + 5, &player->state, 4);
    memcpy(playerInfoMes + 9, &player->lives, 4);
    memcpy(playerInfoMes + 13, player->name, 50);
    for (i = 0; i < room->playerList->cur; i++) {
        Writen(room->playerList->players[i]->id, playerInfoMes, 63);
    }
}

/**
 * Send change turn message to all players in a room
 * @param sockfd
 * @param room
 */
void sendChangeTurnMessage(Room room) {
    char message[5];
    int i = 0;

    message[0] = ID_GAME_CHANGE_TURN;
    memcpy(message + 1, &room->currentPlayerIndex, 4);

    for (i = 0; i < room->playerList->cur; i++) {
        Writen(room->playerList->players[i]->id, message, 5);
    }
}

/**
 * Broadcast end game message 
 * Format:
 *                      bytes   Description
 *                        0      ID_GAME_END
 *                       1-4     playerId = Winner
 * @param room
 * @param playerId
 */
void sendGameEndMessage(Room room, int playerId) {
    room->state = 0;
    
    char message[5];

    message[0] = ID_GAME_END;
    memcpy(message + 1, &playerId, 4);

    int i = 0;
    for (i = 0; i < room->playerList->cur; i++) {
        Writen(room->playerList->players[i]->id, message, 5);
    }
}