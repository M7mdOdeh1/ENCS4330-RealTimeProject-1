/*
cahirer.c

*/

#include "local3.h"

struct ALL_CASHIERS *memptr_cashiers;
int cashier_index;
int BEHAVIOR_CHANGE_SEC;

float cashierIncome = 0;
float CASHIER_THRESHOLD;
pid_t pid;

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <cashier_id>\n", argv[0]);
        exit(1);
    }


    key_t key_cashier = atoi(argv[1]);
    cashier_index = atoi(argv[2]);
    BEHAVIOR_CHANGE_SEC = atoi(argv[3]);
    CASHIER_THRESHOLD = atof(argv[4]);


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

    // fork a child process to indicate that the cashier behavior is 0, then leave the market
    pid = fork();
    if (pid == -1) {
        perror("fork -- cashier -- failed");
        exit(10);
    }
    else if (pid == 0) {
        // child process
        // signal handler for SIGALRM
        if (signal (SIGALRM, catchAlarm) == SIG_ERR) {
            perror("signal -- cashier -- SIGALRM");
            exit(SIGALRM);
        }
        // alarm after behavior change seconds
        alarm(BEHAVIOR_CHANGE_SEC);

        while (1)
            pause();

        exit(0);
    }

    // signal handler for SIGUSR1
    if (signal (SIGUSR1, catchSIGUSR1) == SIG_ERR) {
        perror("signal -- cashier -- SIGUSR1");
        exit(SIGUSR1);
    }

    // signal handler for SIGINT
    if (signal (SIGINT, catchSIGINT) == SIG_ERR) {
        perror("signal -- cashier -- SIGINT");
        exit(SIGINT);
    }

    while (1){
        pause();
        
         
    }


}


/*
This function serves customers waiting in the cashier's queue.
It sends a SIGUSR1 signal to notify the customers when it's their turn,
and another SIGUSR1 signal when the cashier has finished serving them.
*/

