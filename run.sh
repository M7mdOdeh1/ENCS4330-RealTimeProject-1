#!/bin/sh


gcc customer.c -o customer
gcc cashier.c -o cashier
gcc project1.c -o project1
gcc -g gui.c -o gui -lglut -lGLU -lGL -lm -lpthread -lrt

./project1 arguments.txt items_prices.txt



