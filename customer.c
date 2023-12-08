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


    if ( argc != 5 ) {
        fprintf(stderr, "Usage: %s message\n", *argv);
    	exit(-1);
     }

    printf("Customer %d is shopping for %d seconds\n", cartID, waitTime);
    sleep(buyTime);

    



    int shmid;
    char *shmptr;
    struct MEMORY *memptr;
    if ( (shmid = shmget((int) parentPid, 0, 0)) != -1 ) {
        if ( (shmptr = (char *) shmat(shmid, (char *)0, 0)) == (char *) -1 ) {
            perror("shmat -- consumer -- attach");
            exit(1);
        }
        memptr = (struct MEMORY *) shmptr;
        // print the items
        for (int i = 0; i < memptr->numItems; i++) {
            printf("%s %d %f\n", memptr->items[i].name, memptr->items[i].inventory, memptr->items[i].price);
        }

    }
    else {
        perror("shmget -- consumer -- access");
        exit(2);
    }

    





}