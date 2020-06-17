#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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

int sock_server;
int sock_client;

int get_med(char *buff)
{
    static float med = 0;
    if(buff == NULL){
        return -1;
    }
    med += 1.3;
    sprintf(buff, "time  %.2f porcent %.2f celcius", med, med*0.9);
    return 0;
}

int main(int argc, char *argv[])
{
    
    if(argc < 1)
    {
        exit(EXIT_FAILURE);
    }
    int ret;
    sock_server = open_inet_sock(argv[1]);
    if(sock_server == -1){
        perror("No se pudo crear socket");
        exit(EXIT_FAILURE);
    }
    sock_client = accept_client_con(sock_server);
    if(sock_client == -1){
        perror("No se pudo conectar con el cliente");
        exit(EXIT_FAILURE);
    }

    char *buff, hum[8], temp[8], aux1[512], *aux2;
    buff = malloc(512);
    int run = 1;
    while (run)
    {
        memset(buff, '\0', 512);
        recv(sock_client, buff, 512, 0);
        switch (buff[0])
        {
        case 'A':
            get_med(buff);
            send(sock_client, buff, strlen(buff), 0);
            break;
        case 'U':
            printf("%s\n", buff);
            strcpy(aux1, &buff[1]);
            strcpy(hum, strtok(aux1, "S"));
            strcpy(temp, strtok(NULL, "F"));
            printf("humedad lim: %s\ttemperatura lim: %s\n", hum, temp);
            break;
        default:
            printf("No debi recibir esto\n");
            run = 0;
            break;
        }
    } 
    close_con(sock_client);
    close_con(sock_server);
    return 0;
}