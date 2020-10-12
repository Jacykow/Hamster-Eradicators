#include "main.h"
#include "watek_mayor.h"

int primes[MAX_SIZE] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};

void gnomeMainLoop()
{
    //debug("PREP");
    changeStateGnome(PREP);
    srandom(rank);
    changeTs(0);
    while (1)
    {
        changeTs(ts + 1);
        switch (state_g)
        {
        case PREP:
            debug("WAIT waiting for mayor init");
            sendPacket(NULL, ROOT, MSG_DONE);
            changeResponseC(0);
            changeStateGnome(WAIT);
            break;
        case WAIT:
            if (responseC >= 3)
            {
                debug("READ_COM starting to read comissions");
                pthread_mutex_lock(&warehouseMut);
                comission_k = 0;
                pthread_mutex_unlock(&warehouseMut);
                changeStateGnome(READ_COM);
            }
            break;
        case READ_COM:
            if (comission_k >= comissionsC)
            {
                debug("WAIT waiting for new comissions");
                sendPacket(NULL, ROOT, MSG_DONE);
                changeResponseC(0);
                changeStateGnome(WAIT);
            }
            else
            {
                pthread_mutex_lock(&warehouseMut);
                comission_id = (comission_k * primes[rank]) % comissionsC;
                comission_k = comission_k + 1;
                pthread_mutex_unlock(&warehouseMut);

                //debug("WAIT_COM trying to get comission %d", comission_id);
                resetGnomes();
                changeStateGnome(WAIT_COM);
                message msg = {.thing = 1, .parameter = comission_id};
                for (int x = 1; x < size; x++)
                {
                    if (x == rank)
                        continue;
                    sendPacket(&msg, x, MSG_REQ);
                }
            }
            break;
        case WAIT_COM:;
            int all_ack = 1;
            for (int x = 1; x < size; x++)
            {
                if (x == rank)
                    continue;
                if (gnomes[x] == 0)
                {
                    all_ack = 0;
                    break;
                }
            }
            if (all_ack == 1)
            {
                changeTs(0);
                //debug("CRIT_COM chosen comission %d", comission_id);
                message msg = {.thing = 1, .parameter = comission_id};
                sendPacket(&msg, ROOT, MSG_COM);
                changeStateGnome(CRIT_COM);
            }
            break;
        case CRIT_COM:
            break;
        case WAIT_PIN:;
            int ack = 0, reject = 0;
            for (int x = 1; x < size; x++)
            {
                if (x == rank)
                    continue;
                if (gnomes[x] == 1)
                    ack = ack + 1;
                else if (gnomes[x] == -1)
                    reject = reject + 1;
            }
            if (pinC - (size - 2 - ack) > 0)
            {
                changeTs(0);
                debug("WAIT_POISON asking for poison for comission %d with %d hamsters", comission_id, comission_hamsters);
                resetGnomes();
                changeStateGnome(WAIT_POISON);
                message msg = {.thing = 3, .parameter = comission_hamsters};
                for (int x = 1; x < size; x++)
                {
                    if (x == rank)
                        continue;
                    sendPacket(&msg, x, MSG_REQ);
                }
            }
            else if (reject >= pinC)
            {
                //debug("REST_PIN not enough pins in the warehouse for comission %d", comission_id);
                changeStateGnome(REST_PIN);
            }
            break;
        case REST_PIN:
            //debug("WAIT_PIN asking again for pin for comission %d", comission_id);
            resetGnomes();
            changeStateGnome(WAIT_PIN);
            message msg2 = {.thing = 2, .parameter = 1};
            for (int x = 1; x < size; x++)
            {
                if (x == rank)
                    continue;
                sendPacket(&msg2, x, MSG_REQ);
            }
            break;
        case WAIT_POISON:;
            int ack_or_reject = 0, p_reject = 0;
            for (int x = 1; x < size; x++)
            {
                if (x == rank)
                    continue;
                if (gnomes[x] != 0)
                    ack_or_reject = ack_or_reject + 1;
                if (gnomes[x] < 0)
                    p_reject = p_reject - gnomes[x];
            }
            if (p_reject + comission_hamsters > poisonC)
            {
                //debug("REST_POISON not enough poison in the warehouse for comission %d with %d hamsters", comission_id, comission_hamsters);
                changeStateGnome(REST_POISON);
            }
            else if (ack_or_reject >= size - 2)
            {
                changeTs(0);
                //debug("CRIT_POISON finishing comission %d with %d hamsters", comission_id, comission_hamsters);
                changeStateGnome(CRIT_POISON);
            }
            break;
        case REST_POISON:
            //debug("WAIT_POISON asking again for poison for comission %d with %d hamsters", comission_id, comission_hamsters);
            resetGnomes();
            changeStateGnome(WAIT_POISON);
            message msg = {.thing = 3, .parameter = comission_hamsters};
            for (int x = 1; x < size; x++)
            {
                if (x == rank)
                    continue;
                sendPacket(&msg, x, MSG_REQ);
            }
            break;
        case CRIT_POISON:
            debug("READ_COM finished comission %d", comission_id);
            changeStateGnome(READ_COM);
            break;
        default:
            break;
        }
        sleep(SEC_IN_STATE);
    }
}

