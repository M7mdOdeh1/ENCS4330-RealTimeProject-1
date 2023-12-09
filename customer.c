/*
Customer.c
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
    pid_t parentPid = atoi(argv[1]);
    int cartID = atoi(argv[2]);
    int buyTime = atoi(argv[3]);
    int waitTime = atoi(argv[4]);
    // convert string to key_t
    key_t key_cashier = atoi(argv[5]);


    struct SHOPPING_CART cart;

    srand((unsigned) getpid()); // seed for the random function with the ID of the current process

    if ( argc != 6 ) {
        fprintf(stderr, "Usage: %s message\n", *argv);
    	exit(-1);
     }


    // sleep for the time the customer is shopping
    printf("Customer %d is shopping for %d seconds\n", cartID, buyTime);
    sleep(buyTime);

    
    

    int shmid_items;
    char *shmptr_items;
    struct MEMORY *memptr_items;
    // get the shared memory segment
    if ( (shmid_items = shmget((int) parentPid, 0, 0)) != -1 ) {
        // attach to the shared memory segment
        if ( (shmptr_items = (char *) shmat(shmid_items, (char *)0, 0)) == (char *) -1 ) {
            perror("shmat -- consumer -- attach");
            exit(1);
        }
        memptr_items = (struct MEMORY *) shmptr_items;
        

    }
    else {
        perror("shmget -- consumer -- access");
        exit(2);
    }


    int numItemsToBuy = randomInRange(1, memptr_items->numItems);


    // print the items
    for (int i = 0; i < numItemsToBuy; i++) {
        int indexOfItem = randomInRange(0, memptr_items->numItems - 1);
        if (memptr_items->items[indexOfItem].inventory > 0) {
            // Buy a random quantity of the item
            int quantity = randomInRange(1, memptr_items->items[indexOfItem].inventory);
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
    // print all items
    for (int i = 0; i < memptr_items->numItems; i++) {
        printf("%s %d %f\n", memptr_items->items[i].name, memptr_items->items[i].inventory, memptr_items->items[i].price);
    }

    printf("----------------------------------------\n");

    // print the cart items
    printf("Customer %d is done shopping. The contents of the cart are:\n", cartID);
    for (int i = 0; i < cart.numItems; i++) {
        printf("%s %s %s\n", cart.items[i][0].str, cart.items[i][1].str, cart.items[i][2].str);
    }

    printf("----------------------------------------\n");
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
        perror("shmat -- parent -- attach");
        exit(1);
    }
    
    // access the cashiers 
    struct ALL_CASHIERS *memptr_cashiers;
    memptr_cashiers = (struct ALL_CASHIERS *) shmptr_cashier;
    printf("numCashiers: %d\n", memptr_cashiers->numCashiers);
    
    // print the cashiers
    for (int i = 0; i < memptr_cashiers->numCashiers; i++) {
        printf("Cashier %d has %d customers\n", memptr_cashiers->cashiers[i].id, memptr_cashiers->cashiers[i].numCustomers);
        // print the carts
        for (int j = 0; j < memptr_cashiers->cashiers[i].numCustomers; j++) {
            printf("Customer %d has %d items\n", j, memptr_cashiers->cashiers[i].cartsQueue[j].numItems);
            for (int k = 0; k < memptr_cashiers->cashiers[i].cartsQueue[j].numItems; k++) {
                printf("%s %s %s\n", memptr_cashiers->cashiers[i].cartsQueue[j].items[k][0].str, memptr_cashiers->cashiers[i].cartsQueue[j].items[k][1].str, memptr_cashiers->cashiers[i].cartsQueue[j].items[k][2].str);
            }
        }
    }



    



}