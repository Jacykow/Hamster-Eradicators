#include "main.h"
#include "watek_mayor.h"

void mayorMainLoop()
{
    int loop_count = 1;
    //debug("INIT");
    changeTs(0);
    srandom(time(NULL));
    while (1)
    {
        changeTs(ts + 1);
        switch (state_m)
        {
        case INIT:
            if (responseC >= size - 1)
            {
                changeStateMayor(CRIT);
            }
            break;
        case REST:;
            int all_done = 1;
            for (int x = 1; x < size; x++)
            {
                if (gnomes[x] == 0)
                {
                    all_done = 0;
                    break;
                }
            }
            if (all_done == 1)
            {
                loop_count = loop_count - 1;
                if (loop_count == 0)
                {
                    return;
                }
                changeStateMayor(CRIT);
            }
            break;
        case CRIT:
            pthread_mutex_lock(&warehouseMut);
            comissionsC = rand() % (MAX_COMISSIONS - MIN_COMISSIONS) + MIN_COMISSIONS;
            for (int x = 0; x < comissionsC; x++)
            {
                comissions[x] = rand() % (MAX_HAMSTERS - MIN_HAMSTERS) + MIN_HAMSTERS;
            }
            pinC = rand() % (MAX_PINS - MIN_PINS) + MIN_PINS;
            poisonC = rand() % (MAX_POISON - MIN_POISON) + MIN_POISON;
            pthread_mutex_unlock(&warehouseMut);

            debug("REST Generated a warehouse with %d comissions, %d pins, %d poison", comissionsC, pinC, poisonC);
            resetGnomes();
            message msg = {.thing = 1, .parameter = comissionsC};
            for (int x = 1; x < size; x++)
            {
                sendPacket(&msg, x, MSG_COM);
            }
            msg.thing = 2;
            msg.parameter = pinC;
            for (int x = 1; x < size; x++)
            {
                sendPacket(&msg, x, MSG_COM);
            }
            msg.thing = 3;
            msg.parameter = poisonC;
            for (int x = 1; x < size; x++)
            {
                sendPacket(&msg, x, MSG_COM);
            }
            changeStateMayor(REST);
            break;
        default:
            break;
        }
        sleep(SEC_IN_STATE);
    }
}

void *mayorComLoop(void *ptr)
{
    MPI_Status status;
    message msg;
    while (1)
    {
        MPI_Recv(&msg, 1, MPI_MESSAGE_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch (status.MPI_TAG)
        {
        case MSG_REQ:
            break;
        case MSG_ACK:
            break;
        case MSG_REJECT:
            break;
        case MSG_COM:
            if (state_m == REST)
            {
                if (msg.thing == 1)
                {
                    if (comissions[msg.parameter] == 0)
                    {
                        sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                    }
                    else
                    {
                        int hamsters = comissions[msg.parameter];
                        pthread_mutex_lock(&warehouseMut);
                        comissions[msg.parameter] = 0;
                        pthread_mutex_unlock(&warehouseMut);
                        msg.parameter = hamsters;
                        sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
                    }
                }
            }
            break;
        case MSG_DONE:
            if (state_m == INIT)
            {
                changeResponseC(responseC + 1);
            }
            if (state_m == REST)
            {
                pthread_mutex_lock(&responseCMut);
                gnomes[status.MPI_SOURCE] = 1;
                pthread_mutex_unlock(&responseCMut);
            }
            break;
        default:
            break;
        }
    }
}
