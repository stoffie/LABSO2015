// Damiano Stoffie 166312, Anatoliy Babushka 165958, gioco multiplayer, 2015

#include "game.h"

int main (int argc, char **argv)
{
    int c;
    // default: numero massimo di giocatori
    int max = 5;
    // default: punteggio per la vittoria
    int win = 10;
    // utilizzato per controllare se sono stati specificati parametri opzionali
    // dal chiamante
    bool bmax = false;
    bool bwin = false;

    // specifico quali sono i parametri in input, opzionali
    struct option long_options[] =
    {
        {"max", required_argument, 0, 'm'},
        {"win", required_argument, 0, 'w'},
        {0, 0, 0, 0}
    };

    // getopt ci obbliga a parsare i parametri opzionali, prima di quelli
    // obbligatori!
    while (1)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long (argc, argv, "",
                         long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }

        switch (c)
        {
            // l'utente ha specificato un valore massimo
            case 'm':
                printf ("option --max with value `%s'\n", optarg);
                max = atoi(optarg);
                bmax = true;
                // verifico che il paramentro sia entro i limiti consentiti
                if (max < 2 || max > 10) {
                    puts("Incorrect value for --max flag");
                    exit(1);
                }
                break;

            // specificato valore per la vittoria
            case 'w':
                printf ("option --win with value `%s'\n", optarg);
                win = atoi(optarg);
                bwin = true;
                // verifico che il paramentro sia entro i limiti consentiti
                if (win < 10 || win > 100) {
                    puts("Incorrect value for --min flag");
                    exit(1);
                }
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort ();
        }
    }

    // verifico che l'utente abbia specificato il parametro obbligatorio
    if (optind != (argc - 1)) {
        puts("Incorrect number of agruments");
        exit(1);
    }
    // lancio modalità server o client a seconda del parametro specificato
    if (strcmp(argv[optind], "server") == 0) {
        puts("Starting server mode");
        run_server(max, win);
    }
    else if (strcmp(argv[optind], "client") == 0) {
        // l'utente non deve specificare parametro opzionali in modalità client
        if (bmax || bwin) {
            puts("You can not specify optional params in client mode");
            exit(1);
        }
        puts("Starting client mode");
        run_client();
    }
    // la stringa non è corretta!
    else {
        puts("You must specify if you want to run server or client mode");
        exit(1);
    }

    return 0;
}
