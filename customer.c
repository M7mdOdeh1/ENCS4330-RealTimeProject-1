/*
Customer.c
*/

#include "local3.h"
#include "local4.h"
#define _XOPEN_SOURCE 500


int isOnCashier = 0;
PositionUpdateMessage msg;
key_t keyMsgQueue = 1234;
int msgQueueId;


int randomInRange(int min_range, int max_range) {
    return (int) (min_range +  (rand() % (max_range - min_range)));
}


int main(int argc, char *argv[])
{
    pid_t parentPid = atoi(argv[1]);
    int cartID = atoi(argv[2]);
    int buyTime = atoi(argv[3]);
    int waitTime = atoi(argv[4]);
    key_t key_cashier = atoi(argv[5]);


    struct SHOPPING_CART cart;

    srand((unsigned) getpid()); // seed for the random function with the ID of the current process

    if ( argc != 6 ) {
        fprintf(stderr, "Usage: %s message\n", *argv);
    	exit(-1);
     }

    msgQueueId = msgget(keyMsgQueue, 0666 | IPC_CREAT);
    if (msgQueueId == -1) {
        perror("msgget");
        return 1;
    }
    
    msg.msgType = MSG_POS_UPDATE;
    msg.id = 10 + cartID; // if id > 10 then it is a customer
    msg.x = 0; // cashier index
    msg.y = 0; // number in the queue
    msg.state = 0; // State (0 = came new to the market)

    if (msgsnd(msgQueueId, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        return 1;
    }

    // sleep for the time the customer is shopping
    printf("Customer %d is shopping for %d seconds\n", cartID, buyTime);
    fflush(stdout);
    sleep(buyTime);

    
    // connect to the shared memory segment for the items
    int shmid_items;
    char *shmptr_items;
    struct MEMORY *memptr_items;
    // get the shared memory segment
    if ( (shmid_items = shmget((int) parentPid, 0, 0)) != -1 ) {
        // attach to the shared memory segment
        if ( (shmptr_items = (char *) shmat(shmid_items, (char *)0, 0)) == (char *) -1 ) {
            perror("shmat -- customer -- attach");
            exit(1);
        }
        memptr_items = (struct MEMORY *) shmptr_items;
        

    }
    else {
        perror("shmget -- customer -- access");
        exit(2);
    }

    /*
    Access the semaphore set
    */
    int semid;
    if ( (semid= semget((int) parentPid, 2, 0)) == -1 ) {
        perror("semget -- customer -- access");
        exit(3);
    }

    

    printf("----------------------------------------\n");
    printf("items in the market:\n");
     // print all items
    for (int i = 0; i < memptr_items->numItems; i++) {
        printf("%s %d %f\n", memptr_items->items[i].name, memptr_items->items[i].inventory, memptr_items->items[i].price);
    }

    int numItemsToBuy = randomInRange(1, memptr_items->numItems);

    // initialize the cart struct
    cart.customerPID = getpid();
    cart.numItems = 0;
    cart.quantityOfItems = 0;
    
    // array of indexs of items to buy
    int IndecisOfItemsToBuy[numItemsToBuy];

    // acquire the semaphore
    acquireSem(semid, 0);

    // choose random items with random quantities to buy
    for (int i = 0; i < numItemsToBuy; i++) {
        int indexOfItem = randomInRange(0, memptr_items->numItems - 1);

        // add the index of the item to the array
        IndecisOfItemsToBuy[i] = indexOfItem;

        if (memptr_items->items[indexOfItem].inventory > 0) {
            // Buy a random quantity of the item
            int quantity = randomInRange(1, 2 + (int) (0.01 * memptr_items->items[indexOfItem].inventory));
            // Update the inventory
            memptr_items->items[indexOfItem].inventory -= quantity;

            // Add the item to the customer's cart
            strcpy(cart.items[i][0].str, memptr_items->items[indexOfItem].name);
            sprintf(cart.items[i][1].str, "%d", quantity);
            sprintf(cart.items[i][2].str, "%f", memptr_items->items[indexOfItem].price);

            cart.numItems++;
            cart.quantityOfItems += quantity;

            
            

        }

    }
    // release the semaphore
    releaseSem(semid, 0);
   

    printf("----------------------------------------\n");

    // print the cart items
    printf("Customer %d is done shopping. The contents of the cart are:\n", cartID);
    for (int i = 0; i < cart.numItems; i++) {
        printf("%s %s %s\n", cart.items[i][0].str, cart.items[i][1].str, cart.items[i][2].str);
    }

    printf("----------------------------------------\n");
    printf("items in the market after customer %d is done shopping:\n", cartID);
    // print the items after the customer is done shopping
    for (int i = 0; i < memptr_items->numItems; i++) {
        printf("%s %d %f\n", memptr_items->items[i].name, memptr_items->items[i].inventory, memptr_items->items[i].price);
    }

    printf("----------------------------------------\n");

    // connect to the shared memory segment for the cashiers
    int shmid_cashiers = shmget(key_cashier, 0, 0);
    if (shmid_cashiers == -1) {
        perror("shmget for all cashiers failed");
        exit(EXIT_FAILURE);
    }

    // attach to the shared memory segment
    char *shmptr_cashier = (char *) shmat(shmid_cashiers, (char)0, 0);
    if (shmptr_cashier== (char *) -1) {
        perror("shmat -- customer -- attach");
        exit(1);
    }
    
    // access the cashiers shared memory
    struct ALL_CASHIERS *memptr_cashiers;
    memptr_cashiers = (struct ALL_CASHIERS *) shmptr_cashier;
    
    // acqurie the semaphore
    acquireSem(semid, 1);
    
    int bestLineIndex = 0;
    int bestLineNumItemsWithScanTime = (1+memptr_cashiers->cashiers[0].numItemsInCarts) * memptr_cashiers->cashiers[0].scanTime;
    int bestLineLength = memptr_cashiers->cashiers[0].numCustomers;
    int bestLineBehavior = memptr_cashiers->cashiers[0].behavior;

    //initially take the first active cashier as the best line
    for (int i = 0; i < memptr_cashiers->numCashiers; i++) {
        if (memptr_cashiers->cashiers[i].isActive == 1) {
            bestLineIndex = i;
            bestLineLength = memptr_cashiers->cashiers[i].numCustomers;
            bestLineBehavior = memptr_cashiers->cashiers[i].behavior;
            bestLineNumItemsWithScanTime = (1 +memptr_cashiers->cashiers[i].numItemsInCarts) * memptr_cashiers->cashiers[i].scanTime;
            break;
        }
    }
    /* Check the best line queue according to this in order:
        - The line with less 'total items' and faster scanning time
        - Shortest length
        - Best behavior
    */ 

    
    
    for (int i = 1; i < memptr_cashiers->numCashiers; i++) {

        //skip the non active cashiers
        if (memptr_cashiers->cashiers[i].isActive == 0) {
            continue;
        }
        
        int numItemWithScanTime = (1 + memptr_cashiers->cashiers[i].numItemsInCarts) * memptr_cashiers->cashiers[i].scanTime;
        // check if the current line is better than the best line
        if ( numItemWithScanTime < bestLineNumItemsWithScanTime) {
            bestLineIndex = i;
            bestLineLength = memptr_cashiers->cashiers[i].numCustomers;
            bestLineBehavior = memptr_cashiers->cashiers[i].behavior;
            bestLineNumItemsWithScanTime = numItemWithScanTime;
        }
        // if the current line has the same number of items as the best line, check the length
        else if (numItemWithScanTime == bestLineNumItemsWithScanTime) {
            if (memptr_cashiers->cashiers[i].numCustomers < bestLineLength) {
                bestLineIndex = i;
                bestLineLength = memptr_cashiers->cashiers[i].numCustomers;
                bestLineBehavior = memptr_cashiers->cashiers[i].behavior;
                bestLineNumItemsWithScanTime = (1 +memptr_cashiers->cashiers[i].numItemsInCarts) * memptr_cashiers->cashiers[i].scanTime;
            }
            // if the current line has the same number of items and the same length as the best line, check the behavior
            else if (memptr_cashiers->cashiers[i].numCustomers == bestLineLength) {
                if (memptr_cashiers->cashiers[i].behavior > bestLineBehavior) {
                    bestLineIndex = i;
                    bestLineLength = memptr_cashiers->cashiers[i].numCustomers;
                    bestLineBehavior = memptr_cashiers->cashiers[i].behavior;
                    bestLineNumItemsWithScanTime = (1 +memptr_cashiers->cashiers[i].numItemsInCarts) * memptr_cashiers->cashiers[i].scanTime; 
                }
            }
        }
    }

    // add the cart to the best line
    memptr_cashiers->cashiers[bestLineIndex].cartsQueue[memptr_cashiers->cashiers[bestLineIndex].tail] = cart;

    msg.msgType = MSG_POS_UPDATE;
    msg.id = 10 + cartID; // if id > 10 then it is a customer
    msg.x = bestLineIndex; // cashier index
    msg.y = memptr_cashiers->cashiers[bestLineIndex].numCustomers; // number in the queue
    msg.state = 1; // State (1 = in queue)
    if (msgsnd(msgQueueId, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        return 1;
    }
    
    // update the number of customers in the line
    memptr_cashiers->cashiers[bestLineIndex].numCustomers++;
    // update the number of items in the line
    memptr_cashiers->cashiers[bestLineIndex].numItemsInCarts += cart.quantityOfItems;
    // update the tail of the line
    memptr_cashiers->cashiers[bestLineIndex].tail = (memptr_cashiers->cashiers[bestLineIndex].tail + 1) % MAX_CUSTOMERS;
    
    // check if iam the first customer in the line
    if (memptr_cashiers->cashiers[bestLineIndex].numCustomers == 1) {
        printf("sending signal to cashier %d\n", memptr_cashiers->cashiers[bestLineIndex].id);
        fflush(stdout);
        // signal the cashier that there is a customer in the line
        if (kill(memptr_cashiers->cashiers[bestLineIndex].id, SIGUSR1) == SIG_ERR) {
            perror("kill -- SIGUSR1 -- Customer -- failed");
            exit(EXIT_FAILURE);
        }
    }

    // release the semaphore
    releaseSem(semid, 1);


    // signal handler for the alarm
    if ( sigset(SIGALRM, catchAlarm) == SIG_ERR) {
        perror("Sigset can not set SIGALRM -- customer");
        exit(SIGALRM);
    }

    // signal handler for the SIGINT
    if ( sigset(SIGINT, catchSIGINT) == SIG_ERR) {
        perror("Sigset can not set SIGINT -- customer");
        exit(SIGINT);
    }

    // singal user1 handler to indicate that the customer is on the cashier 
    if ( sigset(SIGUSR1, catchSIGUSR1) == SIG_ERR) {
        perror("Sigset can not set SIGUSR1 -- customer");
        exit(SIGUSR1);
    }

    // sleep for the time the customer is waiting in line
    printf("Customer %d is waiting in line %d for at maximum %d seconds\n", cartID, bestLineIndex, waitTime);
    fflush(stdout);
    alarm(waitTime); // set the alarm for the remaining time

    int quantityOfItems = cart.quantityOfItems;
    pause();

    if (!isOnCashier) {

        // acquire the semaphore
        acquireSem(semid, 1);
        int ind = 0;
        // the customer may be in the middle of the line, so we need to get the index of the customer in the line
        // get the index of the customer in the line and shift the carts in the line
        for (int i = 0; i < memptr_cashiers->cashiers[bestLineIndex].numCustomers; i++) {
            if (memptr_cashiers->cashiers[bestLineIndex].cartsQueue[i].customerPID == getpid()) {
                ind = i;
                // shift the carts in the line
                for (int j = i; j < memptr_cashiers->cashiers[bestLineIndex].numCustomers; j++) {
                    memptr_cashiers->cashiers[bestLineIndex].cartsQueue[j] = memptr_cashiers->cashiers[bestLineIndex].cartsQueue[j + 1];
                }
                break;
            }
        }

        // clear the last cart in the line
        memptr_cashiers->cashiers[bestLineIndex].cartsQueue[memptr_cashiers->cashiers[bestLineIndex].numCustomers].customerPID = 0;
        memptr_cashiers->cashiers[bestLineIndex].cartsQueue[memptr_cashiers->cashiers[bestLineIndex].numCustomers].numItems = 0;
        memptr_cashiers->cashiers[bestLineIndex].cartsQueue[memptr_cashiers->cashiers[bestLineIndex].numCustomers].quantityOfItems = 0;
        
        // Release the semaphore

        printf("Customer %d is leaving the store without buying anything\n", cartID);
        fflush(stdout);

        msg.msgType = MSG_POS_UPDATE;
        msg.id = 10 + cartID; // if id > 10 then it is a customer
        msg.x = bestLineIndex; // cashier index
        msg.y = ind; // number in the queue
        msg.state = 2; // State (2 = left the line)

        if (msgsnd(msgQueueId, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            return 1;
        }
        
        // The customer may be in the middle of the line, so we need to update the tail
        memptr_cashiers->cashiers[bestLineIndex].tail = (memptr_cashiers->cashiers[bestLineIndex].tail - 1) % MAX_CUSTOMERS;

        

        // acquire the semaphore
        acquireSem(semid, 0);
        // Return the items to the market inventory
        for (int i = 0; i < numItemsToBuy; i++) {
            memptr_items->items[IndecisOfItemsToBuy[i]].inventory += atoi(cart.items[i][1].str);
        }

        // release the semaphore
        releaseSem(semid, 0);

        // send SIGUSR2 to the parent (project1.c) to indicate that the customer is leaving the market because impatient
        if (kill(parentPid, SIGUSR2) == SIG_ERR) {
            perror("kill -- SIGUSR2 -- Customer -- failed");
            exit(EXIT_FAILURE);
        }
        sleep(0.01); // To make the signal reach the parent sequentially
        // release the semaphore
        releaseSem(semid, 1);
        
        
        exit(0);
    }
    // if the customer is on the cashier
    else{
        
        // pause the process until the cashier is done
        pause();       
    }
    
    msg.msgType = MSG_POS_UPDATE;
    msg.id = 10 + cartID; // if id > 10 then it is a customer
    msg.x = bestLineIndex; // cashier index
    msg.y = 0; // number in the queue
    msg.state = 2; // State (2 = left the line)

    if (msgsnd(msgQueueId, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        return 1;
    }

    // exit the market
    printf("Customer %d is leaving the store after buying %d items\n", cartID, quantityOfItems);
    fflush(stdout);
    exit(0);    



}

void catchAlarm(int sig_num) {
    
}

void catchSIGUSR1(int sig_num) {
    //clear the alarm since the customer is on the cashier
    alarm(0);
    isOnCashier = 1;
}

void catchSIGINT(int sig_num) {
    exit(0);
}

// function to acuire the semaphore
void acquireSem(int semid, int semnum) {
    acquire.sem_num = semnum;
    if ( semop(semid, &acquire, 1) == -1 ) {
        perror("semop -- consumer -- acquire");
        exit(4);
    }
}

// function to release the semaphore
void releaseSem(int semid, int semnum) {
    release.sem_num = semnum;
    if ( semop(semid, &release, 1) == -1 ) {
        perror("semop -- consumer -- release");
        exit(5);
    }
}
