#include	"unp.h"
#include        "constant.h"
#include        "time.h"
#include        "utils.h"
#include        "room.h"

void process(FILE*, int);
void createMenu();
void login(int, char*);
void logout(int);

void guess(int, int);
void listPlayer(int);
void listRoom(int);
void joinRoom(int);
void leaveRoom(int);

void processMessage(char*, int);
void processListPlayerMessage(char*, int);
void processListRoomMessage(char*, int);
void processJoinRoomMessage(char*, int);
void processGameStartMessage(char *, int);
void processListRoomPlayerMessage(char *, int);
void processPlayerJoinedRoomMessage(char *, int);
void processGuessNumberMessage(char *, int);
void processChangeTurnMessage(char *, int);
void processLeaveRoomMessage(char *, int);
void processGameEndMessage(char *, int);

Player currentPlayer = NULL;
Room currentRoom = NULL;
fd_set allset;

int main(int argc, char** argv) {
    int sockfd = 0, choice = 0, n = 0;
    char playerName[50];
    struct sockaddr_in servaddr;
    char buf[MAXLINE];
    char *server_ip = "127.0.0.1";
		int server_port = SERV_PORT;
    
    if (argc > 3) {
        printf("Usage: hw_client + <IP_address(default is 127.0.0.1) <Port>>.\n");
        exit(1);
    }
    
    if (argc >= 2) {
        server_ip = argv[1];
    }
if (argc == 3) {
		server_port = atoi(argv[2]);
}

    int maxfd = 0;
    int flag = 0;
    int nready = 0;

    fd_set rset;

    FD_ZERO(&allset);
    FD_SET(STDIN_FILENO, &allset);

    maxfd = STDIN_FILENO + 1;
    
    struct timeval interval;
    interval.tv_sec = 1;
    interval.tv_usec = 0;

    while (1) {
        createMenu();

        while (1) {
            rset = allset;
            if (currentPlayer == NULL || currentPlayer->state != 2) {
                // Not playing, dont need to count anything
                nready = Select(maxfd, &rset, NULL, NULL, NULL);
            }
            else {
                nready = Select(maxfd, &rset, NULL, NULL, &interval);
            }

            if (FD_ISSET(STDIN_FILENO, &rset)) {
                // Data from user input
                if (Readline(STDIN_FILENO, buf, MAXLINE) != 0) {
                    flag = 1;
                    choice = atoi(buf);

                    if (currentPlayer == NULL) {
                        switch (choice) {
                            case 1:
                            {
                                sockfd = Socket(AF_INET, SOCK_STREAM, 0);

                                bzero(&servaddr, sizeof (servaddr));
                                servaddr.sin_family = AF_INET;
                                servaddr.sin_port = htons(server_port);
                                Inet_pton(AF_INET, server_ip, &servaddr.sin_addr);

                                Connect(sockfd, (SA *) & servaddr, sizeof (servaddr));

                                printf("Enter your name: ");
                                scanf("%s", playerName);
                                login(sockfd, playerName);

                                FD_SET(sockfd, &allset);
                                maxfd = max(sockfd, STDIN_FILENO) + 1;

                                break;
                            }
                            case 2:
                            {
                                freePlayer(currentPlayer);
                                currentPlayer = NULL;
                                exit(0);
                                break;
                            }
                        }
                    } else if (currentPlayer->state == 2) {
                        if (buf[0] == 'q') {
                            leaveRoom(sockfd);
                        } else if (currentPlayer->id != getCurrentPlayerFromRoom(currentRoom)->id) {
                            printf("It's not your turn.\n");
                        } else {
                            guess(sockfd, choice);
                        }
                    } else {
                        switch (choice) {
                            case 1:
                                listPlayer(sockfd);
                                break;
                            case 2:
                                listRoom(sockfd);
                                break;
                            case 3:
                                if (currentPlayer -> state == 0) {
                                    joinRoom(sockfd);
                                } else {
                                    leaveRoom(sockfd);
                                }
                                break;
                            case 4:
                                if (currentPlayer -> state == 0) 
                                    logout(sockfd);
                                break;
                            case 5:
                                if (currentPlayer -> state == 0) {
                                    logout(sockfd);
                                    exit(1);
                                }
                                break;
                        }
                    }

                    if (--nready <= 0) {
                        break;
                    }
                }
            }

            if (FD_ISSET(sockfd, &rset)) {
                // Data from server
                flag = 1;
                if ((n = read(sockfd, buf, MAXLINE)) == 0) {
                    printf("Server is down :(\n");
                    FD_CLR(sockfd, &allset);
                    Close(sockfd);
                    if (currentPlayer != NULL) {
                        freePlayer(currentPlayer);
                    }
                    if (currentRoom != NULL) {
                        freeRoom(currentRoom);
                    }
                } else {
                    processMessage(buf, n);
                }
                if (--nready <= 0) {
                    break;
                }
            }
            
            if (interval.tv_sec == 0 && interval.tv_usec == 0) {
                // 1 second has passed
                interval.tv_sec = 1;
                interval.tv_usec = 0;
                
                // Again check for player state for sure.
                if (currentPlayer->state == 2) {
                    printf("\r%d sec(s) remains", currentRoom->timeLeft--);
										fflush(stdout);
                }
            }
        }
    }

}

