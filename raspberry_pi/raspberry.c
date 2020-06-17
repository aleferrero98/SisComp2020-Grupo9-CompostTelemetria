/**
 * https://www.circuitbasics.com/how-to-set-up-the-dht11-humidity-sensor-on-the-raspberry-pi/
 * gcc -o rpi raspberry.c -lwiringPi
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../db/inc/db_rpi.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/time.h>

#define MAXTIMINGS	85
#define DHTGPIO		4	//GPIO4 del sensor DHT11
#define PERIODO		3000  //1800000  //periodo de 30 min
#define CANT_BYTES	5
#define LIM_ERROR 	100
#define TEMPERATURA	2
#define HUMEDAD		1
#define ALL			3
#define ERROR		-1
#define PIN_OUT     "echo s > /dev/raspin"
#define PIN_IN		"echo e > /dev/raspin"	
#define SET_PIN		"echo 1 > /dev/raspin"
#define CLR_PIN		"echo 0 > /dev/raspin"
#define TEMP_LIM	50
#define HUM_LIM		80
#define T_HUM		500
#define	T_TEMP		1000
#define BUZZERGPIO	26
#define PIN			7

typedef struct dato{
	int entero;
	int decimal;
}dato;

int read_dht11_dat(int *dht11_dat);
void sensar(dato *hum, dato *temp);
void pin_mode(int modo, int pin);
void clear_pin(int pin);
void set_pin(int pin);
void set_valores_limites(int humedad, int temperatura);
void sonar_alarma(int per);

int close_con(int socket);
int open_inet_con(char *host_name, char *port);
int accept_client_con(int socket);
int open_inet_sock(char *port);


long int periodo = PERIODO;
int remoto_activo = TRUE; 
int temp_limite = TEMP_LIM;
int hum_limite = HUM_LIM;
int sock_server;
int sock_client;
 
int main(int argc, char *argv[])
{
	printf("%s\n", "GRUPO 9: Sistema de monitoreo del compost");
	char buffer[100];
	dato humedad, temperatura;
	if ( wiringPiSetup() == ERROR )
		exit(EXIT_FAILURE);
		
	if(argc < 1){
        exit(EXIT_FAILURE);
    }
    int ret;
    sock_server = open_inet_sock(argv[1]);//nro puerto
    if(sock_server == -1){
        perror("No se pudo crear socket");
        exit(EXIT_FAILURE);
    }
   sock_client = accept_client_con(sock_server);
    if(sock_client == -1){
        perror("No se pudo conectar con el cliente");
        exit(EXIT_FAILURE);
    }
    struct timeval tv;
    fd_set readfds;
 
    tv.tv_sec = 60;
    tv.tv_usec = 0;
 
    FD_ZERO(&readfds);
    FD_SET(sock_client, &readfds);
 
    
	printf("%s\n", "cliente acepto");
	char *buff, hum[8], temp[8], aux1[512], *aux2;
	int pid;
    buff = malloc(512);
	while ( 1 )
	{
		//pid = fork();
		//if(pid){
		select(sock_client+1, &readfds, NULL, NULL, &tv);
			if(FD_ISSET(sock_client, &readfds)){
				memset(buff, '\0', 512);
				recv(sock_client, buff, 512, 0);
				switch (buff[0])
				{
					case 'A':
						sensar(&humedad, &temperatura);
						read_formated_last_entry(buff);
						send(sock_client, buff, strlen(buff), 0);
						break;
					case 'U':
						printf("%s\n", buff);
						strcpy(aux1, &buff[1]);
						strcpy(hum, strtok(aux1, "S"));
						strcpy(temp, strtok(NULL, "F"));
						printf("humedad lim: %s\ttemperatura lim: %s\n", hum, temp);
						set_valores_limites(atoi(temp), atoi(temp));
						break;
					default:
						printf("No debi recibir esto\n");
						break;
				}

			}else{
			
				sensar(&humedad, &temperatura);
				if(humedad.entero > hum_limite){
					sonar_alarma(T_HUM);
				}
				if(temperatura.entero > temp_limite){
					sonar_alarma(T_TEMP);
				}
		
				read_formated_last_entry(buffer);
			}
		printf("%s\n", buffer);
		printf("Humedad = %d.%d %% Temperatura = %d.%d °C\n", humedad.entero, humedad.decimal, temperatura.entero, temperatura.decimal);
		//delay(periodo); 
	}
	
	close_con(sock_client);
    close_con(sock_server);
 
	return EXIT_SUCCESS;
}

void sonar_alarma(int per){
	pin_mode(OUTPUT, BUZZERGPIO);
	int i=0;
	while(i < 1000){
		set_pin(BUZZERGPIO);
		delay(per);
		clear_pin(BUZZERGPIO);
		delay(per);
		i++;
	}
}
void set_valores_limites(int humedad, int temperatura){
	if(humedad > 0){
		hum_limite = humedad;
	}
	if(temperatura > 0){
		temp_limite = temperatura;
	}
}
void set_periodo_muestreo(int num){
	if(num > 0){
		periodo = num;
	}	
}
void toggle_control_remoto(void){
	remoto_activo = !remoto_activo;
}
void sensar(dato *hum, dato *temp){
	int dht11_dat[CANT_BYTES] = { 0, 0, 0, 0, 0 }, ret, error = 0;
	float temperatura, humedad;
	
	ret = read_dht11_dat(dht11_dat);
	while((ret == EXIT_FAILURE) && (error < LIM_ERROR)){ //hasta 100 mediciones erroneas insiste
		ret = read_dht11_dat(dht11_dat);
		error++;
		delay( 100 );
	}
	hum->entero = dht11_dat[0];
	hum->decimal = dht11_dat[1];
	temp->entero  = dht11_dat[2];
	temp->decimal = dht11_dat[3];

	//printf("Humedad = %d.%d %% Temperatura = %d.%d °C\n", hum->entero, hum->decimal, temp->entero, temp->decimal);
	
	//escribir BDD
	if(hum->decimal < 10){
		humedad = (float) hum->decimal/10;
	}else{
		humedad = (float) hum->decimal/100;
	}
	if(temp->decimal < 10){
		temperatura = (float) temp->decimal/10;
	}else{
		temperatura = (float) temp->decimal/100;
	}
	
	humedad += (float) hum->entero + hum->decimal/100;
	temperatura += (float) temp->entero + temp->decimal/100;
	write_new_entry(humedad, temperatura);
		
}
/**
 * Configura el pin como de entrada o salida
 */
