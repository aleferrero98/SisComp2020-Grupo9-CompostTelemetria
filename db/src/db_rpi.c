#include "../inc/db_rpi.h"

int write_new_entry(float hum, float temp)
{
    int ret;
    time_t stamp = time(NULL);
    FILE *db = fopen(DB_PATH, "ab");
    if(db == NULL){
        perror("No se pudo abrir la db en modo append");
        return -1;
    }
    ret = _write_new_entry(db, stamp, hum, temp);
    fclose(db);
    return ret;
}

int _write_new_entry(FILE *db, time_t stamp, float hum, float temp)
{
    if(db == NULL){
        return -1;
    }
    size_t ret;
    struct entry new;
    new.timestamp = stamp;
    new.humedad = hum;
    new.temperatura = temp;
    ret = fwrite(&new, 1, sizeof(new), db);
    if(ret != sizeof(new)){
        perror("No se pudo agregar entrada correctamente a la db");
        return -1;
    }
    return 0;
}

int read_formated_last_entry(char *dest_entry)
{
    return read_formated_entry(dest_entry, -1);
}

int read_last_entry(struct entry *dest)
{
    return read_entry(dest, -1);
}

int read_formated_entry(char *dest_entry, long pos)
{
    if(dest_entry == NULL){
        perror("No hay espacio para dest_entry");
        return -1;
    }
    int ret;
    struct entry entry;
    char *str_time;
    FILE *db = fopen(DB_PATH, "rb");
    if(db == NULL){
        perror("No se pudo abrir la db en modo read");
        return -1;
    }
    ret = _read_entry(db, pos, &entry);
    fclose(db);
    if(ret == -1){
        return -1;
    }
    str_time = malloc(FORMAT_SIZE);
    ret = (int)strftime(str_time, STAMP_SIZE, TIMESTAMP_FORMAT, localtime(&(entry.timestamp)));
    if(ret == 0){
        free(str_time);
        return -1;
    }
    ret = sprintf(dest_entry, ENTRY_FORMAT, str_time, entry.humedad, entry.temperatura);
    free(str_time);
    if(ret < 0){
        return -1;
    }
    return 0;
}

int read_entry(struct entry *dest, long pos)
{
    if(dest == NULL){
        perror("No hay espacio para dest");
        return -1;
    }
    int ret;
    FILE *db = fopen(DB_PATH, "rb");
    if(db == NULL){
        perror("No se pudo abrir la db en modo read");
        return -1;
    }
    ret = _read_entry(db, pos, dest);
    fclose(db);
    return ret;
}

int _read_entry(FILE *db, long pos, struct entry *dest)
{
    if(db == NULL){
        return -1;
    }
    int ret;
    int seek_mode;
    long cant_entradas;
    cant_entradas = _count_entrys(db);
    if(cant_entradas == -1){
        return -1;
    }
    if(pos < 0){
        if((-pos) > cant_entradas){
            return -1;
        }
        seek_mode = SEEK_END;
    } else{
        if(pos > cant_entradas){
            return -1;
        }
        seek_mode = SEEK_SET;
    }
    ret = fseek(db, pos*sizeof(*dest), seek_mode);
    ret = (int)fread(dest, 1, sizeof(*dest), db);
    if(ret != sizeof(*dest)){
        perror("No se pudo leer entrada correctamente de la db");
        return -1;
    }
    return 0;
}

long _count_entrys(FILE *db)
{
    if(db == NULL){
        return -1;
    }
    fseek(db, 0, SEEK_END);   ///UBICA CURSOR EN FINAL DE DB
    return ftell(db) / sizeof(struct entry);
}

int db_fprint(FILE *output, long n)
{
    if(output == NULL || n < 0){
        return -1;
    }
    int ret;
    long cant_entradas;
    long pos;
    char *entry_str;
    FILE *db = fopen(DB_PATH, "rb");
    if(db == NULL){
        return -1;
    }
    cant_entradas = _count_entrys(db);
    if(cant_entradas == -1){
        return -1;
    }
    if(n > cant_entradas){
        n = cant_entradas;
    }
    pos = (n > cant_entradas)? 0 : cant_entradas - n;
    entry_str = malloc(FORMAT_SIZE);
    for(; pos < cant_entradas; pos++){
        ret = read_formated_entry(entry_str, pos);
        if(ret == -1){
            fclose(db);
            free(entry_str);
            return -1;
        }
        strcat(entry_str, "\n");
        fwrite(entry_str, 1, strlen(entry_str), output);
        //printf("%s", entry_str);
    }
    fclose(db);
    free(entry_str);
    return 0;
}



