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


int main(int argc, char *argv[])
{


	
    if  ( argc != 3 ) {
    	fprintf(stderr, "Usage: %s message\n", *argv);
    	exit(-1);
     }
    int CASHIER_THRESHOLD;
    int CASHIER_BEHAVIOR = 10;
    int MIN_SCAN_TIME;
    int MAX_SCAN_TIME;
    int NUM_CASHIERS;
    int MAX_CUSTOMER_PERSEC;
    int MIN_CUSTOMER_PERSEC;
    int MAX_BUY_TIME;
    int MIN_BUY_TIME;
    int WAIT_TIME;
    int LEFT_BCS_IMPATIENT;

    srand((unsigned) getpid()); // seed for the random function with the ID of the current process


    FILE *file = fopen(argv[1], "r"); // Replace "config.txt" with your file name
    if (file == NULL) {
        perror("Error opening file");
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
            CASHIER_THRESHOLD = value;
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
        } else if (strcmp(varName, "LEFT_BCS_IMPATIENT") == 0) {
            LEFT_BCS_IMPATIENT = value;
        }
    }
    fclose(file); // closing the file

    
    
     // Declare an array of Item structures
    struct Item items[MAX_ITEMS];

    FILE *file2 = fopen(argv[2], "r"); // Replace "items.txt" with your file name
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

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
    char *shmptr;
    int semid;
    static ushort  start_val[2] = {1, 0};
    union semun    arg;


    
    // Create a shared memory segment
    int shmid = shmget((int) pid, sizeof(memory), 0666 | IPC_CREAT);
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
   
    /*
    // Forking and executing child processes
    for (int i = 0; i < NUM_CASHIERS; i++) {

        

        pid_t cash_pid = fork();
        if (cash_pid == -1) {
            perror("fork cashier failed");
            exit(5);
        }
        
        if (cash_pid == 0) {
            // Child process
            execl("./cashier", "./cashier", (char *)0);
            perror("execl -- cashier -- failed");
            exit(6);
        }
    }
    */


    pid_t cust_spawner_pid = fork();
    if (cust_spawner_pid == -1) {
        perror("fork failed");
        exit(7);
    }

    if (cust_spawner_pid == 0) {
        int cartID = 0;
        // Customer Spawning Child Process
        while (1) { // Replace with a suitable condition to stop spawning
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
                char pidStr[10], cartIDStr[10], buyTimeStr[10], waitTimeStr[10];
                sprintf(pidStr, "%d", (int)pid);
                sprintf(cartIDStr, "%d", cartID);
                sprintf(buyTimeStr, "%d", buyTime);
                sprintf(waitTimeStr, "%d", WAIT_TIME);

                // Customer Process
                execl("./customer", "./customer", pidStr, cartIDStr, buyTimeStr, waitTimeStr, (char *)0);
                perror("execl -- customer -- failed");
                exit(9);
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

    // Detach the shared memory segment
    if (shmdt(shmptr) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }

    // Remove shared memory segment
    if (shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0) == -1) {
        perror("shmctl IPC_RMID failed");
        exit(EXIT_FAILURE);
    }

    // Remove semaphore
    if (semctl(semid, 0, IPC_RMID, 0) == -1) {
        perror("semctl IPC_RMID failed");
        exit(EXIT_FAILURE);
    }
        



    return 0;

}




