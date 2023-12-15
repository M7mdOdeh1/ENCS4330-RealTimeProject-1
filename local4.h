#ifndef LOCAL4_H
#define LOCAL4_H

#include <errno.h>
#define MSG_POS_UPDATE 1

typedef struct {
    long msgType; // Message type, used to filter messages
    int id;       // Identifier for cashier or customer
    int x, y;   // Position coordinates
    int state;    // State (e.g., busy, idle for cashier; shopping, in queue for customer)
} PositionUpdateMessage;



// Structure to represent a customer
typedef struct {
    float x, y; // Position of the customer
} Customer;

// Structure to represent a cashier
typedef struct {
    float x, y; // Position of the cashier
    int queueLength; // Number of customers in the queue
    int indecies[MAX_CUSTOMERS];
    float a, b, c; // Color of the cashier
} Cashier;


#endif