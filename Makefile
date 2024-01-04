CC = gcc
CFLAGS = -g -Wall
LIBS = -lglut -lGLU -lGL -lm -lpthread -lrt

ARGS= arguments.txt
ITEMS= items_prices.txt

all: customer cashier gui project1

customer: customer.c
	$(CC) $(CFLAGS) $< -o $@

cashier: cashier.c
	$(CC) $(CFLAGS) $< -o $@

project1: project1.c
	$(CC) $(CFLAGS) $< -o $@

gui: gui.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)
	
run: project1
	./project1 $(ARGS) $(ITEMS)

clean:
	rm -f customer cashier project1 gui
