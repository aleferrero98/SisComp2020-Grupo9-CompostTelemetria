#ifndef _DB_RPI_H
#define _DB_RPI_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#define DB_PATH "../db.txt"
#define STAMP_SIZE 80
#define TIMESTAMP_FORMAT "%Y-%m-%d  %X"
#define ENTRY_FORMAT "%s || Humedad: %.2f || Temperatura: %.2f °C"
#define FORMAT_SIZE 200

struct __attribute__((packed)) entry
{
    time_t timestamp;
    float humedad;
    float temperatura;
};

/**
 * @brief imprime en un stream las ultimas **n** entradas
 * 
 * @param output FILE stream abierto en modo escritura donde se imprimen las entrys
 * @param n cantidad de entradas a imprimer desde la mas actual(timestamp mas nuevo)
 * @return int 0 en caso de exito, -1 en caso de error
 */
int db_fprint(FILE *output, long n);

/**
 * @brief Escribe una nueva entrada del par de mediciones al final del
 * la DB (abre el txt en modo append). Esta entrada nueva contienen un timestamp
 * que se genera al escribirla en la DB
 * 
 * @param hum valor de la humedad
 * @param temp valor de la temperatura
 * @return int 0 en caso de exito, -1 en caso de error
 */
int write_new_entry(float hum, float temp);

/**
 * @brief devuelve un string que representa la entrada mas nueva 
 * 
 * @param dest_entry buffer donde se guarda al string
 * @return int 0 en caso de exito, -1 en caso de error
 */
int read_formated_last_entry(char *dest_entry);

/**
 * @brief devuelve un string que representa la entrada en la posicion *pos*
 * Si se ingresan valores negativos estos se cuentan desde el final de la db, es decir
 * si pos = -1 se devuelve la ultima entrada que se escribio. Sirve imaginarse a la
 * db como una cinta que une el final con el comienzo.
 * 
 * @param dest_entry  buffer donde se guarda al string
 * @param pos posicion en la db de la entrada que se quiere leer
 * @return int 0 en caso de exito, -1 en caso de error
 */
int read_formated_entry(char *dest_entry, long pos);

/**
 * @brief una struct entry de la entrada en la posicion *pos*
 * Si se ingresan valores negativos estos se cuentan desde el final de la db, es decir
 * si pos = -1 se devuelve la ultima entrada que se escribio. Sirve imaginarse a la
 * db como una cinta que une el final con el comienzo.
 * 
 * @param dest donde se devuelve la entrada
 * @param pos posicion en la db de la entrada que se quiere leer
 * @return int 0 en caso de exito, -1 en caso de error
 */
int read_entry(struct entry *dest, long pos);

/**
 * @brief una struct entry de la entrada mas nueva
 * 
 * @param dest donde se devuelve la entrada
 * @return int 0 en caso de exito, -1 en caso de error
 */
int read_last_entry(struct entry *dest);


int _write_new_entry(FILE *db, time_t stamp, float hum, float temp);
int _read_entry(FILE *db, long pos, struct entry *dest);
long _count_entrys(FILE *db);

#endif //_DB_RPI_H