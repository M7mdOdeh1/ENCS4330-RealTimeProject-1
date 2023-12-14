/*

Project 1
Mahmoud Hamdan

*/


#include "local3.h"



char* trim(char *str) {
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n')) {
        str++;
    }
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\n')) {
        len--;
    }
    str[len] = '\0';

    return str;
}

int randomInRange(int min_range, int max_range) {
    return (int) (min_range +  (rand() % (max_range - min_range)));
}


// global variables
int shmid, semid, shmid_cashiers;
char *shmptr, *shmptr_cashiers;

int cahsiersLeftTheMarket = 0;
int customersLeftTheMarket = 0;
int totalCustomersSpawned = 0;

int CUST_IMPATIENT_TH;
int NUM_CASHIERS;

int temp=0;

struct ALL_CASHIERS all_cashiers;
int pids_customer[MAX_CUSTOMERS];
pid_t cust_spawner_pid;

int main(int argc, char *argv[])
{

    if  ( argc != 3 ) {
    	fprintf(stderr, "Usage: %s message\n", *argv);
    	exit(-1);
    }


    float CASHIER_THRESHOLD;
    int CASHIER_BEHAVIOR;
    int MIN_SCAN_TIME;
    int MAX_SCAN_TIME;
    int MAX_CUSTOMER_PERSEC;
    int MIN_CUSTOMER_PERSEC;
    int MAX_BUY_TIME;
    int MIN_BUY_TIME;
    int WAIT_TIME;
    int BEHAVIOR_CHANGE_SEC;



    srand((unsigned) getpid()); // seed for the random function with the ID of the current process


    FILE *file = fopen(argv[1], "r"); // Replace "config.txt" with your file name
    if (file == NULL) {
        perror("Error opening the arguments file");
        return 1;
    }

    FILE *file2 = fopen(argv[2], "r"); // Replace "items.txt" with your file name
    if (file == NULL) {
        perror("Error opening the items file");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    char varName[MAX_LINE_LENGTH];
    char valueStr[MAX_LINE_LENGTH];
    int value;

    while (fgets(line, sizeof(line), file) != NULL) {
        // Split the line into variable name and value
        sscanf(line, "%s %s", varName, valueStr);

        // Remove leading and trailing whitespaces from the variable name and value
        trim(varName);
        trim(valueStr);

        int min, max;
        // Assign values based on variable name
        if (strstr(valueStr, ",") != NULL) {
            // If the value is a range, convert it to two integers
            sscanf(valueStr, "%d,%d", &min, &max);
        } else {
            // If the value is a single number, convert it to an integer
            value = atoi(valueStr);
        }

        // Assign values based on variable name
        if (strcmp(varName, "CASHIER_THRESHOLD") == 0) {
            CASHIER_THRESHOLD = (float)value;
        } else if (strcmp(varName, "NUM_CUS_PERSEC") == 0) {
            MAX_CUSTOMER_PERSEC = max;
            MIN_CUSTOMER_PERSEC = min;
        } else if (strcmp(varName, "SCAN_TIME") == 0) {
            MAX_SCAN_TIME = max;
            MIN_SCAN_TIME = min;
        } else if (strcmp(varName, "NUM_CASHIERS") == 0) {
            NUM_CASHIERS = value;
        } else if (strcmp(varName, "CUSTOMER_BUY_TIME") == 0) {
            MAX_BUY_TIME = max;
            MIN_BUY_TIME = min;
        } else if (strcmp(varName, "WAIT_TIME") == 0) {
            WAIT_TIME = value;
        } else if (strcmp(varName, "CUST_IMPATIENT_TH") == 0) {
            CUST_IMPATIENT_TH = value;
        } else if (strcmp(varName, "CASHIER_BEHAVIOR") == 0) {
            CASHIER_BEHAVIOR = value;
        } else if (strcmp(varName, "BEHAVIOR_CHANGE_SEC") == 0) {
            BEHAVIOR_CHANGE_SEC = value;
        }
        
    }
    fclose(file); // closing the file

     // Declare an array of Item structures
    struct Item items[MAX_ITEMS];
  

    char line2[MAX_LINE_LENGTH];
    int itemCount = 0;

    // Read data from the file and load it into the array of items
    while (fgets(line2, sizeof(line2), file2) != NULL && itemCount < MAX_ITEMS) {
        // Split the line into name, inventory, and price
        char itemName[50];
        int itemInventory;
        float itemPrice;

        sscanf(line2, "%s %d %f", itemName, &itemInventory, &itemPrice);

        // Remove leading and trailing whitespaces from the item name
        trim(itemName);

        // Assign values to the array of items
        strcpy(items[itemCount].name, itemName);
        items[itemCount].inventory = itemInventory;
        items[itemCount].price = itemPrice;

        itemCount++;
    }
    // Close the file
    fclose(file2);


    pid_t pid = getpid();
    static struct  MEMORY memory;
    static ushort  start_val[2] = {1, 1};  
    union semun    arg;



    // Create a shared memory segment
    shmid = shmget((int) pid, sizeof(memory), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmid -- parent -- creation");
        exit(1);
    }

    // Attach the shared memory segment to the parent process
    if ( (shmptr = (char *) shmat(shmid, 0, 0)) == (char *) -1 ) {
        perror("shmptr -- parent -- attach");
        exit(2);
    }

    memory.numItems = itemCount;
    // Copy the array of items to the struct memory
    memcpy(memory.items, items, sizeof(items));

    // copy the memory struct to the shared memory segment
    memcpy(shmptr, (char *) &memory, sizeof(memory));



    semid = semget((int) pid, 2, IPC_CREAT | 0666);
    if ( semid == -1 ) {
        perror("semget -- parent -- creation");
        exit(3);
    }

    arg.array = start_val;
    if ( semctl(semid, 0, SETALL, arg) == -1 ) {
      perror("semctl -- parent -- initialization");
      exit(4);
    }

    // Signal Handlers for SIGINT to clear the IPCs before exiting
    if ( sigset(SIGINT, catchSIGINT) == SIG_ERR ) {

        perror("Sigset can not set SIGINT");
        exit(SIGINT);
    }

    // signal handler for SIGUSR1 to indicate that a cashier has left the market
    if (sigset (SIGUSR1, catchSIGUSR1) == SIG_ERR) {
        perror("signal -- parent -- SIGUSR1");
        exit(SIGUSR1);
    }
    // signal handler for SIGUSR2 to indicate that a customer has left the market
    if (sigset (SIGUSR2, catchSIGUSR2) == SIG_ERR) {
        perror("signal -- parent -- SIGUSR2");
        exit(SIGUSR2);
    }
    // signal handler for SIGALARM to indicate that the income increase and check the threshold
    if (sigset (SIGALRM, catchAlarm) == SIG_ERR) {
        perror("signal -- parent -- SIGRTMIN");
        exit(SIGALRM);
    }

    // generate a key for the shared memory segment
    key_t key_cashiers = ftok(".", 'B'); // Ensure 'somefile' exists
    if (key_cashiers == -1) {
        perror("ftok for all cashiers failed");
        exit(EXIT_FAILURE);
    }
    
    // create shared memory for all cashiers
    shmid_cashiers = shmget(key_cashiers, sizeof(struct CASHIER) * NUM_CASHIERS, 0666 | IPC_CREAT);
    if (shmid_cashiers == -1) {
        perror("shmget for all cashiers failed");
        exit(EXIT_FAILURE);
    }

    // attach to the shared memory segment
    shmptr_cashiers = (char *) shmat(shmid_cashiers, (char)0, 0);
    if (shmptr_cashiers== (char *) -1) {
        perror("shmat -- parent -- attach");
        exit(1);
    }
    //all_cashiers.cashiers = (struct CASHIER *)malloc(sizeof(struct CASHIER) * NUM_CASHIERS);
    all_cashiers.numCashiers = NUM_CASHIERS;
    all_cashiers.isCashierBehaviorThresholdReached = 0;
    all_cashiers.isIncomeThresholdReached = 0;
    all_cashiers.isCustomerThresholdReached = 0;
    
    
    struct CASHIER cashier ;

    // Forking and executing child processes for cashiers
    for (int i = 0; i < NUM_CASHIERS; i++) {
        
        // // Initialize the carts queue for each cashier
        // for (int j = 0; j < 1; j++) {
        //     all_cashiers.cashiers[i].cartsQueue[j].numItems = 1;         // Or any initial value
        //     all_cashiers.cashiers[i].cartsQueue[j].quantityOfItems = 2;  // Or any initial value
        //     // Initialize the items in each cart (if needed)
        //     for (int k = 0; k < 1; k++) {
        //         strcpy(all_cashiers.cashiers[i].cartsQueue[j].items[k][0].str, "A"); // Empty string or initial value
        //         strcpy(all_cashiers.cashiers[i].cartsQueue[j].items[k][1].str, "2"); // Empty string or initial value
        //         strcpy(all_cashiers.cashiers[i].cartsQueue[j].items[k][2].str, "100"); // Empty string or initial value
        //     }
        // }

        pid_t cash_pid = fork();
        if (cash_pid == -1) {
            perror("fork cashier failed");
            exit(5);
        }

          if (cash_pid == 0) {
            // Child process
            
            char iStr[10], BEHAVIOR_CHANGE_SECStr[10], CASHIER_THRESHOLDSTR[10], *key_cashiersStr, *semidStr;
            key_cashiersStr = (char *)malloc(sizeof(char) * 32);
            semidStr = (char *)malloc(sizeof(char) * 32);

            sprintf(key_cashiersStr, "%d",key_cashiers);
            sprintf(iStr, "%d", i);
            sprintf(BEHAVIOR_CHANGE_SECStr, "%d", BEHAVIOR_CHANGE_SEC);
            sprintf(CASHIER_THRESHOLDSTR, "%f", CASHIER_THRESHOLD);
            sprintf(semidStr, "%d", semid);
            
            
            execl("./cashier", "./cashier", key_cashiersStr, iStr, BEHAVIOR_CHANGE_SECStr, CASHIER_THRESHOLDSTR, semidStr, (char *)0);
            perror("execl -- cashier -- failed");
            exit(6);

        }

        else{
            // Initialize the cashier
            all_cashiers.cashiers[i].id = cash_pid;
            all_cashiers.cashiers[i].behavior = CASHIER_BEHAVIOR;
            all_cashiers.cashiers[i].numCustomers = 0; 
            all_cashiers.cashiers[i].numItemsInCarts = 0; 
            all_cashiers.cashiers[i].scanTime = randomInRange(MIN_SCAN_TIME, MAX_SCAN_TIME);
            all_cashiers.cashiers[i].isActive = 1;
            all_cashiers.cashiers[i].head = 0;         
            all_cashiers.cashiers[i].tail = 0;

            printf("Cashier id= %d is created with scan time %d\n", all_cashiers.cashiers[i].id, all_cashiers.cashiers[i].scanTime);
        }


    }
    
    // copy the cashiers struct to the shared memory segment of all cashiers
    memcpy(shmptr_cashiers, (char *) &all_cashiers, sizeof(all_cashiers));

    // print all_cashiers
    for (int i = 0; i < all_cashiers.numCashiers; i++) {
        printf("Cashier %d has %d customers\n", all_cashiers.cashiers[i].id, all_cashiers.cashiers[i].numCustomers);
    }

    

    // Customer Spawner
    cust_spawner_pid = fork();
    if (cust_spawner_pid == -1) {
        perror("fork failed");
        exit(7);
    }

    if (cust_spawner_pid == 0) {
        int cartID = 0;

        int c = 1;
        // Customer Spawning Child Process
        while (1) { 
            int delay = randomInRange(MIN_CUSTOMER_PERSEC, MAX_CUSTOMER_PERSEC);
            printf("Customer %d is comming to the market in %d seconds\n", cartID, delay);
            int buyTime = randomInRange(MIN_BUY_TIME, MAX_BUY_TIME);
            sleep(delay);

            pid_t customerPid = fork();
            if (customerPid == -1) {
                perror("fork  customer failed");
                exit(8);
            }

            if (customerPid == 0) {
                char pidStr[10], cartIDStr[10], buyTimeStr[10], waitTimeStr[10], key_cashiersStr[20];
                sprintf(pidStr, "%d", (int)pid);
                sprintf(cartIDStr, "%d", cartID);
                sprintf(buyTimeStr, "%d", buyTime);
                sprintf(waitTimeStr, "%d", WAIT_TIME);
                sprintf(key_cashiersStr, "%d", key_cashiers);
                

                // Customer Process
                execl("./customer", "./customer", pidStr, cartIDStr, buyTimeStr, waitTimeStr, key_cashiersStr, (char *)0);
                perror("execl -- customer -- failed");
                exit(9);
            }
            else{
                pids_customer[cartID] = customerPid;
                totalCustomersSpawned++;
            }
            cartID++;
        }
        exit(0); // Exit the customer spawning process once done
    }




    /*
    TODO: Wait for all child processes to finish
    */
    while (1)
        pause();

    // clear the IPCs
    exitingProgram(shmid, shmptr, semid);




    return 0;

}

// Signal handler for SIGINT
void catchSIGINT(int signo) {

    // if the spawner process (child of project1) catch the signal
    if (cust_spawner_pid == 0){

        printf("Killing all customrs child processes...\n");
        // kill all customer that are generated from the spawnner
        for (int i = 0; i < totalCustomersSpawned; i++) {
            kill(pids_customer[i], SIGINT);
        }

        exit(0);

    }
        
    printf("Caught SIGINT\n");
    exitingProgram();
}


/*
 function to kill all child processes and clear the IPCs
 before exiting the program
*/
void exitingProgram() {
    if (temp==1 || cust_spawner_pid == 0)
        return;
    temp=1;

    // kill the customer spawner
    kill(cust_spawner_pid, SIGINT);

    printf("Killing all cashier processes...\n");
    // kill all cashiers
    for (int i = 0; i < NUM_CASHIERS; i++) {
        kill(all_cashiers.cashiers[i].id, SIGINT);
    }

    printf("Clearing IPCs...\n");

    // Detach the shared memory segment for items
    if (shmdt(shmptr) == -1) {
        perror("shmdt failed -- items");
        exit(EXIT_FAILURE);
    }

    // Remove shared memory segment for items
    if (shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0) == -1) {
        perror("shmctl IPC_RMID failed -- items");
        exit(EXIT_FAILURE);
    }

    // Remove semaphore
    if (semctl(semid, 0, IPC_RMID, 0) == -1) {
        perror("semctl IPC_RMID failed");
        exit(EXIT_FAILURE);
    }

    // Detach the shared memory segment of all cashiers
    if (shmdt(shmptr_cashiers) == -1) {
        perror("shmdt failed -- all cashiers");
        exit(EXIT_FAILURE);
    }

    // Remove shared memory segment of all cashiers
    if (shmctl(shmid_cashiers, IPC_RMID, (struct shmid_ds *) 0) == -1) {
        perror("shmctl IPC_RMID failed -- all cashiers");
        exit(EXIT_FAILURE);
    }

    printf("IPCs cleared successfully\n");
    printf("Exiting...\n");

    exit(0);
}


