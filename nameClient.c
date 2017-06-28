

/*
 *  C Implementation: nameClient
 *
 * Description:
 *
 *
 * Author: MCarmen de Toro <mc@mc>, (C) 2015
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include "nameClient.h"

/**
 * Function that sets the field addr->sin_addr.s_addr from a host name
 * address.
 * @param addr struct where to set the address.
 * @param host the host name to be converted
 * @return -1 if there has been a problem during the conversion process.
 */
int setaddrbyname(struct sockaddr_in *addr, char *host)
{
  struct addrinfo hints, *res;
  int status;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(host, NULL, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }

  addr->sin_addr.s_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;

  freeaddrinfo(res);

  return 0;
}

/**
 * Function that gets the dns_file name and port options from the program
 * execution.
 * @param argc the number of execution parameters
 * @param argv the execution parameters
 * @param reference parameter to set the host name.
 * @param reference parameter to set the port. If no port is specified
 * the DEFAULT_PORT is returned.
 */
int getProgramOptions(int argc, char* argv[], char *host, int *_port)
{
  int param;
  *_port = DEFAULT_PORT;

  // We process the application execution parameters.
  while((param = getopt(argc, argv, "h:p:")) != -1){
    switch((char) param){
      case 'h':
        strcpy(host, optarg);
        break;
      case 'p':
        // Donat que hem inicialitzat amb valor DEFAULT_PORT (veure common.h)
        // la variable port, aquest codi nomes canvia el valor de port en cas
        // que haguem especificat un port diferent amb la opcio -p
        *_port = atoi(optarg);
        break;
      default:
        printf("Parametre %c desconegut\n\n", (char) param);
        return -1;
    }
  }

  return 0;
}

/**
 * Shows the menu options.
 */
void printa_menu()
{
  // Mostrem un menu perque l'usuari pugui triar quina opcio fer

  printf("\nAplicatiu per la gestió d'un DNS Server\n");
  printf("  0. Hola mon!\n");
  printf("  1. Llistat dominis\n");
  printf("  2. Consulta domini\n");
  printf("  3. Alta Ip\n");
  printf("  4. Alta Ips\n");
  printf("  5. Modificacio Ip\n");
  printf("  6. Baixa Ip\n");
  printf("  7. Baixa Domini\n");
  printf("  8. Sortir\n\n");
  printf("Escolliu una opcio: ");
}

/**
 * Function that sends a list request receives the list and displays it.
 * @param s The communications socket.
 */
void process_list_operation(int s)
{
  char buffer[DNS_TABLE_MAX_SIZE];
  int msg_size;

  //TODO: uncomment
  sendOpCodeMSG(s, MSG_LIST_RQ); //remember to implement sendOpCode in common.c, NOT DONE
  //memset(buffer, '\0', sizeof(buffer));
  memset(buffer, '\0', sizeof(buffer));

  //TODO: rebre missatge LIST, em queda fer-ho
  msg_size = recv(s, buffer, sizeof(buffer), 0);
  if(msg_size <= 0){
  	perror("Error recieving");
  	exit(-1);
  }
  //TODO: Descomentar la següent línia
  printDNSTableFromAnArrayOfBytes(buffer+sizeof(short), msg_size-sizeof(short));
}

void process_domain_operation(int s)
{
    char buffer[MAX_BUFF_SIZE];
    char bufferResponse[MAX_BUFF_SIZE];
    char zeroBuffer[MAX_BUFF_SIZE] = "0.0.0.0";
    char domain[MAX_BUFF_SIZE];
    int i = 0;

    memset(buffer, '\0', sizeof(buffer));
    stshort(MSG_DOMAIN_RQ, buffer);

    printf("Entra el nom del domini: ");
    scanf("%s", domain);

    memcpy(buffer+2, domain, sizeof(domain));

    if (send(s, buffer, sizeof(buffer),0) < 0){
        perror("ERROR send");
        exit(1);
    }

    if(recv(s, bufferResponse, sizeof(bufferResponse), 0) <= 0){
        perror("Error recieving!");
        exit(1);
    }

    if(ldshort(bufferResponse) == MSG_OP_ERR){
        process_error(bufferResponse);
    }
    else{
        printf("Domini:\n%s\nIPs\n", domain);
        while(i*sizeof(in_addr_t) < MAX_BUFF_SIZE && strcmp(inet_ntoa(ldaddr(bufferResponse+2+sizeof(in_addr_t)*i)), zeroBuffer) != 0){
            printf("%s\n", inet_ntoa(ldaddr(bufferResponse+2+sizeof(in_addr_t)*i)));
            i++;
        }
    }
}

