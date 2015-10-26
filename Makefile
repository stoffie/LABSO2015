# Damiano Stoffie 166312, Anatoliy Babushka 165958, gioco multiplayer, 2015

notarget:
	@echo Progetto sistemi operativi 2015
	@echo Gioco multiplayer
	@echo Damiano Stoffie 166312, Anatoliy Babushka 165958

# Ogni file oggetto dipende dal corrispettivo file sorgente e dal file di header
# comune
%.o: src/%.c src/game.h
	gcc -c $^

# il file binario dipende da ogni file oggetto
bin/multiplayer: client.o common.o main.o pidfile.o server.o
	gcc -pthread client.o common.o main.o pidfile.o server.o -o bin/multiplayer

# file oggetto specifico con client automatizzao
test_client.o: src/client.c src/game.h
	gcc -D GAME_TEST -c src/client.c -o test_client.o

# file binario con testing automatizzato
test_multiplayer: test_client.o common.o main.o pidfile.o server.o
	gcc -pthread test_client.o common.o main.o pidfile.o server.o -o test_multiplayer

# il task bin dipende dal file bin/multiplayer
bin: bin/multiplayer

# cancella ogni file generato da make
clean:
	rm -f *.o
	rm -f test_multiplayer
	rm -f bin/multiplayer

assets:
	@echo Non disponibile

# esegue test automatizzato, dipende dal file binario test_multiplayer
test: test_multiplayer
	./test_multiplayer server &
	sleep 1
	./test_multiplayer client
	@echo Test successful
