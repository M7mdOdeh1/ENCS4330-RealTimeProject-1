/*



Project 1

Mohammad Odeh

Mahmoud Hamdan

Yazeed Hamdan

Mohammad Abu-Shams



*/





#include "local3.h"



int main(int argc, char *argv[])

{





	

    if  ( argc != 3 ) {

    	fprintf(stderr, "Usage: %s message\n", *argv);

    	exit(1);

     }

    int CASHIER_THRESHOLD;

    int NUM_CUS_PERSEC;

    int minArrivalTime;

    int maxArrivalTime;

    int SCAN_TIME;

    int NUM_CASHIERS;

    int CUSTOMER_BUY_TIME;

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



        // Assign values based on variable name

        if (strstr(valueStr, ",") != NULL) {

            // If the value is a range, generate a random number within the range

            int min, max;

            sscanf(valueStr, "%d,%d", &min, &max);

            value = RandomNumberGen(min, max);

        } else {

            // If the value is a single number, convert it to an integer

            value = atoi(valueStr);

        }



        // Assign values based on variable name

        if (strcmp(varName, "CASHIER_THRESHOLD") == 0) {

            CASHIER_THRESHOLD = value;

        } else if (strcmp(varName, "NUM_CUS_PERSEC") == 0) {

            NUM_CUS_PERSEC = value;

        } else if (strcmp(varName, "SCAN_TIME") == 0) {

            SCAN_TIME = value;

        } else if (strcmp(varName, "NUM_CASHIERS") == 0) {

            NUM_CASHIERS = value;

        } else if (strcmp(varName, "CUSTOMER_BUY_TIME") == 0) {

            CUSTOMER_BUY_TIME = value;

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

    

    

    #ifdef __exec__



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

    // Copy the array of items to the shared memory segment

    memcpy(shmptr, (char *) &memory, sizeof(memory));

    

     // Create a semaphore to control the access of the shared memory  

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



    // Forking and executing child processes

    for (int i = 0; i < NUM_CASHIERS; i++) {

        pid_t cash_pid = fork();

        if (cash_pid == -1) {

            perror("fork failed");

            exit(EXIT_FAILURE);

        }



        if (cash_pid == 0) {

            // Child process

            execl("./cashier", "./cashier", (char *)NULL);

            perror("execl -- cashier -- failed");

            exit(EXIT_FAILURE);

        }

    }





    pid_t cust_spawner_pid = fork();

    if (cust_spawner_pid == -1) {

        perror("fork failed");

        exit(EXIT_FAILURE);

    }



    if (pid == 0) {

        // Customer Spawning Child Process

        while (1) { // Replace with a suitable condition to stop spawning

            int delay = RandomNumberGen(minArrivalTime, maxArrivalTime);

            sleep(delay);



            pid_t customerPid = fork();

            if (customerPid == -1) {

                perror("fork failed");

                // Decide how to handle fork failure - maybe continue or break

            }



            if (customerPid == 0) {

                // Customer Process

                execl("./customer", "./customer", (char *)NULL);

                perror("execl failed");

                exit(EXIT_FAILURE);

            }

        }

        exit(0); // Exit the customer spawning process once done

    }





	



    /*

    TODO: Wait for all child processes to finish

    */





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

        

	#endif





    return 0;



}











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



int RandomNumberGen(int min_range, int max_range) {

    return (int) (min_range +  (rand() % (max_range - min_range)));

}



