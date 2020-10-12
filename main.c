#include "main.h"
#include "watek_gnome.h"
#include "watek_mayor.h"
#include <pthread.h>

volatile char end = FALSE;
int size, rank;
pthread_t threadKom;
int comissionsC;
int pinC;
int poisonC;
int responseC;
int gnomes[MAX_SIZE];
int ts;

state_mayor state_m = INIT;
int comissions[MAX_COMISSIONS];

state_gnome state_g = PREP;
int comission_id;
int comission_k;
int comission_hamsters;

pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tsMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t responseCMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t warehouseMut = PTHREAD_MUTEX_INITIALIZER;

void check_thread_support(int provided)
{
    //printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided)
    {
    case MPI_THREAD_SINGLE:
        printf("Brak wsparcia dla wątków, kończę\n");
        /* Nie ma co, trzeba wychodzić */
        fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
        MPI_Finalize();
        exit(-1);
        break;
    case MPI_THREAD_FUNNELED:
        printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
        break;
    case MPI_THREAD_SERIALIZED:
        /* Potrzebne zamki wokół wywołań biblioteki MPI */
        printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
        break;
    case MPI_THREAD_MULTIPLE:
        //printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
        break;
    default:
        printf("Nikt nic nie wie\n");
    }
}

void inicjuj(int *argc, char ***argv)
{
    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    const int nitems = 3;
    int blocklengths[3] = {1, 1, 1};
    MPI_Datatype typy[3] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[3];
    offsets[0] = offsetof(message, ts);
    offsets[1] = offsetof(message, thing);
    offsets[2] = offsetof(message, parameter);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_MESSAGE_T);
    MPI_Type_commit(&MPI_MESSAGE_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);

    if (rank == ROOT)
    {
        changeResponseC(0);
        changeStateMayor(INIT);
        pthread_create(&threadKom, NULL, *mayorComLoop, 0);
        mayorMainLoop();
    }
    else
    {
        pthread_create(&threadKom, NULL, *gnomeComLoop, 0);
        gnomeMainLoop();
    }
}

void finalizuj()
{
    pthread_mutex_destroy(&stateMut);
    println("czekam na wątek \"komunikacyjny\"\n");
    pthread_join(threadKom, NULL);
    MPI_Type_free(&MPI_MESSAGE_T);
    MPI_Finalize();
    println("koniec\n");
}

void sendPacket(message *pkt, int destination, int tag)
{
    int freepkt = 0;
    if (pkt == 0)
    {
        pkt = malloc(sizeof(message));
        freepkt = 1;
    }
    pkt->ts = ts;
    MPI_Send(pkt, 1, MPI_MESSAGE_T, destination, tag, MPI_COMM_WORLD);
    if (freepkt)
        free(pkt);
}

void changeStateMayor(state_mayor newState)
{
    pthread_mutex_lock(&stateMut);
    state_m = newState;
    pthread_mutex_unlock(&stateMut);
}

void changeStateGnome(state_gnome newState)
{
    pthread_mutex_lock(&stateMut);
    state_g = newState;
    pthread_mutex_unlock(&stateMut);
}

void changeTs(int newTs)
{
    pthread_mutex_lock(&tsMut);
    ts = newTs;
    pthread_mutex_unlock(&tsMut);
}

void changeResponseC(int newResponseC)
{
    pthread_mutex_lock(&responseCMut);
    responseC = newResponseC;
    pthread_mutex_unlock(&responseCMut);
}

void resetGnomes()
{
    pthread_mutex_lock(&responseCMut);
    for (int x = 1; x < size; x++)
    {
        gnomes[x] = 0;
    }
    gnomes[rank] = 1;
    pthread_mutex_unlock(&responseCMut);
}

int main(int argc, char **argv)
{
    inicjuj(&argc, &argv);
    finalizuj();
    return 0;
}