/**
 * Create appropriate menu
 */
void createMenu() {
    if (currentPlayer == NULL) {
        // Not logged in
        printf("Menu\n");
        printf("1. Login\n");
        printf("2. Exit\n");
    } else if (currentPlayer->state == 2) {
        // Playing
    } else {
        printf("Menu\n");
        printf("1. List player\n");
        printf("2. List room\n");
        if (currentPlayer->state == 0) {
            printf("3. Join room\n");
            printf("4. Logout\n");
            printf("5. Exit\n");
        } else {
            printf("3. Leave room\n");
        }
    }
}

/**
 * Process message from server
 * @param buf
 * @param n
 */
void processMessage(char *message, int n) {
    if (message[0] == ID_LOGIN) {
        // Do nothing
    } else if (message[0] == ID_LOGOUT) {

    } else if (message[0] == ID_LIST_PLAYER) {
        processListPlayerMessage(message + 1, n - 1);
    } else if (message[0] == ID_LIST_ROOM) {
        processListRoomMessage(message + 1, n - 1);
    } else if (message[0] == ID_JOIN_ROOM) {
        processJoinRoomMessage(message + 1, n - 1);
    } else if (message[0] == ID_GAME_START) {
        processGameStartMessage(message + 1, n - 1);
    } else if (message[0] == ID_GUESS_NUMBER) {
        processGuessNumberMessage(message + 1, n - 1);
    } else if (message[0] == ID_PLAYER_JOINED_ROOM) {
        processPlayerJoinedRoomMessage(message + 1, n - 1);
    } else if (message[0] == ID_GAME_CHANGE_TURN) {
        processChangeTurnMessage(message + 1, n - 1);
    } else if (message[0] == ID_LEAVE_ROOM) {
        processLeaveRoomMessage(message + 1, n - 1);
    } else if (message[0] == ID_GAME_END) {
        processGameEndMessage(message + 1, n - 1);
    }
}

void processGameEndMessage(char * message, int n) {
    int playerId = 0;
    
    memcpy(&playerId, message, 4);
    
    Player player = getPlayerById(currentRoom->playerList, playerId);
    
    if (playerId == currentPlayer->id) {
        printf("You has won the game. Again, congratulation.");
    } else if (player != NULL) {
        printf("%s has won the game.", player->name);
    }
    
    if (currentRoom->playerList->cur == currentRoom->playerList->size) {
        printf(" Waiting 5s to start a new game\n");
    } else {
        printf(" Not enough player to start new game.\n");
        currentPlayer->state = 1; // Change player state to waiting.
    }
}

/**
 * 
 * @param message
 * @param n
 */
void processLeaveRoomMessage(char * message, int n) {
    int playerId = 0;
    
    memcpy(&playerId, message, 4);
    
    if (currentPlayer->id == playerId) {
        // I left the room.
        printf("You has left the room\n");
        
    } else {
        Player player = getPlayerById(currentRoom->playerList, playerId);
        printf("%s has left the room.\n", player->name);
        
        removePlayerFromRoomById(currentRoom, playerId);
    }
}

/**
 * 
 * @param message
 * @param n
 */
void processChangeTurnMessage(char * message, int n) {
    memcpy(&currentRoom->currentPlayerIndex, message, 4);
    
    currentRoom->timeLeft = 30;
    if (getCurrentPlayerFromRoom(currentRoom)->id == currentPlayer->id) {
        printf("It's your turn. Make a guess. You have 30s.\n");
    }
    else {
        printf("It's %s turn.\n", getCurrentPlayerFromRoom(currentRoom)->name);
    }
}

