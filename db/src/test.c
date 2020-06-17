#include <stdlib.h>
#include <stdio.h>
#include "../inc/db_rpi.h"

int main()
{
    FILE *test = fopen(DB_PATH, "w+");
    fclose(test);
    int ret;
    for(float f = 0; f < 10; f+=1){
        ret = write_new_entry(f, f*1.4);
        printf("%i", ret);
        //sleep(1);
    }
    
    test = fopen(DB_PATH, "rb");
    struct entry en;
    rewind(test);
    fseek(test, -sizeof(en), SEEK_END);
    fread(&en, 1, sizeof(en), test);
    printf("\n%i\n%s\t%f\t%f\n", ret, ctime(&(en.timestamp)), en.humedad, en.temperatura);
    fclose(test);

    char *s = malloc(FORMAT_SIZE);
    ret = read_formated_entry(s, 3);
    printf("\n%i\n%s\n", ret, s);
    free(s);


    struct entry e;
    ret = read_last_entry(&e);
    printf("\n%i\n%s\t%f\t%f\n", ret, ctime(&(e.timestamp)), e.humedad, e.temperatura);
    ret = db_fprint(stdout, 5);
    printf("\n%i\n", ret);

    return 0;
}