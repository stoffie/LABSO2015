// Damiano Stoffie 166312, Anatoliy Babushka 165958, gioco multiplayer, 2015

#include "game.h"

// identifica un client all'interno del codice del server
// questa struttura viene impiegata per costruire una linked list
typedef struct connected_client connected_client;
typedef struct connected_client
{
    int pid; // il pid del client
    int points; // il suo punteggio
    int fd; // fifo in scrittura associata
    connected_client * next;
    connected_client * prev;
}
connected_client;

// stato del server
typedef struct server_ctx
{
    // file descriptor usato in lettura
    int in_fd;
    // numero di client connessi
    int nclients;
    // n1 e n2 sono i due numeri casuali, n3 è la loro somma
    int n1;
    int n2;
    int n3;
    // numero massimo di clients
    int max;
    // punteggio massimo per la vittoria
    int win;
    // i clients connessi
    connected_client * client;
}
server_ctx;

// trova un client in base al suo pid
connected_client * find_client (server_ctx * ctx, int pid)
{
    connected_client * c = ctx->client;
    while (c != NULL) {
        if (c->pid == pid) {
            return c;
        }
        c = c->next;
    }
    // Il client richiesto non è stato trovato
    // non deve essere necessariamente gestito come errore fatale
    puts("Fatal: client was not found");
    abort();
    return NULL;
}

// variabile globale che contiene lo stato del server, usata dalla successiva
// funzione
server_ctx * global_ctx;

// rilascia le risorse precedentemente iniziailizzate
void server_interupt_handler (int signal)
{
    int j;
    connected_client * c;
    // ignora tutti i segnali eccetto SIGINT
    if (signal != SIGINT) {
        return;
    }
    puts("");
    puts("Shutting down");
    // avvisa tutti i client connessi
    server_msg msg;
    msg.type = server_msg_termination;
    c = global_ctx->client;
    while (c != NULL) {
        safe_write(c->fd, &msg, sizeof(server_msg));
        c = c->next;
    }
    // rilascia le risorse in uso
    close(global_ctx->in_fd);
    remove_pid("game.pid");
    unlink("server.fifo");
    
    exit(0);
}

// salva un client all'interno di una linked list
void insert_client (server_ctx * ctx, connected_client * c)
{
    // alloca lo spazio per il nuovo client sullo heap
    connected_client * heap_c = malloc(sizeof(connected_client));
    // copia il conenuto dallo stack
    memcpy(heap_c, c, sizeof(connected_client));

    if (ctx->client == NULL) {
        // non vi è alcun client
        ctx->client = heap_c;
        heap_c->prev = NULL;
        heap_c->next = NULL;
        return;
    }
    c = ctx->client;
    if (heap_c->points > c->points) {
        // il nuovo client ha il punteggio più alto
        ctx->client = heap_c;
        heap_c->prev = NULL;
        heap_c->next = c;
        c->prev = heap_c;
        return;
    }
    // scorre tutti i client esistenti
    while (c->next != NULL) {
        if (c->points >= heap_c->points && heap_c->points > c->next->points) {
            heap_c->next = c->next;
            c->next->prev = heap_c;
            heap_c->prev = c;
            c->next = heap_c;
        }
        c = c->next;
    }
    // il nuovo client ha il punteggio più basso
    c->next = heap_c;
    heap_c->prev = c;
    heap_c->next = NULL;
}

// rimuove un nodo dalla linked list
// rilascia la memoria
void remove_client (server_ctx * ctx, connected_client * c)
{
    if (c->next == NULL && c->prev == NULL) {
        ctx->client = NULL;
    } else if (c->next == NULL) {
        c->prev->next = NULL;
    } else if (c->prev == NULL) {
        c->next->prev = NULL;
        ctx->client = c->next;
    } else {
        c->prev->next = c->next;
        c->next->prev = c->prev;
    }
    free(c);
}

// scambia la posizione dei due nodi
void swap_nodes (server_ctx * ctx, connected_client * prev, connected_client * next)
{
    connected_client tmp;
    tmp.next = next->next;
    tmp.prev = prev->prev;
    next->next = prev;
    prev->prev = next;
    next->prev = tmp.prev;
    prev->next = tmp.next;
    // controllo se è stato scambiato il nodo iniziale
    if (tmp.prev == NULL) {
        ctx->client = next;
    }
}

