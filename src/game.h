// Damiano Stoffie 166312, Anatoliy Babushka 165958, gioco multiplayer, 2015

#ifndef _GAME_H
#define _GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/file.h>

// messaggio inviato dal client al server
// tipo di messaggio
typedef enum client_msg_e
{
    client_msg_connect,
    client_msg_answer,
    client_msg_termination
}
client_msg_e;
// struttura messaggio
typedef struct client_msg
{
    client_msg_e type;
    int pid;
    int answer;
}
client_msg;
// se il messaggio è di tipo client_msg_connect o client_msg_disconnect, il pid
// deve essere specificato, se il messaggio è di tipo client_msg_answer, pid e
// answer devono essere specificati.

// messaggio inviato dal server al client
// tipo di messaggio
typedef enum server_msg_e
{
    server_msg_accepted, // connessione accettata
    server_msg_other_accepted, // altro client accettato
    server_msg_rejected, // connessione rifiutata
    server_msg_question, // nuova domanda
    server_msg_reply, // risposta
    server_msg_rank, // classifica
    server_msg_end, // fine del gioco
    server_msg_termination // chiusura immediata del server
}
server_msg_e;
// struttura messaggio
typedef struct server_msg
{
    server_msg_e type;
    int points;
    int n1;
    int n2;
    bool correct;
    int winner_pid;
    int pid;
}
server_msg;
// accepted -> points, n1, n2
// rejected ->
// question -> n1, n2
// reply -> correct, points
// end -> winner_pid

// esegue il server
void run_server (int max, int win);
// esegue il client
void run_client ();

// documentate in pidfile.c
int read_pid (char *pidfile);
int check_pid (char *pidfile);
int write_pid (char *pidfile);
int remove_pid (char *pidfile);

// normali operazioni di lettura o scrittura, abortiscono il programma in caso
// di errore
void safe_write(int fd, void * buffer, int size);
void safe_read(int fd, void * buffer, int size);

#endif
