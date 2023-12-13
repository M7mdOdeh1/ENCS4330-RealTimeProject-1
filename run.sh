#!/bin/sh


gcc customer.c -o customer
gcc cashier.c -o cashier
gcc project1.c -o project1

./project1 arguments.txt items_prices.txt



