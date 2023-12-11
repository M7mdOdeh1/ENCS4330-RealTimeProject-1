/*
cahirer.c

*/

#include "local3.h"

struct ALL_CASHIERS *memptr_cashiers;
int cashier_index;


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <cashier_id>\n", argv[0]);
        exit(1);
    }


    key_t key_cashier = atoi(argv[1]);
    cashier_index = atoi(argv[2]);

    printf("Cashier key in cashier.c: %d\n", key_cashier);

    // connect to the shared memory segment for the cashiers
    int shmid_cashiers = shmget(key_cashier, 0, 0);
    if (shmid_cashiers == -1) {
        perror("shmget for all cashiers failed in cashier.c");
        exit(EXIT_FAILURE);
    }

    // attach to the shared memory segment
    char *shmptr_cashier = (char *) shmat(shmid_cashiers, (char)0, 0);
    if (shmptr_cashier== (char *) -1) {
        perror("shmat -- cashier -- attach");
        exit(1);
    }
    
    // access the cashiers shared memory
    memptr_cashiers = (struct ALL_CASHIERS *) shmptr_cashier;

    printf("Cashier %d has id %d\n", cashier_index, memptr_cashiers->cashiers[cashier_index].id);

    // signal handler for SIGUSR1
    if (signal (SIGUSR1, catchSIGUSR1) == SIG_ERR) {
        perror("signal -- cashier -- SIGUSR1");
        exit(SIGUSR1);
    }

    while (1)

        pause();


}

void catchSIGUSR1(int sig_num) {
    printf("Cashier %d received SIGUSR1\n", getpid());


    // keep serveing the customers until the queue is empty
    while (memptr_cashiers->cashiers[cashier_index].numCustomers > 0) {
        // serve the customer in the head of the queue
        int j = memptr_cashiers->cashiers[cashier_index].head;

        // send SIGUSR1 to the customer to indicate  he customer that it's turn
        if(kill(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].customerPID, SIGUSR1) == -1) {
            perror("kill -- cashier -- SIGUSR1");
            exit(SIGUSR1);
        }

        for (int i = 0; i < memptr_cashiers->cashiers[cashier_index].cartsQueue[j].numItems; i++) {
            printf("Cashier %d is scanning item %s\n", getpid(), memptr_cashiers->cashiers[cashier_index].cartsQueue[j].items[i][0].str);
            // update the quantity of the item in the cart
            memptr_cashiers->cashiers[cashier_index].cartsQueue[j].quantityOfItems--;
            memptr_cashiers->cashiers[cashier_index].numItemsInCarts--;

            // sleep for the scan time of the item
            sleep(memptr_cashiers->cashiers[cashier_index].scanTime);
        }
        // send SIGUSR1 to the customer to indicate that the cashier is done
        kill(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].customerPID, SIGUSR1);
        // update the number of customers in the line
        memptr_cashiers->cashiers[cashier_index].numCustomers--;

        



        

    }

    



    



    
    


    exit(0);
}

