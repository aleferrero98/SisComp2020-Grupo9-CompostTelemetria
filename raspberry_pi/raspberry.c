/**
 * https://www.circuitbasics.com/how-to-set-up-the-dht11-humidity-sensor-on-the-raspberry-pi/
 * gcc -o rpi raspberry.c -lwiringPi
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

int read_dht11_dat(int *dht11_dat);
void sensar(int var);
void pin_mode(int modo, int pin);
void clear_pin(int pin);
void set_pin(int pin);

long int periodo = PERIODO;
int remoto_activo = TRUE; 
 
int main( void )
{
	printf( "Raspberry Pi wiringPi DHT11 Temperature test program\n" );
	 
	int dht11_dat[CANT_BYTES] = { 0, 0, 0, 0, 0 }, ret, error;
	if ( wiringPiSetup() == -1 )
		exit(EXIT_FAILURE);

	while ( 1 )
	{
		error = 0;
		ret = read_dht11_dat(dht11_dat);
		while((ret == EXIT_FAILURE) && (error < LIM_ERROR)){ //hasta 100 mediciones erroneas insiste
			ret = read_dht11_dat(dht11_dat);
			error++;
			delay( 100 );
		}
		delay(periodo); 
	}
 
	return EXIT_SUCCESS;
}

void set_periodo_muestreo(int num){
	if(num > 0){
		periodo = num;
	}	
}
void toggle_control_remoto(void){
	remoto_activo = !remoto_activo;
}
void sensar(int var){
	int dht11_dat[CANT_BYTES] = { 0, 0, 0, 0, 0 }, ret, error = 0;
	
	ret = read_dht11_dat(dht11_dat);
	while((ret == EXIT_FAILURE) && (error < LIM_ERROR)){ //hasta 100 mediciones erroneas insiste
		ret = read_dht11_dat(dht11_dat);
		error++;
		delay( 10 );
	}
	if(var == HUMEDAD){
		//strcat(buf,)
		
	}else if(var == TEMPERATURA){
			
	}else if(var == ALL){
		
	}
		
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
	printf("mode %s\n", comando);
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
	printf("set %s\n", comando);
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
	printf("clear %s\n", comando);
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
	digitalWrite( 7 , HIGH );
	//set_pin(DHTGPIO);
	delayMicroseconds( 25 );//entre 20 y 40 dice el datasheet
	pinMode( 7, INPUT );//lectura del pin
	//pin_mode(INPUT, DHTGPIO);
	
	for ( i = 0; i < MAXTIMINGS; i++ )
	{
		counter = 0;
		while ( digitalRead( 7 ) == laststate )
		{
			counter++;
			delayMicroseconds( 1 );
			if ( counter == 255 )
			{
				break;
			}
		}
		laststate = digitalRead( 7 );
 
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
		printf("Humedad = %d.%d %% Temperatura = %d.%d °C\n",
			dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
			//printf("j true es%d",j);
		return EXIT_SUCCESS;
	}else  {
		printf( "Data not good, skip\n" );
		//printf("%d",j);
		return EXIT_FAILURE;
	}

}