void serveCustomers(){
    
    // keep serveing the customers until the queue is empty
    while (memptr_cashiers->cashiers[cashier_index].numCustomers > 0) {
        // serve the customer in the head of the queue
        int j = memptr_cashiers->cashiers[cashier_index].head;


        // hold SIGUSR2 signal until the customer is done
        if (sighold(SIGUSR2) == -1) {
            perror("sighold -- cashier -- SIGUSR2");
            exit(SIGUSR2);
        }

        // send SIGUSR1 signal to notify the customer when it's his turn
        if(kill(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].customerPID, SIGUSR1) == -1) {
            perror("kill -- cashier -- SIGUSR1");
            exit(SIGUSR1);
        }
    

        int itemInTheCart = memptr_cashiers->cashiers[cashier_index].cartsQueue[j].numItems;
        float totalPrice = 0;
        // scan the items in the cart
        for (int i = 0; i < itemInTheCart; i++) {
            int q = atoi(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].items[i][1].str);
            printf("quantity of item type %d is: %d\n", i+1, q);
            // loop through the quantity of each item
            for (int j=0; j < q; j++) {
                printf("Cashier %d is scanning item %d from type %d\n", getpid(), j+1, i+1);

                // update the quantity of the item in the cart
                memptr_cashiers->cashiers[cashier_index].cartsQueue[j].quantityOfItems--;
                memptr_cashiers->cashiers[cashier_index].numItemsInCarts--;
                // increase the total price
                totalPrice += atof(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].items[i][2].str);
                // sleep for the scan time of the item
                sleep(memptr_cashiers->cashiers[cashier_index].scanTime);
            }
        }

        // update the cashier income
        cashierIncome += totalPrice;
        printf("cashier %d income is: %f===============================\n", cashier_index, cashierIncome);
        printf("cashier %d threshold is: %f========**********=========\n", cashier_index, CASHIER_THRESHOLD);
        if (cashierIncome >= CASHIER_THRESHOLD) {
            // kill alarmer child process
            if (kill(pid, SIGINT) == -1) {
                perror("kill -- cashier -- SIGINT -- cashierIncome >= CASHIER_THRESHOLD");
                exit(SIGINT);
            }

            // check if one of the thresholds is reached from an other cashier or customer
            if (memptr_cashiers->isCashierBehaviorThresholdReached==0 && memptr_cashiers->isIncomeThresholdReached==0
             && memptr_cashiers->isCustomerThresholdReached==0) {

                // set the flag to indicate that the cashier income threshold is reached
                memptr_cashiers->isIncomeThresholdReached = 1;

                // send SIGUSR2 to the parent process to indicate that the cashier is leaving
                if (kill(getppid(), SIGUSR2) == -1) {
                    perror("kill -- cashier -- SIGUSR2");
                    exit(SIGUSR2);
                }
            }
            else{
                printf("cashier threshold is reached *****************************************\n");

            }
            exit(0);
        }

        // send SIGUSR1 to the customer to notify him that the cashier is done
        if (kill(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].customerPID, SIGUSR1) == -1) {
            perror("kill -- cashier -- SIGUSR1");
            exit(SIGUSR1);
        }
        
        // update the number of customers in the line
        memptr_cashiers->cashiers[cashier_index].numCustomers--;    

        // clear the cart
        memptr_cashiers->cashiers[cashier_index].cartsQueue[j].customerPID = 0;
        memptr_cashiers->cashiers[cashier_index].cartsQueue[j].numItems = 0;
        memptr_cashiers->cashiers[cashier_index].cartsQueue[j].quantityOfItems = 0;
        memset(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].items, 0, sizeof(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].items));
        // for (int i = 0; i < MAX_ITEMS; i++) {
        //     strcpy(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].items[i][0].str, "");
        //     strcpy(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].items[i][1].str, "");
        //     strcpy(memptr_cashiers->cashiers[cashier_index].cartsQueue[j].items[i][2].str, "");
        // }

        // update the head of the queue
        memptr_cashiers->cashiers[cashier_index].head = (memptr_cashiers->cashiers[cashier_index].head + 1) % MAX_CUSTOMERS;
        
        // release SIGUSR2 signal
        if (sigrelse(SIGUSR2) == -1) {
            perror("sigrelse -- cashier -- SIGUSR2");
            exit(SIGUSR2);
        }
        

    }
    exit(0);
}

/*
    move the the cart queue to the other cashiers
    and choose the cashier with the best queue
*/
void moveQueueToOtherCashiers(){
    // move the the cart queue to the other cashiers
    for (int i = 0; i < memptr_cashiers->cashiers[cashier_index].numCustomers; i++) {
        // get the customer in the head of the queue
        int j = memptr_cashiers->cashiers[cashier_index].head;


        /*
            choose the cashier according to the following criteria (in order):
            - the less items in the queue and faster scan time
            - the less customers in the queue
            - best behavior
        */
        int min = 0;
        int minItemsAndScan = memptr_cashiers->cashiers[0].numItemsInCarts * memptr_cashiers->cashiers[0].scanTime;
        int minCustomers = memptr_cashiers->cashiers[0].numCustomers;
        int minBehavior = memptr_cashiers->cashiers[0].behavior;
        for (int k = 1; k < memptr_cashiers->numCashiers; k++) {
            int tempItemsAndScan = memptr_cashiers->cashiers[k].numItemsInCarts * memptr_cashiers->cashiers[k].scanTime;
            int tempCustomers = memptr_cashiers->cashiers[k].numCustomers;
            int tempBehavior = memptr_cashiers->cashiers[k].behavior;
            if (tempItemsAndScan < minItemsAndScan) {
                min = k;
                minItemsAndScan = tempItemsAndScan;
                minCustomers = tempCustomers;
                minBehavior = tempBehavior;
            }
            else if (tempItemsAndScan == minItemsAndScan) {
                if (tempCustomers < minCustomers) {
                    min = k;
                    minItemsAndScan = tempItemsAndScan;
                    minCustomers = tempCustomers;
                    minBehavior = tempBehavior;
                }
                else if (tempCustomers == minCustomers) {
                    if (tempBehavior > minBehavior) {
                        min = k;
                        minItemsAndScan = tempItemsAndScan;
                        minCustomers = tempCustomers;
                        minBehavior = tempBehavior;
                    }
                }
            }
        }

        // move the customer to the cashier with the best queue
        memptr_cashiers->cashiers[min].cartsQueue[memptr_cashiers->cashiers[min].tail] = memptr_cashiers->cashiers[cashier_index].cartsQueue[j];
        memptr_cashiers->cashiers[min].tail = (memptr_cashiers->cashiers[min].tail + 1) % MAX_CUSTOMERS;
        memptr_cashiers->cashiers[min].numCustomers++;
        memptr_cashiers->cashiers[min].numItemsInCarts += memptr_cashiers->cashiers[cashier_index].cartsQueue[j].numItems;
        // update the head of the queue
        memptr_cashiers->cashiers[cashier_index].head = (memptr_cashiers->cashiers[cashier_index].head + 1) % MAX_CUSTOMERS;
        // update the number of customers in the line
        memptr_cashiers->cashiers[cashier_index].numCustomers--;
    }
}


