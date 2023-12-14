#include <GL/glut.h> // Make sure to include the appropriate header for your setup
#include "local3.h"
// Assuming global arrays/lists of cashiers and customers, and their counts

int fram_rate = 1000 / 60;

// Structure to represent a customer
typedef struct {
    float x, y, z; // Position of the customer
    // Add more attributes as needed (e.g., color, size)
} Customer;


// Structure to represent a cashier
typedef struct {
    float x, y, z; // Position of the cashier
    int queueLength; // Number of customers in the queue
} Cashier;

void drawCashier(Cashier cashier);
void drawCustomer(Customer customer);
void drawSupermarketLayout();
void initOpenGL();
void displaySupermarket();
void drawRectangle(float x, float y, float width, float height);
void drawCircle(float cx, float cy, float radius, int num_segments);
void drawRectangle(float x, float y, float width, float height);


Cashier cashiers[10];
Customer customers[10];

int numCustomers = 10;
int numCashiers = 3;

void initOpenGL() {
    // Set the background color (e.g., black)
    glClearColor(0.0, 0.0, 0.0, 1.0);

    // Set up the 2D projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 800.0, 0.0, 600.0); // Adjust these values based on your window size

    // Switch to model-view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void drawSupermarketLayout() {
    // Set the color for the layout elements (e.g., grey)
    glColor3f(0.5, 0.5, 0.5);

    // Example: Draw aisles as rectangles
    float aisleWidth = 50.0f;
    float aisleHeight = 10.0f;
    float spacing = 100.0f; // Spacing between aisles

    // Assuming we have 3 aisles in the supermarket
    for (int i = 0; i < 3; i++) {
        float x = 100.0f; // X position of the aisle
        float y = (i + 1) * spacing; // Y position of the aisle

        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + aisleWidth, y);
        glVertex2f(x + aisleWidth, y + aisleHeight);
        glVertex2f(x, y + aisleHeight);
        glEnd();
    }

    // draw a cashier
    cashiers[0].x = 100.0f;
    cashiers[0].y = 100.0f;
    cashiers[0].queueLength = 0;

    cashiers[1].x = 200.0f;
    cashiers[1].y = 100.0f;
    cashiers[1].queueLength = 0;

    drawCashier(cashiers[0]);
    drawCashier(cashiers[1]);
    

    // You can add more elements like shelves or walls in a similar fashion
}


// Display callback function
void displaySupermarket() {
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Reset transformations
    glLoadIdentity();

    // Draw the supermarket layout (you need to define this function)
    drawSupermarketLayout();

    // Draw cashiers and customers
    for (int i = 0; i < numCashiers; i++) {
        drawCashier(cashiers[i]);
    }

    for (int i = 0; i < numCustomers; i++) {
        drawCustomer(customers[i]);
    }

    moveToAisle(&cashiers[0], 1); // Move the first cashier to the second aisle
    glutPostRedisplay(); // Mark the window as needing to be redisplayed

    // Swap buffers (if using double buffering)
    glutSwapBuffers();
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
    glColor3f(0.0, 0.0, 1.0);
    drawRectangle(cashier.x, cashier.y, 20.0, 20.0); // Example size

    glColor3f(1.0, 0.0, 0.0);
    for (int i = 0; i < cashier.queueLength; i++) {
        drawRectangle(cashier.x, cashier.y - (i + 1) * 25.0, 10.0, 10.0); // Smaller size for queue
    }
}

void drawCustomer(Customer customer) {
    glColor3f(0.0, 1.0, 0.0);
    drawCircle(customer.x, customer.y, 10.0, 16); // Example radius and segment count
}

// Function to move a cashier to a specified aisle
void moveToAisle(Cashier *cashier, int aisleIndex) {
    // Define the positions of aisles
    float aislePositions[3][2] = {
        {100.0f, 100.0f}, // Aisle 1 position
        {100.0f, 200.0f}, // Aisle 2 position
        {100.0f, 300.0f}  // Aisle 3 position
    };

    // Check if the aisleIndex is within the valid range
    if (aisleIndex >= 0 && aisleIndex < 3) {
        // Update the cashier's position to the position of the specified aisle
        cashier->x = aislePositions[aisleIndex][0];
        cashier->y = aislePositions[aisleIndex][1];
    } else {
        // Handle invalid aisle index
        printf("Invalid aisle index\n");
    }
}



int main(int argc, char** argv) {
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

    // Enter the GLUT main loop
    glutMainLoop();

    return 0;
}