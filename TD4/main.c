#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// https://p-fb.net/master1/parapp/fiches/ParApp_TD_4_2024_2025.pdf
// https://p-fb.net/master1/parapp/

void mastercode(int n, int m)
{
    int i;
    long long answer;
    int who, nprocs;
    int task[2];
    MPI_Status status;
    long long sum = 0;
    int answers_to_receive, received_answers;
    int *counts;
    int whomax, num;

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    answers_to_receive = (n - 1) / m + 1;
    received_answers = 0;
    num = 1;

    /* send tasks to workers */
    whomax = nprocs - 1;
    if (whomax > answers_to_receive)
        whomax = answers_to_receive;
    for (who = 1; who <= whomax; who++)
    {
        task[0] = num;
        task[1] = num + m - 1;
        if (task[1] > n)
            task[1] = n;
        MPI_Send(&task[0], 2, MPI_INT, /* sending two ints */
                 who,                  /* to the lucky one */
                 1,                    /* tag */
                 MPI_COMM_WORLD);      /* communicator */
        num += m;
    }
    while (received_answers < answers_to_receive)
    {                            /* wait for an answer from a worker. */
        MPI_Recv(&answer,        /* address of receive buffer */
                 1,              /* number of items to receive */
                 MPI_LONG_LONG,  /* type of data */
                 MPI_ANY_SOURCE, /* can receive from any other */
                 1,              /* tag */
                 MPI_COMM_WORLD, /* communicator */
                 &status);       /* status */

        who = status.MPI_SOURCE; /* find out who sent us the answer */
        sum += answer;           /* update the sum */
        received_answers++;      /* and the number of received answers */

        /* put the worker on work, but only if not all tasks have been sent.
         * we use the value of num to detect this */
        if (num <= n)
        {
            task[0] = num;
            task[1] = num + m - 1;
            if (task[1] > n)
                task[1] = n;
            MPI_Send(&task[0], 2, MPI_INT, /* sending two ints */
                     who,                  /* to the lucky one */
                     1,                    /* tag */
                     MPI_COMM_WORLD);      /* communicator */
            num += m;
        }
    }
    /* Now master sends a message to the workers to signify that they should
     * end the calculations. We use a special tag for that:
     */
    counts = (int *)malloc(sizeof(int) * (nprocs - 1));
    for (who = 1; who < nprocs; who++)
    {
        MPI_Send(&task[0], 1, MPI_INT, /* sending one int
                                        * it is permitted to send a shorter message than will be received.
                                        * The other case:
                                        * sending a longer message than the receiver expects is not allowed. */
                 who,                  /* to who */
                 2,                    /* tag */
                 MPI_COMM_WORLD);      /* communicator */

        /* the worker will send to master the number of calculations
         * that have been performed. We put this number in the counts array.
         */
        MPI_Recv(&counts[who - 1], /* address of receive buffer */
                 1,                /* number of items to receive */
                 MPI_INT,          /* type of data */
                 who,              /* receive from process who */
                 7,                /* tag */
                 MPI_COMM_WORLD,   /* communicator */
                 &status);         /* status */
    }
    printf("The sum of the integers from 1..%d is %lld\n", n, sum);
    printf("The work was divided using %d summations per worker\n", m);
    printf(" WORKER calculations\n\n");
    for (i = 1; i < nprocs; i++)
        printf("%6d %8d\n", i, counts[i - 1]);
}

void workercode()
{
    int i, rank;
    long long answer;
    int task[2];
    MPI_Status status;
    int count = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* worker first enters 'waiting for message' state
     */

    MPI_Recv(&task[0],       /* address of receive buffer */
             2,              /* number of items to receive */
             MPI_INT,        /* type of data */
             0,              /* can receive from master only */
             MPI_ANY_TAG,    /* can expect two values, so
                              * we use the wildcard MPI_ANY_TAG
                              * here */
             MPI_COMM_WORLD, /* communicator */
             &status);       /* status */

    /* if tag equals 2, then skip the calculations */

    if (status.MPI_TAG != 2)
    {
        while (1)
        {
            answer = 0;

            for (i = task[0]; i <= task[1]; i++)
                answer += i;
            count++;
            MPI_Send(&answer, 1,       /* sending one int */
                     MPI_LONG_LONG, 0, /* to master */
                     1,                /* tag */
                     MPI_COMM_WORLD);  /* communicator */

            MPI_Recv(&task[0],       /* address of receive buffer */
                     2,              /* number of items to receive */
                     MPI_INT,        /* type of data */
                     0,              /* can receive from master only */
                     MPI_ANY_TAG,    /* can expect two values, so
                                      * we use the wildcard MPI_ANY_TAG
                                      * here */
                     MPI_COMM_WORLD, /* communicator */
                     &status);       /* status */

            if (status.MPI_TAG == 2) /* leave this loop if tag equals 2 */
                break;
        }
    }

    /* this is the point that is reached when a task is received with
     * tag = 2 */

    /* send the number of calculations to master and return */

    MPI_Send(&count, 1, MPI_INT, /* sending one int */
             0,                  /* to master */
             7,                  /* tag */
             MPI_COMM_WORLD);    /* communicator */
}




int main(int argc, char *argv[])
{
    int rank, nprocs;

    /* MPI INIT
    cette fonction permet de « connecter » le processus à la machine parallèle et de
    définir les paramètres de l’application parallèle, en particulier le nombre de nœuds utilisés ;
    Ce doit être la première instruction à exécuter
    */
    MPI_Init(&argc, &argv);

    /* MPI COMM RANK
    MPI_Comm_rank : cette fonction retourne le numéro du « processeur » courant au sein de
    l’application parallèle, ce qui permet d’en tenir compte dans l’algorithme ;  
    */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* MPI COMM SIZE
    cette fonction retourne le nombre de processeurs/processus utilisés dans
    la machine parallèle ;
    */
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc != 3)
    {
        if (rank == 0)
        {
            printf("Please call this program with to arguments: N and M\n");
            printf("The program will than calculate sum(1..N)\n");
            printf("Each worker will calculate the sum of M numbers\n");
        }

        /*
        Termine l'application parallèle
        */
        MPI_Finalize();
        return 1;
    }

    if (rank == 0)
    {
        int n, m;
        n = atoi(argv[1]);
        m = atoi(argv[2]);
        mastercode(n, m);
    }
    else
        workercode();
    /*
        Termine l'application parallèle
    */
    MPI_Finalize();
    return 0;
}