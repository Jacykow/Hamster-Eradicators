#ifndef GLOBALH
#define GLOBALH

#define DEBUG

#define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define TRUE 1
#define FALSE 0

#define SEC_IN_STATE 1

#define ROOT 0

typedef enum
{
    INIT,
    REST,
    CRIT
} state_mayor;

typedef enum
{
    PREP,
    WAIT,
    READ_COM,
    WAIT_COM,
    CRIT_COM,
    WAIT_PIN,
    REST_PIN,
    WAIT_POISON,
    REST_POISON,
    CRIT_POISON
} state_gnome;

MPI_Datatype MPI_MESSAGE_T;
typedef struct
{
    int ts;
    int thing;
    int parameter;
} message;
extern MPI_Datatype MPI_PAKIET_T;

#define MAX_SIZE 10
#define MIN_COMISSIONS 40
#define MAX_COMISSIONS 60
#define MIN_HAMSTERS 5
#define MAX_HAMSTERS 15
#define MIN_PINS 3
#define MAX_PINS 6
#define MIN_POISON 15
#define MAX_POISON 25

#define MSG_REQ 101
#define MSG_ACK 102
#define MSG_REJECT 103
#define MSG_COM 104
#define MSG_DONE 105

#ifdef DEBUG
#define debug(FORMAT, ...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, ##__VA_ARGS__, 27, 0, 37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm", 27, 1, 37);
#define P_BLACK printf("%c[%d;%dm", 27, 1, 30);
#define P_RED printf("%c[%d;%dm", 27, 1, 31);
#define P_GREEN printf("%c[%d;%dm", 27, 1, 33);
#define P_BLUE printf("%c[%d;%dm", 27, 1, 34);
#define P_MAGENTA printf("%c[%d;%dm", 27, 1, 35);
#define P_CYAN printf("%c[%d;%d;%dm", 27, 1, 36);
#define P_SET(X) printf("%c[%d;%dm", 27, 1, 31 + (6 + X) % 7);
#define P_CLR printf("%c[%d;%dm", 27, 0, 37);

/* printf ale z kolorkami i automatycznym wyświetlaniem RANK. Patrz debug wyżej po szczegóły, jak działa ustawianie kolorków */
#define println(FORMAT, ...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, ##__VA_ARGS__, 27, 0, 37);

extern int rank;
extern int size;
extern int comissionsC;
extern int pinC;
extern int poisonC;
extern pthread_mutex_t responseCMut;
extern int responseC;
extern int gnomes[];
extern int ts;

extern state_mayor state_m;
extern pthread_mutex_t warehouseMut;
extern int comissions[];

extern state_gnome state_g;
extern int comission_id;
extern int comission_k;
extern int comission_hamsters;

/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void sendPacket(message *pkt, int destination, int tag);
void changeStateMayor(state_mayor);
void changeStateGnome(state_gnome);
void changeTs(int);
void changeResponseC(int);
void resetGnomes();
#endif