void pin_mode(int modo, int pin){
	char comando[50], num_pin[5];
	switch(modo){
		case INPUT:
			strcpy(comando, PIN_IN);
			sprintf(num_pin, "%d", pin);
			strcat(comando, num_pin);
			system(comando);
			break;
		case OUTPUT:
			strcpy(comando, PIN_OUT);
			sprintf(num_pin, "%d", pin);
			strcat(comando, num_pin);
			system(comando);
			break;
		default:
			fprintf(stderr, "%s\n", "Error en pinmode()");
	}
	//printf("mode %s\n", comando);
}
/**
 * Manda un 1 al pin de salida
 */
void set_pin(int pin){
	char comando[50], num_pin[5];
	strcpy(comando, SET_PIN);
	sprintf(num_pin, "%d", pin);
	strcat(comando, num_pin);
	system(comando);
	//printf("set %s\n", comando);
}
/**
 * Pone en 0 el pin de salida
 */
void clear_pin(int pin){
	char comando[50], num_pin[5];
	strcpy(comando, CLR_PIN);
	sprintf(num_pin, "%d", pin);
	strcat(comando, num_pin);
	system(comando);
	//printf("clear %s\n", comando);
}

int read_dht11_dat(int *dht11_dat)
{
	uint8_t laststate	= HIGH;
	uint8_t counter		= 0;
	uint8_t j			= 0, i;

	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

	//pinMode( 7 , OUTPUT );
	//digitalWrite( 7 , LOW );
	pin_mode(OUTPUT, DHTGPIO);
	clear_pin(DHTGPIO);
	delay( 18 );				//18 ms en bajo
	digitalWrite( PIN , HIGH );
	//set_pin(DHTGPIO);
	delayMicroseconds( 25 );//entre 20 y 40 dice el datasheet
	pinMode( PIN, INPUT );//lectura del pin
	//pin_mode(INPUT, DHTGPIO);
	
	for ( i = 0; i < MAXTIMINGS; i++ )
	{
		counter = 0;
		while ( digitalRead(PIN) == laststate )
		{
			counter++;
			delayMicroseconds( 1 );
			if ( counter == 255 )
			{
				break;
			}
		}
		laststate = digitalRead(PIN);
 
		if ( counter == 255 )
			break;
 		//ignore first 3 transitions
		if ( (i >= 4) && (i % 2 == 0) )
		{
			dht11_dat[j / 8] <<= 1;
			if ( counter > 50 )
				dht11_dat[j / 8] |= 1;
			j++;
		}
	}
 
	//chequea que se hayan leido 40 bits(5 bytes) y verifica la checksum en el ultimo byte
	if ( (j >= 40) && (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
	{
		//printf("Humedad = %d.%d %% Temperatura = %d.%d °C\n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
			//printf("j true es%d",j);
		return EXIT_SUCCESS;
	}else  {
		printf( "Data not good, skip\n" );
		//printf("%d",j);
		return EXIT_FAILURE;
	}

}

/*
/ Conecta con un servidor remoto a traves de socket INET
*/
int open_inet_con(char *host_name, char *port)
{
	struct sockaddr_in addr;
	uint16_t port_num;
	struct hostent *host;
	int fd;

    //cambiar:
	port_num = (u_int16_t)atoi(port);

	host = gethostbyname (host_name);
	if (host == NULL)
		return -1;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ((struct in_addr *)(host->h_addr))->s_addr;
	addr.sin_port = ntohs(port_num);
	
	fd = socket (AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		return -1;

	if (connect (fd, (struct sockaddr *)&addr, sizeof (addr)) == -1)
	{
		return -1;
	}

	return fd;
}

int accept_client_con(int socket)
{
	socklen_t client_len;
	struct sockaddr client_addr;
	int client;

	/*
	* La llamada a la funcion accept requiere que el parametro 
	* client_len contenga inicialmente el tamano de la
	* estructura client que se le pase. A la vuelta de la
	* funcion, esta variable contiene la longitud de la informacion
	* util devuelta en client
	*/
	client_len = sizeof (client_addr);
	client = accept (socket, &client_addr, &client_len);
	if (client == -1)
		return -1;

	/*
	* Se devuelve el fd en el que esta "enchufado" el client.
	*/
	return client;
}

/*
* Abre un socket servidor de tipo AF_INET. Devuelve el fd
*	del socket o -1 si hay probleamas
* Se pasa como parametro el nombre del servicio. Debe estar dado
* de alta en el fichero /etc/services
*/
int open_inet_sock(char *port)
{
	struct sockaddr_in addr;
	uint16_t port_num;
	int32_t fd;

	/*
	* se abre el socket
	*/
	fd = socket (AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	 	return -1;

    port_num = (u_int16_t)atoi(port);
	/*
	* Se rellenan los campos de la estructura addr, necesaria
	* para la llamada a la funcion bind()
	*/
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(port_num);
	addr.sin_addr.s_addr =INADDR_ANY;


    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

	if (bind (
			fd, 
			(struct sockaddr *)&addr, 
			sizeof (addr)) == -1)
	{
		close (fd);
		return -1;
	}

	/*
	* Se avisa al sistema que comience a atender llamadas de clients
	*/
	if (listen (fd, 1) == -1)
	{
		close (fd);
		return -1;
	}

	/*
	* Se devuelve el fd del socket servidor
	*/
	return fd;
}


int close_con(int socket)
{
	shutdown(socket, SHUT_RDWR);
	return 0;
}



