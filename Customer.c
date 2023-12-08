/*
Customer.c
*/

#include "local3.h"


int main(int argc, char *argv[])
{
    pid_t parentPid = atoi(argv[1]);
    int cartID = atoi(argv[2]);
    int buyTime = atoi(argv[3]);
    int waitTime = atoi(argv[4]);


    if ( argc != 4 ) {
    	fprintf(stderr, "Usage: %s message\n", *argv);
    	exit(-1);
     }

    printf("Customer %d is waiting for %d seconds\n", cartID, waitTime);
    sleep(buyTime);

    // access shared memory
    


    int shmid;
    char *shmptr, *memptr;
    if ( (shmid = shmget((int) parentPid, 0, 0)) != -1 ) {
        if ( (shmptr = (char *) shmat(shmid, (char *)0, 0)) == (char *) -1 ) {
            perror("shmat -- consumer -- attach");
            exit(1);
        }
        memptr = (struct MEMORY *) shmptr;
    }
    else {
        perror("shmget -- consumer -- access");
        exit(2);
    }






}