void *gnomeComLoop(void *ptr)
{
    MPI_Status status;
    message msg;
    while (1)
    {
        MPI_Recv(&msg, 1, MPI_MESSAGE_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG)
        {
        case MSG_REQ:
            //debug("%d REQ do %d o %d", status.MPI_SOURCE, rank, msg.parameter);
            if (state_g == WAIT || state_g == READ_COM)
            {
                sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
            }
            else if (state_g == WAIT_COM)
            {
                if (msg.thing == 1 && msg.parameter == comission_id)
                {
                    if (ts > msg.ts || (ts == msg.ts && rank < status.MPI_SOURCE) || gnomes[status.MPI_SOURCE] == 1)
                    {
                        sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                    }
                    else
                    {
                        sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
                    }
                }
                else
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
                }
            }
            else if (state_g == CRIT_COM)
            {
                if (msg.thing == 1 && msg.parameter == comission_id)
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                }
                else
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
                }
            }
            else if (state_g == WAIT_PIN || state_g == REST_PIN)
            {
                if (msg.thing == 1 && msg.parameter == comission_id)
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                }
                else if (msg.thing == 2)
                {
                    if (ts > msg.ts || (ts == msg.ts && rank < status.MPI_SOURCE) || gnomes[status.MPI_SOURCE] == 1)
                    {
                        sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                    }
                    else
                    {
                        sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
                    }
                }
                else
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
                }
            }
            else if (state_g == WAIT_POISON || state_g == REST_POISON)
            {
                if (msg.thing == 1 && msg.parameter == comission_id)
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                }
                else if (msg.thing == 2)
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                }
                else if (msg.thing == 3)
                {
                    if (ts > msg.ts || (ts == msg.ts && rank < status.MPI_SOURCE) || gnomes[status.MPI_SOURCE] == 1)
                    {
                        msg.parameter = comission_hamsters;
                        sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                    }
                    else
                    {
                        sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
                    }
                }
                else
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
                }
            }
            else if (state_g == CRIT_POISON)
            {
                if (msg.thing == 1 && msg.parameter == comission_id)
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                }
                else if (msg.thing == 2)
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                }
                else if (msg.thing == 3)
                {
                    msg.parameter = comission_hamsters;
                    sendPacket(&msg, status.MPI_SOURCE, MSG_REJECT);
                }
                else
                {
                    sendPacket(&msg, status.MPI_SOURCE, MSG_ACK);
                }
            }
            break;
        case MSG_ACK:
            if (state_g == WAIT_COM)
            {
                if (msg.thing == 1 && msg.parameter == comission_id)
                {
                    pthread_mutex_lock(&responseCMut);
                    gnomes[status.MPI_SOURCE] = 1;
                    pthread_mutex_unlock(&responseCMut);
                }
            }
            if (state_g == CRIT_COM)
            {
                if (status.MPI_SOURCE == ROOT)
                {
                    comission_hamsters = msg.parameter;
                    debug("WAIT_PIN asking for pin for comission %d", comission_id);
                    resetGnomes();
                    changeStateGnome(WAIT_PIN);
                    message msg = {.thing = 2, .parameter = 1};
                    for (int x = 1; x < size; x++)
                    {
                        if (x == rank)
                            continue;
                        sendPacket(&msg, x, MSG_REQ);
                    }
                }
            }
            if (state_g == WAIT_PIN)
            {
                pthread_mutex_lock(&responseCMut);
                gnomes[status.MPI_SOURCE] = 1;
                pthread_mutex_unlock(&responseCMut);
            }
            if (state_g == WAIT_POISON)
            {
                pthread_mutex_lock(&responseCMut);
                gnomes[status.MPI_SOURCE] = 1;
                pthread_mutex_unlock(&responseCMut);
            }
            break;
        case MSG_REJECT:
            if (state_g == WAIT_COM && msg.thing == 1 && msg.parameter == comission_id)
            {
                //debug("READ_COM gnome %d rejected comission %d", status.MPI_SOURCE, comission_id);
                changeStateGnome(READ_COM);
            }
            if (state_g == CRIT_COM && msg.thing == 1)
            {
                //debug("READ_COM mayor rejected comission %d", comission_id);
                changeStateGnome(READ_COM);
            }
            if (state_g == WAIT_PIN && msg.thing == 2)
            {
                pthread_mutex_lock(&responseCMut);
                gnomes[status.MPI_SOURCE] = -1;
                pthread_mutex_unlock(&responseCMut);
            }
            if (state_g == WAIT_POISON && msg.thing == 3)
            {
                pthread_mutex_lock(&responseCMut);
                gnomes[status.MPI_SOURCE] = -msg.parameter;
                pthread_mutex_unlock(&responseCMut);
            }
            break;
        case MSG_COM:
            if (state_g == WAIT)
            {
                pthread_mutex_lock(&warehouseMut);
                if (msg.thing == 1)
                    comissionsC = msg.parameter;
                if (msg.thing == 2)
                    pinC = msg.parameter;
                if (msg.thing == 3)
                    poisonC = msg.parameter;
                pthread_mutex_unlock(&warehouseMut);
                changeResponseC(responseC + 1);
            }
            break;
        case MSG_DONE:
            break;
        default:
            break;
        }
    }
}
