#ifndef LOCAL3_H
#define LOCAL3_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h> 
#include <wait.h>
#include <signal.h>

#define MAX_LINE_LENGTH 100
#define MAX_ITEMS 1000
#define MAX_CUSTOMERS 10000

struct Item {
    char name[50];
    int inventory;
    float price;
};

struct MEMORY {
    struct Item items[MAX_ITEMS];
    int numCashiersLeftSimulation; // Number of cashiers whose behavior dropped to 0
    int numImpatientCustomers; // Number of customers who left without buying
    float totalSupermarketIncome; 
    int head, tail;
};

struct String {
    char str[50];
};

struct SHOPPING_CART {
    int numItems;
    struct String items[MAX_ITEMS][2];
};

struct ALL_SHOPPING_CARTS {
    struct SHOPPING_CART carts[MAX_CUSTOMERS];
    int head, tail;
};





union semun {
    int val;
    struct semid_ds *buf;
    ushort *array; 
};

struct sembuf acquire = {0, -1, SEM_UNDO}, 
              release = {0,  1, SEM_UNDO};

#endif