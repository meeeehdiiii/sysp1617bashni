CFLAGS = -Wpedantic -Wextra -Wall -Werror -g

bashni: bashni.o performConnection.o config.o
	gcc -o bashni bashni.o performConnection.o config.o $(CFLAGS)

ifndef $(PLAYER)
play: ./bashni
	./bashni $(GAME_ID) $(PLAYER)
else
play: ./bashni
	./bashni $(GAME_ID)
endif


bashni.o: bashni.c
	gcc -c bashni.c $(CFLAGS)

performConnection.o: performConnection.c
	gcc -c performConnection.c $(CFLAGS)

config.o: config.c
	gcc -c config.c $(CFLAGS)

clean:
	rm -f *.o play bashni