void processPlayerJoinedRoomMessage(char * message, int n) {
    char name[50];
    Player player = newPlayer(0, 0, 0, name);
    
    memcpy(&player->id, message, 4);
    memcpy(&player->state, message + 4, 4);
    memcpy(&player->lives, message + 8, 4);
    memcpy(&player->name, message + 12, 50);
    
    printf("%s has joined the game.\n", player->name);
    
    addPlayerToRoom(currentRoom, player);
}

/**
 * 
 * @param message
 * @param n
 */
void processGameStartMessage(char * message, int n) {
    printf("The game has started.(Enter q to exit room)\n");
    currentPlayer->state = 2;

    memcpy(&(currentRoom->currentPlayerIndex), message, 4);
    memcpy(&(currentRoom->timeLeft), message + 4, 4);
    
    if (currentPlayer->id == getCurrentPlayerFromRoom(currentRoom)->id) {
        // My turn...
        printf("It's your turn. You'll have %d seconds to make a guess\n", currentRoom->timeLeft);
    } else {
        printf("It's %s turn.\n", getCurrentPlayerFromRoom(currentRoom)->name);
    }
}

/**
 * 
 * @param message
 * @param n
 */
void processGuessNumberMessage(char * message, int n) {
    int result = message[0];
    int playerId = 0;
    int number = 0;
    
    memcpy(&playerId, message + 1, 4);
    memcpy(&number, message + 5, 4);
    
    char * name = "You";
    if (playerId != currentPlayer->id) {
        name = getPlayerById(currentRoom->playerList, playerId)->name;
    }
    
    if (result == 0) {
        printf("%s guessed %d. And it was too low\n", name, number);
    }
    else if (result == 2) {
        printf("%s guessed %d. And it was too high\n", name, number);
    }
    else {
        printf("%s guessed %d. And it was correct. Congratulation\n", name, number);
    }
}

/**
 * Process room list result from server
 * @param message
 * @param n
 */
void processListRoomMessage(char * message, int n) {
    int roomId = 0;
    int numberOfPlayer = 0;
    int size = 0;
    int state = 0;
    char name[50];

    int roomInfoSize = 0;
    int i = 0;
    int count = 0;

    memcpy(&roomInfoSize, message, 4);

    count = (n - 4) / roomInfoSize;

    printf("Room list: \n");
    printf("ID\tName\tSize\tState\n");

    for (i = 0; i < count; i++) {
        memcpy(&roomId, message + 4 + i * 66, 4);
        memcpy(&numberOfPlayer, message + 8 + i * 66, 4);
        memcpy(&size, message + 12 + i * 66, 4);
        memcpy(&state, message + 16 + i * 66, 4);

        memcpy(name, message + 20 + i * 66, 50);

        if (state == 0) {
            printf("%d\t%s\t%d/%d\tWaiting\n", roomId, name, numberOfPlayer, size);
        } else {
            printf("%d\t%s\t%d/%d\tPlaying\n", roomId, name, numberOfPlayer, size);
        }
    }
}

/**
 * Process player list result from server
 * @param message
 * @param n
 */
void processListPlayerMessage(char * message, int n) {
    int id = 0;
    int state = 0;
    int lives = 0;
    char name[50];
    int playerSize = 0, i = 0, count = 0;

    memcpy(&playerSize, message, 4);
    count = (n - 4) / playerSize;

    printf("Player List: \n");
    printf("ID\tName\tState\n");

    for (i = 0; i < count; i++) {
        memcpy(&id, message + 4 + i * playerSize, 4);
        memcpy(&state, message + 8 + i * playerSize, 4);
        memcpy(&lives, message + 12 + i * playerSize, 4);
        memcpy(name, message + 16 + i * playerSize, 50);

        printf("%d\t%s\t%d\n", id, name, state);
    }
}

/**
 * Process join room response from server
 * @param message
 * @param n
 */