// vettore dei client connessi e dimensione del vettore
void handle_connection (server_ctx * ctx, client_msg * msg)
{
    // messaggio di risposta del server
    server_msg reply;
    // file descriptor usato per la risposta
    int fd;
    // contiene la informazioni relativa a un client
    connected_client c;
    connected_client * cp;

    printf("Client with pid %i is connecting\n", msg->pid);
    if (ctx->nclients < ctx->max) {
        // è possibile accettare il client
        puts("Accepting connection");
        // informa tutti gli altri client connessi
        cp = ctx->client;
        reply.type = server_msg_other_accepted;
        reply.pid = msg->pid;
        while (cp != NULL) {
            safe_write(cp->fd, &reply, sizeof(server_msg));
            cp = cp->next;
        }
        // icrementa il numero di client connessi
        ctx->nclients++;
        // costruisce il messaggio di risposta
        reply.type = server_msg_accepted;
        reply.points = ctx->max - ctx->nclients;
        reply.n1 = ctx->n1;
        reply.n2 = ctx->n2;
        // concatena il pid del client con una stringa per ottenere il nome
        // della fifo da utilizzare in scrittura
        char filename[64];
        snprintf(filename, sizeof(char) * 64, "client.fifo.%i", msg->pid);        
        // salva le informazioni inviate dal client
        c.pid = msg->pid;
        c.points = ctx->max - ctx->nclients;
        c.fd = open(filename, O_WRONLY);
        insert_client(ctx, &c);
        // inoltra il messaggio di risposta al client
        safe_write(c.fd, &reply, sizeof(server_msg));
    } else {
        // non ci sono posti disponibili
        puts("Refusing connection");
        // costruisce un messaggio di risposta
        reply.type = server_msg_rejected;
        // concatena il pid del client con una stringa per ottenere il nome
        // della fifo da utilizzare in scrittura        
        char filename[64];
        snprintf(filename, sizeof(char) * 64, "client.fifo.%i", msg->pid);
        // inoltra il messaggio e chiude immediatamente la fifo
        fd = open(filename, O_WRONLY);
        safe_write(fd, &reply, sizeof(server_msg));
        close(fd);
    }
}

void handle_answer (server_ctx * ctx, client_msg * msg)
{
    // messaggio di risposta del server
    server_msg reply;
    // posizione in cui si trova il client
    int i;
    // indice ustao per scorrere tutti i clients
    int j;

    connected_client * c;
    connected_client * c2;
    
    printf("Client with pid %i is answering\n", msg->pid);
    
    reply.type = server_msg_reply;
    
    if (msg->answer == ctx->n3) {
        // la rispota è corretta
        // trova il client con il pid specificato dal messaggio
        c = find_client(ctx, msg->pid);
        // costruisce il messaggio di risposta
        reply.correct = true;
        // invia il messaggio di risposta
        safe_write(c->fd, &reply, sizeof(server_msg));
        // incrementa il punteggio del client
        c->points++;
        // verifica se è necessario muovere il client nella classifica
        if (c->prev != NULL && c->prev->points < c->points) {
            swap_nodes(ctx, c->prev, c);
        }
        
        printf("Answer is correct, client has now %i points\n", c->points);
        if (c->points == ctx->win) {
            // punteggio massimo raggiunto
            printf("Client %i has won! :)\n", msg->pid);
            // invia la classifica ad ogni giocatore
            c = ctx->client;
            reply.type = server_msg_rank;
            while (c != NULL) {
                reply.pid = c->pid;
                reply.points = c->points;
                c2 = ctx->client;
                while (c2 != NULL) {
                    safe_write(c2->fd, &reply, sizeof(server_msg));
                    c2 = c2->next;
                }
                c = c->next;
            }
            // costruisce un addizionale messaggio di risposta
            reply.type = server_msg_end;
            reply.winner_pid = msg->pid;
            // scorre ogni possibile client connesso
            c = ctx->client;
            while (c != NULL) {
                safe_write(c->fd, &reply, sizeof(server_msg));
                c = c->next;
            }
            printf("Good! My work here is done. Goodbye\n");
            // rilascia le risorse acquisite
            close(global_ctx->in_fd);
            remove_pid("game.pid");
            unlink("server.fifo");
            exit(0);
        } else {
            // genera una nuova domanda
            ctx->n1 = rand() % 100;
            ctx->n2 = rand() % 100;
            ctx->n3 = ctx->n1 + ctx->n2;
            // costruisce un nuovo messaggio
            reply.type = server_msg_question;
            reply.n1 = ctx->n1;
            reply.n2 = ctx->n2;
            // scorre ogni possibile client connesso
            c = ctx->client;
            while (c != NULL) {
                safe_write(c->fd, &reply, sizeof(server_msg));
                c = c->next;
            }
        }
    } else {
        // risposta sbagliata
        c = find_client(ctx, msg->pid);
        // costruisce il messaggio di risposta
        reply.correct = false;
        // inoltra il messaggio
        safe_write(c->fd, &reply, sizeof(server_msg));
        // decrementa il punteggio del client. Eventualmente è possibile fare un
        // controllo per non permettere punteggi negativi
        c->points--;
        printf("Answer is incorrect, client has now %i points\n", ctx->client[i].points);
        // verifica se è necessario muovere il client nella classifica
        if (c->next != NULL && c->next->points > c->points) {
            swap_nodes(ctx, c, c->next);
        }
    }
    // stampa la classifica
    c = ctx->client;
    puts("Client, Points");
    while (c != NULL) {
        printf("%i, %i\n", c->pid, c->points);
        c = c->next;
    }
}