// Signal handler for SIGUSR1 to indicate that a cashier has left the market
void catchSIGUSR1(int signo) {
    cahsiersLeftTheMarket++;
    printf("left^^^^^^^^^^^^^^^^^^^^^^^^\n");
    fflush(stdout);
    if(cahsiersLeftTheMarket == NUM_CASHIERS){
        printf("Behavior threshold of All Cashiers reached. Sending SIGINT to all cashiers\n");
        struct ALL_CASHIERS *memptr_cashiers;
        memptr_cashiers = (struct ALL_CASHIERS *) shmptr_cashiers;
        fflush(stdout);
        // acquire the semaphore
        acquireSem(semid, 1);
        fflush(stdout);
        // check if one of the thresholds is reached previously
        if (memptr_cashiers->isCustomerThresholdReached == 0 && memptr_cashiers->isCashierBehaviorThresholdReached 
            == 0 && memptr_cashiers->isIncomeThresholdReached == 0){
            fflush(stdout);
            // set the flag to indicate that the cashier behavior threshold is reached
            memptr_cashiers->isCashierBehaviorThresholdReached = 1;
        
            releaseSem(semid, 1);
            exitingProgram();

            
        }
        // release the semaphore
        releaseSem(semid, 1);
        exit(0);

    }

}

// Signal handler for SIGUSR2 to indicate that a customer has left the market
void catchSIGUSR2(int signo) {
    printf("__________________________IMAPTIENT CUSTOMER________________\n");
    fflush(stdout);
    customersLeftTheMarket++;
    if(customersLeftTheMarket == CUST_IMPATIENT_TH ){

        struct ALL_CASHIERS *memptr_cashiers;
        memptr_cashiers = (struct ALL_CASHIERS *) shmptr_cashiers;

        // acquire the semaphore
        acquireSem(semid, 1);

        /* check if one of the thresholds is reached  
            (no need to exit the program again)*/
        if (memptr_cashiers->isCustomerThresholdReached == 0 && memptr_cashiers->isCashierBehaviorThresholdReached 
            == 0 && memptr_cashiers->isIncomeThresholdReached == 0){
            
            // kill spawner
            kill(cust_spawner_pid, SIGINT);
            // set the flag to indicate that the customer threshold is reached
            memptr_cashiers->isCustomerThresholdReached = 1;

            // release the semaphore
            releaseSem(semid, 1);

            printf("Customer Impatient Thresold reached. Sending SIGINT to all cashiers\n");

            exitingProgram();
        }

        // release the semaphore
        releaseSem(semid, 1);

        exit(0);
    }
}

// Signal handler for SIGALRM to indicate that the income threshold is reached
void catchAlarm(int signo) {
    printf("Income threshold reached.\n");
    exitingProgram();
}


// function to acuire the semaphore
void acquireSem(int semid, int semnum) {
    acquire.sem_num = semnum;
    if ( semop(semid, &acquire, 1) == -1 ) {
        perror("semop -- project1 -- acquire");
        exit(4);
    }
}

// function to release the semaphore
void releaseSem(int semid, int semnum) {
    release.sem_num = semnum;
    if ( semop(semid, &release, 1) == -1 ) {
        perror("semop -- project1 -- release");
        exit(5);
    }
}




