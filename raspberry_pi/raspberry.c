/**
 * https://www.circuitbasics.com/how-to-set-up-the-dht11-humidity-sensor-on-the-raspberry-pi/
 * gcc -o rpi raspberry.c -lwiringPi
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAXTIMINGS	85
#define DHTPIN		7	//pin del sensor DHT11
#define PERIODO		3000  //1800000  //periodo de 30 min
#define CANT_BYTES	5
#define LIM_ERROR 	100
 
int read_dht11_dat(int *dht11_dat)
{
	uint8_t laststate	= HIGH;
	uint8_t counter		= 0;
	uint8_t j			= 0, i;
 
	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
 
	pinMode( DHTPIN, OUTPUT );
	digitalWrite( DHTPIN, LOW );
	delay( 18 );				//18 ms en bajo
	digitalWrite( DHTPIN, HIGH );
	delayMicroseconds( 25 );//entre 20 y 40 dice el datasheet
	pinMode( DHTPIN, INPUT );//lectura del pin
 
	for ( i = 0; i < MAXTIMINGS; i++ )
	{
		counter = 0;
		while ( digitalRead( DHTPIN ) == laststate )
		{
			counter++;
			delayMicroseconds( 1 );
			if ( counter == 255 )
			{
				break;
			}
		}
		laststate = digitalRead( DHTPIN );
 
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
			delay( 10 );
		}
		delay(PERIODO); 
	}
 
	return EXIT_SUCCESS;
}