// Gestisce la disconnessione di un client
void handle_termination (server_ctx * ctx, client_msg * msg)
{
    connected_client * c;
    printf("Client %i is terminating\n", msg->pid);
    c = find_client(ctx, msg->pid);
    close(c->fd);
    ctx->nclients--;
    remove_client(ctx, c);
}

// inizializza le risorse impiegate dal server
void run_server (int max, int win)
{
    // stato del server
    server_ctx ctx;
    // messaggio inviato dal client al server   
    client_msg msg;
    // true quando il gioco è terminato
    bool endgame = false;
    
    // inizializza lo stato del server
    ctx.client = NULL;
    ctx.max = max;
    ctx.win = win;
    ctx.nclients = 0;
    // inizializza il generatore di numeri casuali e la prima domanda
    srand(time(NULL));
    ctx.n1 = rand() % 100;
    ctx.n2 = rand() % 100;
    ctx.n3 = ctx.n1 + ctx.n2;
    
    // FIXME: è consentito usare un file preso da internet?
    // la funzione check_pid ci fornisce un controllo più solido rispetto al
    // controllare semplicemente se esiste una fifo del server
    
    // controlla che non ci sia già un server in esecuzione
    if (check_pid("game.pid")) {
        puts("Server already running");
        exit(1);
    }
    // segnala il server come in esecuzione
    if (write_pid("game.pid") == 0) {
        puts("Could not write game.pid");
        exit(1);
    }
    // inizializza la fifo in lettura
    puts("Registering server fifo");
    mkfifo("server.fifo", 0666);
    
    // permette l'accesso allo stato del server da una variabile globale
    global_ctx = &ctx;
    // evento lanciato se il programma è interrotto
    signal(SIGINT, server_interupt_handler);
    
    puts("Waiting for clients to connect");
    // O_RDONLY causa un malfunzionamento (?)
    // Quando la pipe è vuota, le successive read non saranno bloccanti!
    // O_RDWR non funziona con cygwin (?)
    ctx.in_fd = open("server.fifo", O_RDWR);
    // loop fino a quando il gioco non è terminato
    while (!endgame) {
        // la chiamata a 'read' è bloccante!
        safe_read(ctx.in_fd, &msg, sizeof(client_msg));
        if (msg.type == client_msg_connect) {
            // un client ha richiesto di connettersi
            handle_connection(&ctx, &msg);
        } else if (msg.type == client_msg_answer) {
            handle_answer(&ctx, &msg);
        } else if (msg.type == client_msg_termination) {
            handle_termination(&ctx, &msg);
        }  else {
            puts("Unexpected type of message");
            abort();
        }
    }
}

