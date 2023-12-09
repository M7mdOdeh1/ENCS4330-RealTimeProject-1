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

    struct SHOPPING_CART cart;

    srand((unsigned) getpid()); // seed for the random function with the ID of the current process

    if ( argc != 5 ) {
        fprintf(stderr, "Usage: %s message\n", *argv);
    	exit(-1);
     }


    // sleep for the time the customer is shopping
    printf("Customer %d is shopping for %d seconds\n", cartID, buyTime);
    sleep(buyTime);

    


    int shmid;
    char *shmptr;
    struct MEMORY *memptr;
    // get the shared memory segment
    if ( (shmid = shmget((int) parentPid, 0, 0)) != -1 ) {
        // attach to the shared memory segment
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


    int numItemsToBuy = randomInRange(1, memptr->numItems);


    // print the items
    for (int i = 0; i < numItemsToBuy; i++) {
        int indexOfItem = randomInRange(0, memptr->numItems - 1);
        if (memptr->items[indexOfItem].inventory > 0) {
            // Buy a random quantity of the item
            int quantity = randomInRange(1, memptr->items[indexOfItem].inventory);
            // Update the inventory
            memptr->items[indexOfItem].inventory -= quantity;

            // Add the item to the customer's cart
            strcpy(cart.items[i][0].str, memptr->items[indexOfItem].name);
            sprintf(cart.items[i][1].str, "%d", quantity);
            sprintf(cart.items[i][2].str, "%f", memptr->items[indexOfItem].price);

            cart.numItems++;
            cart.quantityOfItems += quantity;
            

        }

    }
    // print all items
    for (int i = 0; i < memptr->numItems; i++) {
        printf("%s %d %f\n", memptr->items[i].name, memptr->items[i].inventory, memptr->items[i].price);
    }

    printf("----------------------------------------\n");

    // print the cart items
    printf("Customer %d is done shopping. The contents of the cart are:\n", cartID);
    for (int i = 0; i < cart.numItems; i++) {
        printf("%s %s %s\n", cart.items[i][0].str, cart.items[i][1].str, cart.items[i][2].str);
    }

    printf("----------------------------------------\n");
    // print the items after the customer is done shopping
    for (int i = 0; i < memptr->numItems; i++) {
        printf("%s %d %f\n", memptr->items[i].name, memptr->items[i].inventory, memptr->items[i].price);
    }



}