#include <GL/glut.h>
#include <stdlib.h>
#include "local3.h"
#include "local4.h"



int frame_rate = 1000 / 60;

pid_t keyItemShm;
key_t keyCashierShm;



Cashier cashiers[MAX_CASHIERS];
Customer customers[MAX_CUSTOMERS];


int numCustomers = MAX_CUSTOMERS;
int numCashiers = MAX_CASHIERS;
int temp = 0;
int temp2 = 0;
int msgQueueId; // Message queue ID
int shmid_cashiers;
int shmid_items;
struct ALL_CASHIERS *shm_cashiers;
struct Item *shm_items;

// Function declarations
void displaySupermarket();
void timerFunc(int value);
void initOpenGL();
void drawCashier(Cashier cashier);
void drawCustomer(Customer customer);
void drawRectangle(float x, float y, float width, float height);
void drawCircle(float cx, float cy, float radius, int num_segments);
void removeCustomerFromCashier(int customerId, int x, int y, int state);
void addCustomerToCashier(int customerId, int x, int y);
void drawSupermarketLayout();



// // function to connect to cashier shared memory
// void connectToCashierShm() {
//     shmid_cashiers = shmget(keyCashierShm, sizeof(struct ALL_CASHIERS) * 10, 0666 | IPC_CREAT);
//     if (shmid_cashiers == -1) {
//         perror("shmget");
//         exit(EXIT_FAILURE);
//     }
//     shm_cashiers = (struct ALL_CASHIERS *)shmat(shmid_cashiers, NULL, 0);
//     if (shm_cashiers == (void *)-1) {
//         perror("shmat");
//         exit(EXIT_FAILURE);
//     }
// }

// // function to connect to item shared memory
// void connectToItemShm() {
//     shmid_items = shmget(keyItemShm, sizeof(struct Item) * 10, 0666 | IPC_CREAT);
//     if (shmid_items == -1) {
//         perror("shmget");
//         exit(EXIT_FAILURE);
//     }
//     shm_items = (struct Item *)shmat(shmid_items, NULL, 0);
//     if (shm_items == (void *)-1) {
//         perror("shmat");
//         exit(EXIT_FAILURE);
//     }
// }

void initOpenGL() {
    // Set the background color (e.g., black)
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);


    // Set up the 2D projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 800.0, 0.0, 600.0); // Adjust these values based on your window size

    // Connect to the message queue
    key_t key = 1234; // Use the same key as used in project1.c
    msgQueueId = msgget(key, 0666 | IPC_CREAT);
    if (msgQueueId == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    int spacing = 50.0f; // Spacing between aisles
    for (int i = 0; i < numCashiers; i++) {
        cashiers[i].x = 80.0f;
        cashiers[i].y = 80.0f + i * spacing;
        cashiers[i].queueLength = 0;
        cashiers[i].a = 0.0f;
        cashiers[i].b = 0.0f;
        cashiers[i].c = 1.0f; // Blue color
    }

    for (int i = 0; i < numCustomers; i++) {
        customers[i].x = 0.0f;
        customers[i].y = 0.0f;
    }
    


    // Switch to model-view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void timerFunc(int value) {
    // Function definition
    glutPostRedisplay();
    glutTimerFunc(1000 / 60, timerFunc, 0); // Re-register the timer for continuous updates
}


void drawSupermarketLayout() {
    // Set the color for the layout elements (e.g., grey for aisles)
    glColor3f(0.5, 0.5, 0.5);

    // Example: Draw aisles as rectangles
    float aisleWidth = 300.0f;
    float aisleHeight = 10.0f;
    float spacing = 50.0f; // Spacing between aisles

    // Draw aisles
    for (int i = 0; i < 10; i++) { 
        float x = 100.0f; // X position of the aisle
        float y = 80.0f + i * spacing; // Y position of the aisle

        drawRectangle(x, y, aisleWidth, aisleHeight);
    }

    // Example: Draw a wall
    glColor3f(0.7, 0.7, 0.7); // Wall color
    drawRectangle(0.0f, 0.0f, 800.0f, 20.0f); // Wall at the bottom
    // wall at the top
    drawRectangle(0.0f, 600.0f - 20.0f, 800.0f, 20.0f);
    // wall at the left
    drawRectangle(0.0f, 0.0f, 20.0f, 600.0f);
    // wall at the right
    drawRectangle(800.0f - 20.0f, 20.0f, 20.0f, 600.0f);

    // draw an item bar at the right beside the wall with red color
    glColor3f(1.0, 0.0, 0.0);
    drawRectangle(800.0f - 60.0f, 50.0f, 40.0f, 500.0f);

    // // Draw the cashier desks
    // for (int i = 0; i < numCashiers; i++) {
    //     drawCashier(cashiers[i]);
    // }
}



