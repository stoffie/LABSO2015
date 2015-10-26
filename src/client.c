// Damiano Stoffie 166312, Anatoliy Babushka 165958, gioco multiplayer, 2015

#include "game.h"

// thread di lettura dell'input utente
void* client_thread (void* arg)
{
    client_msg msg;
    int pid = getpid();
    // recupera il parametro passato dal chiamante
    int out_fd = *((int *) arg);
    
    while (true) {
        // legge l'input dell'utente
        char buf[64];
        if (fgets(buf, sizeof(buf), stdin) != NULL) { 
            int i = atoi(buf);
            // costruisce un messaggio di risposta
            msg.type = client_msg_answer;
            msg.pid = pid;
            msg.answer = i;
            // inoltra il messaggio
            safe_write(out_fd, &msg, sizeof(client_msg));
        }
    }

    return NULL;
}

// nomde del file in scrittura
char filename[64];
// fd in scrittura
int out_fd;

// rilascia le risorse precedentemente iniziailizzate
void client_interupt_handler (int signal)
{
    int j;
    // ignora tutti i segnali eccetto SIGINT
    if (signal != SIGINT) {
        return;
    }
    puts("");
    puts("Shutting down");
    // avvisa il server
    client_msg msg;
    msg.type = client_msg_termination;
    msg.pid = getpid();
    safe_write(out_fd, &msg, sizeof(client_msg));
    
    // rilascia esplicitamente le risorse utilizzate per scrivere verso il
    // server
    close(out_fd);
    unlink(filename);
    // le altre risorse sono rilasciate automaticamente dal sistema operativo
    exit(0);
}

void run_client ()
{
    // tiene traccia di quanti punti ha il client
    int points;
    // fd in lettura
    int in_fd;
    
    int pid = getpid();
    // messaggio inviato dal client al server
    client_msg msg;
    // messaggio di risposta inviato dal server al client
    server_msg reply;
    
    printf("Hello, my pid is %i\n", pid);

    // verifica che il server sia effettivamente in esecuzione
    if (check_pid("game.pid") == 0) {
        puts("Server is not running");
        abort();
    }
    
    // produce il nome della fifo concatenando una stringa con il pid del client
    snprintf(filename, sizeof(char) * 64, "client.fifo.%i", pid);
    // permessi normali
    mkfifo(filename, 0666);
    
    // evento lanciato se il programma è interrotto
    signal(SIGINT, client_interupt_handler);
    
    // informa il server della presenza del client
    puts("Informing server of my presence");
    msg.type = client_msg_connect;
    msg.pid = pid;
    out_fd = open("server.fifo", O_WRONLY);
    safe_write(out_fd, &msg, sizeof(client_msg));
    
    // leggo il messaggio di risposta
    in_fd = open(filename, O_RDWR);
    safe_read(in_fd, &reply, sizeof(server_msg));
    if (reply.type == server_msg_accepted) {
        // il server accetta la nostra richiesta
        puts("Server has accepted our request!");
        printf("Initial points: %i\n", reply.points);
        points = reply.points;
        printf("Question: %i + %i\n", reply.n1, reply.n2);
#ifdef GAME_TEST
		// rispondo in maniera automatica
		puts("Automatic response");
		msg.type = client_msg_answer;
		msg.pid = getpid();
		msg.answer = reply.n1 + reply.n2;
		safe_write(out_fd, &msg, sizeof(client_msg));
#endif
    } else if (reply.type == server_msg_rejected) {
        // richiesta negata
        puts("Server rejected our connection request!");
        puts("Shutting down :(");
        exit(0);
    } else {
        // messaggio sconosciuto
        puts("Unknown message recived");
        abort();
    }

#ifndef GAME_TEST 
    // lancio il thread di lettura dell'input utente   
    pthread_t t;
    pthread_create (&t, NULL, &client_thread, &out_fd);
#endif
    
    // leggo i messaggi del server e inoltro le informazioni all'utente
    while (true) {
        safe_read(in_fd, &reply, sizeof(server_msg));
        if (reply.type == server_msg_question) {
            // nuova domanda
            printf("Question: %i + %i\n", reply.n1, reply.n2);
#ifdef GAME_TEST
			// rispondo in maniera automatica
			puts("Automatic response");
			msg.type == client_msg_answer;
			msg.pid = getpid();
			msg.answer = reply.n1 + reply.n2;
			safe_write(out_fd, &msg, sizeof(client_msg));
#endif
        } else if (reply.type == server_msg_reply) {
            // esito della risposta
            printf("Answer is: %s\n", reply.correct ? "correct" : "wrong");
            points += (reply.correct ? +1 : -1);
            printf("Points: %i\n", points);
        } else if (reply.type == server_msg_termination) {
            // il server è stato terminato
            puts("Server is terminating, quitting client");
            // dealloca risorse
            close(out_fd);
            unlink(filename);            
            exit(0);
        } else if (reply.type == server_msg_other_accepted) {
            // un altro client si sta connettendo
            printf("Client %i is connecting\n", msg.pid);
        } else if (reply.type == server_msg_rank) {
            // ricevo la classifica
            puts("Final ranking:\npid, points");
            printf("%i, %i\n", reply.pid, reply.points);
            while (true) {
                // continuo a leggere la classifica
                safe_read(in_fd, &reply, sizeof(server_msg));
                if (reply.type == server_msg_rank) {
                    printf("%i, %i\n", reply.pid, reply.points);
                } else if (reply.type == server_msg_end) {
                    // fine del gioco
                    printf("Game ended, winner is %i\n", reply.winner_pid);
                    if (reply.winner_pid == pid) {
                        puts("You are the winner! Congratulations!");
                    }
                    // dealloca risorse
                    close(out_fd);
                    unlink(filename);            
                    exit(0);
                } else {
                    puts("Unexpected type of message");
                    abort();
                }
            }
        } else {
            puts("Unexpected type of message");
            abort();
        }
    }
}