void process_addDomain(int s, int nIP){
    char buffer[MAX_BUFF_SIZE];
    char bufferResponse[MAX_BUFF_SIZE];
    char domain[MAX_BUFF_SIZE];
    char IP[MAX_BUFF_SIZE];
    struct in_addr address;
    int i = 0;
    char *IPs;

    memset(buffer, '\0', sizeof(buffer));
    memset(bufferResponse, '\0', sizeof(bufferResponse));
    memset(domain, '\0', sizeof(domain));
    memset(IP, '\0', sizeof(IP));
    stshort(MSG_ADD_DOMAIN, buffer);

    printf("\nEntra el nom del domini: ");
    scanf("%s", domain);

    memcpy(buffer+2, domain, sizeof(domain));

    IPs = buffer + sizeof(short) + strlen(domain) + sizeof(char);

    if(nIP == 0){ //1 IP
        printf("\nEntra la IP: ");
        scanf("%s", IP);
        inet_aton(IP, &address);
        staddr(address, IPs);
    }
    else{ //n IPs
        do{
            memset(IP, '\0', sizeof(IP));
            printf("\nEntra la %d IP (0 per acabar): ", (i+1));
            scanf("%s", IP);
            inet_aton(IP, &address);
            if(strcmp(IP, "0") != 0)
                staddr(address, IPs + i * sizeof(in_addr_t));
            i++;
        }while(strcmp(IP, "0") != 0);
    }

    if (send(s, buffer, sizeof(buffer),0) < 0){
        perror("ERROR send");
        exit(1);
    }

    if(recv(s, bufferResponse, sizeof(buffer), 0) <= 0){
        perror("Error MSG_HELLO");
        exit(-1);
    }
    if(ldshort(bufferResponse) == MSG_OP_OK){
        printf("Operació realitzada amb èxit!");
    }
}

void process_changeDomain(int s){
    char buffer[MAX_BUFF_SIZE];
    char bufferResponse[MAX_BUFF_SIZE];
    char domain[MAX_BUFF_SIZE];
    char IP[MAX_BUFF_SIZE];
    char *IPs;
    struct in_addr address;

    memset(buffer, '\0', sizeof(buffer));
    memset(domain, '\0', sizeof(domain));
    memset(bufferResponse, '\0', sizeof(bufferResponse));
    memset(IP, '\0', sizeof(IP));
    stshort(MSG_CHANGE_DOMAIN, buffer);

    printf("\nEntra el nom del domini: ");
    scanf("%s", domain);

    memcpy(buffer+2, domain, strlen(domain));

    IPs = buffer + sizeof(short) + strlen(domain) + sizeof(char);

    printf("\nEntra la IP que vols canviar: ");
    scanf("%s", IP);
    inet_aton(IP, &address);
    staddr(address, IPs);

    IPs += sizeof(in_addr_t);

    memset(IP, '\0', sizeof(IP));
    printf("\nEntra la nova IP: ");
    scanf("%s", IP);
    inet_aton(IP, &address);
    staddr(address, IPs);

    if (send(s, buffer, sizeof(buffer),0) < 0){
        perror("ERROR send");
        exit(1);
    }

    if(recv(s, bufferResponse, sizeof(bufferResponse), 0) <= 0){
        perror("Error MSG_HELLO");
        exit(-1);
    }

    if(ldshort(bufferResponse) == MSG_OP_ERR){
        process_error(bufferResponse);
    }
    else{
        printf("Operació realitzada amb èxit!");
    }
}

/**
 * Function that process the menu option set by the user by calling
 * the function related to the menu option.
 * @param s The communications socket
 * @param option the menu option specified by the user.
 */

void process_hello(int s){
  char buffer[MAX_BUFF_SIZE];

  bzero(buffer, sizeof(buffer));
  sendOpCodeMSG(s, MSG_HELLO_RQ);

  if(recv(s, buffer, sizeof(buffer), 0) < 0){
    perror("Error MSG_HELLO");
    exit(-1);
  }

  printf("%s\n", buffer+2);

}
void process_error(char *buffer){
    unsigned short option;
    option = ldshort(buffer+2);

    switch(option){
     case ERR_1:
        printf("Error 1: IP inexistent!\n");
        break;
     case ERR_2:
        printf("Error 2: Domini inexistent!\n");
        break;
     default:
        printf("Invalid error!");
    }
}

void process_menu_option(int s, int option)
{
  switch(option){
    case MENU_OP_HELLO:
      process_hello(s);
      break;
    case MENU_OP_LIST:
      process_list_operation(s);
      break;
    case MENU_OP_DOMAIN_RQ:
      process_domain_operation(s);
      break;
    case MENU_OP_ADD_DOMAIN_IP:
        process_addDomain(s, 0);
        break;
    case MENU_OP_ADD_DOMAIN_IPS:
        process_addDomain(s, 1);
        break;
    case MENU_OP_CHANGE:
        process_changeDomain(s);
        break;
    case MENU_OP_FINISH:
      sendOpCodeMSG(s, MSG_FINISH);
      break;
    default:
      printf("Invalid menu option\n");
  }
}


int main(int argc, char *argv[])
{
  int port;
  char host[MAX_HOST_SIZE];
  int menu_option = 0;
  int ctrl_options;
  int s;

  struct sockaddr_in server_address;
  socklen_t addr_size;

  ctrl_options = getProgramOptions(argc, argv, host, &port);

  // Comprovem que s'hagi introduit un host. En cas contrari, terminem l'execucio de
  // l'aplicatiu
  if(ctrl_options<0){
    perror("No s'ha especificat el nom del servidor\n");
    return -1;
  }

  //TODO: setting up the socket for communication

  s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s == -1){
    perror("ERROR creating server_address\n");
    exit(-1);
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  if (setaddrbyname(&server_address, host) == -1){
    perror("Error in setaddrbyname\n");
    exit(-1);
  }

  addr_size = sizeof(server_address);

  if (connect(s, (struct sockaddr*)&server_address, addr_size)<0){
    perror("ERROR connect\n");
    exit(-1);
  }

  do{
    printa_menu();
    // getting the user input.
    scanf("%d",&menu_option);
    printf("\n\n");
    process_menu_option(s, menu_option);


  }while(menu_option != MENU_OP_FINISH); //end while(opcio)
  // TODO

  close(s);
  return 0;
}