/*
catch SIGUSR1 for parent process to serve customers
*/
void catchSIGUSR1(int sig_num) {
    printf("Cashier %d received SIGUSR1\n", getpid());
    serveCustomers();
   
    
}

/*
catch SIGUSR2 for parent process to indicate that the cashier
 behavior is 0, then leave the market
*/
void catchSIGUSR2(int sig_num) {
    printf("Cashier %d received SIGUSR2\n", getpid());
    
    // make this cashier non active
    memptr_cashiers->cashiers[cashier_index].isActive = 0;

    // send SIGUSR1 to the parent process to indicate that the cashier is done
    if (kill(getppid(), SIGUSR1) == -1) {
        perror("kill -- cashier -- SIGUSR1");
        exit(SIGUSR1);
    }

    moveQueueToOtherCashiers();
    
    exit(0);
}


/*
catch SIGALRM for child process to decrease the cashier behavior by 1
and send SIGUSR2 to the parent process to indicate that the cashier is leaving
this function is used only by the child of the cashier process
*/
void catchAlarm(int sig_num) {
    printf("Cashier %d received SIGALRM\n", getpid());
    // decrese the cashier behavior by 1
    memptr_cashiers->cashiers[cashier_index].behavior--;
    
    // if the cashier behavior is 0, then leave the market
    if (memptr_cashiers->cashiers[cashier_index].behavior == 0) {
        printf("Cashier %d is leaving the market because his behavior dropped to zero\n", getpid());

        // check if one of the thresholds is reached from an other cashier or customer
        if (memptr_cashiers->isCashierBehaviorThresholdReached==0 && memptr_cashiers->isIncomeThresholdReached==0
         && memptr_cashiers->isCustomerThresholdReached==0) {

            // set the flag to indicate that the cashier behavior threshold is reached
            memptr_cashiers->isCashierBehaviorThresholdReached = 1;

            // send SIGUSR2 to the parent process to indicate that the cashier is leaving
            if (kill(getppid(), SIGUSR2) == -1) {
                perror("kill -- cashier -- SIGUSR2");
                exit(SIGUSR2);
            }
        }
        else{
            printf("cashier threshold is reached *****************************************\n");

        }
    
        exit(0);
    }
    // alarm after next behavior change seconds
    alarm(BEHAVIOR_CHANGE_SEC);
}

/*
catch SIGINT for parent process to indicate kill alarmer child process
before leaving the market
*/

void catchSIGINT(int sig_num) {
    printf("Cashier %d received SIGINT\n", getpid());
    // kill alarmer child process
    if (kill(pid, SIGINT) == -1) {
        perror("kill -- catchSIGINT -- cashier ");
        exit(SIGINT);
    }
    exit(0);
}
