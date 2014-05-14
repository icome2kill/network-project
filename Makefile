CC = gcc
CFLAGS = -Iunpv13e/unpv13e/lib -O2 -D_REENTRANT -Wall -g
LIBS = unpv13e/unpv13e/libunp.a -lpthread

all:	utils.o player.o player_list.o room.o hw_client hw_server

hw_client:	hw_client.o
	${CC} ${CFLAGS} hw_client.o utils.o player.o player_list.o room.o -o hw_client ${LIBS}

hw_server:	hw_server.o
	${CC} ${CFLAGS} hw_server.o utils.o player.o player_list.o room.o -o hw_server ${LIBS}

hw_client.o:	hw_client.c
	${CC} ${CFLAGS} -c hw_client.c ${LIBS}

hw_server.o:	hw_server.c
	${CC} ${CFLAGS} -c hw_server.c ${LIBS}

room.o:	    room.c
	${CC} ${CFLAGS} -c room.c player_list.c player.c

player.o:	player.c
	${CC} ${CFLAGS} -c player.c
	
player_list.o:	player_list.c
	${CC} ${CFLAGS} -c player_list.c player.c
utils.o:	utils.c
	${CC} ${CFLAGS} -c utils.c
	
clean:
	rm -rf *.o hw_client hw_server