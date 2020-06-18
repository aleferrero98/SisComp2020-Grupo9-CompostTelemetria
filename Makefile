# Directorios
BINARY_DIR=bin
RASPI_DIR=raspberry_pi
DB_DIR=db
SOURCE=src

#Binarios
RASPI=rpi


# Opt de compilacion 
CC=gcc
CFLAGS= 

all: raspberry
raspberry: $(RASPI_DIR)/raspberry.c $(DB_DIR)/$(SOURCE)/db_rpi.c $(DB_DIR)/inc/db_rpi.h 
	mkdir -p $(BINARY_DIR)
	$(CC) $(CFLAGS) -o $(BINARY_DIR)/$(RASPI) $(RASPI_DIR)/raspberry.c $(DB_DIR)/$(SOURCE)/db_rpi.c -lwiringPi


.PHONY: clean
clean :
	rm  -Rf $(BINARY_DIR) $(DOC_DIR)