void processJoinRoomMessage(char * message, int n) {
    char status = message[0];

    switch (status) {
        case 0:
            printf("Unknown Error\n");
            break;
        case 1:
            printf("Room is playing\n");
            break;
        case 2:
            printf("Room is full\n");
            break;
        case 3:
            printf("Success\n");
            
            int roomId = 0;
            int size = 0;
            char name[50];
            
            memcpy(&roomId, message + 1, 4);
            memcpy(&size, message + 5, 4);
            memcpy(name, message + 9, 50);
            
            currentPlayer->state = 1;
            currentRoom = newRoom(roomId, name, size);
            processListRoomPlayerMessage(message + 59, n - 59);
            break;
        default:
            printf("WTF\n");
            break;
    }
}

void processListRoomPlayerMessage(char * message, int n) {
    int id = 0;
    int state = 0;
    int lives = 0;
    char name[50];
    int playerSize = 0, i = 0, count = 0;

    memcpy(&playerSize, message, 4);
    count = (n - 4) / playerSize;

    printf("Current player in room: \n");
    
    for (i = 0; i < count; i++) {
        memcpy(&id, message + 4 + i * playerSize, 4);
        memcpy(&state, message + 8 + i * playerSize, 4);
        memcpy(&lives, message + 12 + i * playerSize, 4);
        memcpy(name, message + 16 + i * playerSize, 50);

        addPlayerToRoom(currentRoom, newPlayer(id, state, lives, name));
        printf("%d\t%s\n", id, name);
    }
}

/**
 * Send login message to server
 * Format:
 *		bytes	description
 *		 0	 message id
 *		 1-50	 player name
 * @param sockfd
 * @param name
 */
void login(int sockfd, char* name) {
    char message[52];
    char response[MAXLINE];

    snprintf(message, 52, "%c%s", ID_LOGIN, name);

    Writen(sockfd, message, 52);

    // Wait for server response. Timeout 30s
    fd_set rset;
    FD_ZERO(&rset);

    FD_SET(sockfd, &rset);

    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    Select(sockfd + 1, &rset, NULL, NULL, &timeout);

    if (FD_ISSET(sockfd, &rset)) {
        if (read(sockfd, response, MAXLINE) == 0) {
            printf("Server shutdown\n");
        } else {
            if (response[1] == 1) {
                printf("Successfully logged in. Welcome %s\n", name);

                int id = 0;
                memcpy(&id, response + 2, 4);

                currentPlayer = newPlayer(id, 0, 0, name);
            } else {
                printf("Maximum number of player reached\n");
                Close(sockfd);
            }
        }
        return;
    }

    printf("Request timed out\n");
}

/**
 * Send logout message
 * Format:  
 *              bytes   description
 *               0       ID_LOGOUT
 * @param sockfd
 */
void logout(int sockfd) {
    char message[2];

    sprintf(message, "%c", ID_LOGOUT);

    Writen(sockfd, message, 2);

    currentPlayer = NULL;
    FD_CLR(sockfd, &allset);
}

/**
 * Send list player request to server
 * @param sockfd
 */
void listPlayer(int sockfd) {
    char message[2];

    sprintf(message, "%c", ID_LIST_PLAYER);

    Writen(sockfd, message, 2);
}

/**
 * Send list room request to server
 * @param sockfd
 */
void listRoom(int sockfd) {
    char message[2];
    sprintf(message, "%c", ID_LIST_ROOM);

    Writen(sockfd, message, 2);
}

/**
 * Send join room request to server
 * @param sockfd
 */
void joinRoom(int sockfd) {
    int roomId;
    printf("Enter a room id\n");
    scanf("%d", &roomId);

    char message[5];
    message[0] = ID_JOIN_ROOM;

    memcpy(message + 1, &roomId, 4);

    Writen(sockfd, message, 5);
}

/**
 * Send guess request to server
 * @param sockfd
 * @param number
 */
void guess(int sockfd, int number) {
    char message[9];
    
    message[0] = ID_GUESS_NUMBER;
    memcpy(message + 1, &currentRoom->id, 4);
    memcpy(message + 5, &number, 4);
    
    // Interpolation
    currentRoom->currentPlayerIndex = (currentRoom->currentPlayerIndex + 1) % currentRoom->playerList->cur;
    
    Writen(sockfd, message, 9);
}

/**
 * Send leave room message to server
 * @param sockfd
 */
void leaveRoom(int sockfd) {
    char message[5];
    
    message[0] = ID_LEAVE_ROOM;
    memcpy(message + 1, &currentRoom->id, 4);
    
    currentPlayer->state = 0;
    
    Writen(sockfd, message, 5);
}