void displaySupermarket() {
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);
    

    // Reset transformations
    glLoadIdentity();

    

    // Check for new messages and update positions
    PositionUpdateMessage msg;
    while (msgrcv(msgQueueId, &msg, sizeof(msg) - sizeof(long), MSG_POS_UPDATE, IPC_NOWAIT) != -1) {
        if (msg.id < numCashiers) { // Check if the message is for a cashier
            // change color of the cashier to dark grey
            cashiers[msg.id].a = 0.3f;
            cashiers[msg.id].b = 0.3f;
            cashiers[msg.id].c = 0.3f;

        } else { // Message is for a customer
            int customerId = msg.id - numCashiers;
            if (customerId < numCustomers) {
                // Update customer position directly
                if (msg.state == 0) {
                    addCustomerToItemBar(customerId);
                }
                else if (msg.state == 1) {
                    addCustomerToCashier(customerId, msg.x, msg.y);
                }
                else {
                    removeCustomerFromCashier(customerId, msg.x, msg.y, msg.state);
                }

            }
        }
    }

    // Draw the supermarket layout
    drawSupermarketLayout();

    // Draw cashiers and customers
    for (int i = 0; i < numCashiers; i++) {
        drawCashier(cashiers[i]);
    }
    for (int i = 0; i < numCustomers; i++) {
        drawCustomer(customers[i]);
    }

    // Swap buffers
    glutSwapBuffers();
}

void addCustomerToItemBar(int customerId) {
    int c = customerId % 20;
    if (temp == 20){
        temp = 0;
        temp2++;
    }
    temp++;
    // add customer to item bar
    customers[customerId].x = 740.0f - 25.0f * temp2;
    customers[customerId].y = 60.0f + 4200.0f / (numCustomers) * c;
}

void addCustomerToCashier(int customerId, int x, int y) {
    // add customer to cashier x
    
    customers[customerId].x =  80.0f + cashiers[x].queueLength * 25.0f;
    customers[customerId].y = 80.0f + x * 50.0f;
    cashiers[x].indecies[cashiers[x].queueLength] = customerId;
    cashiers[x].queueLength++;
    
}

// remove customer from cashier x
void removeCustomerFromCashier(int customerId, int x, int y, int state) {
      
    customers[customerId].x = 0.0f;
    customers[customerId].y = 0.0f;
    // The customer left from the middle of the queue so we need to update the positions of the customers behind him
    for (int i = 0; i < cashiers[x].queueLength; i++) {
        if (cashiers[x].indecies[i] == customerId) {
            for (int j = i; j < cashiers[x].queueLength - 1; j++) {
                cashiers[x].indecies[j] = cashiers[x].indecies[j + 1];
                // update the position of the customer
                customers[cashiers[x].indecies[j]].x = 80.0f + j * 25.0f;
                customers[cashiers[x].indecies[j]].y = 80.0f + x * 50.0f;
            }
            break;
        }
    }
    cashiers[x].indecies[cashiers[x].queueLength - 1] = 0;
    // remove customer from cashier x
    cashiers[x].queueLength--;        
    
}



void drawRectangle(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawCircle(float cx, float cy, float radius, int num_segments) {
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < num_segments; i++) {
        float theta = 2.0f * 3.1415926f * (float)i / (float)num_segments;
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

void drawCashier(Cashier cashier) {
    glColor3f(cashier.a, cashier.b, cashier.c);
    drawRectangle(cashier.x, cashier.y, 20.0, 20.0); // Example size

}

void drawCustomer(Customer customer) {
    glColor3f(0.0, 1.0, 0.0);
    drawCircle(customer.x, customer.y, 10.0, 16); // Example radius and segment count
}



int main(int argc, char** argv) {

    if (argc != 3) {
        printf("Usage: ./gui <input_file>\n");
        exit(1);
    }
    keyItemShm = atoi(argv[1]);
    keyCashierShm = atoi(argv[2]);

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    
    // Set the window size
    glutInitWindowSize(800, 600);
    
    // Create a window with a title
    glutCreateWindow("Supermarket Simulation");

    // Initialize OpenGL settings
    initOpenGL();

    // Register the display callback function
    glutDisplayFunc(displaySupermarket);

    // Register the idle function for continuous animation
    glutIdleFunc(displaySupermarket);

    glutTimerFunc(1000 / 60, timerFunc, 0); // Start the timer for the first time



    // Enter the GLUT main loop
    glutMainLoop();

    return 0;
}