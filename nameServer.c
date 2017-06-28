
/*
*  C Implementation: nameServer
*
* Description:
*
*
* Author: MCarmen de Toro <mc@mc>, (C) 2015
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include "nameServer.h"



/* Reads a line ended with \n from the file pointer.  */
/* Return: a line ended not with an EOL but with a 0 or NULL if the end of the
file is reached */
char *readLine(FILE *file, char *line, int sizeOfLine)
{

  int line_length;

  if (fgets(line, sizeOfLine, file) != NULL)
  {
    line_length = strlen(line)-1;
    line[line_length] = 0;
  }
  else
  {
    line = NULL;
  }

  return line;
}


/**
 * Creates a DNSEntry variable from the content of a file line and links it
 * to the DNSTable.
 * @param line the line from the file to be parsed
 * @param delim the character between tokens.
 */
struct _DNSEntry* buildADNSEntryFromALine(char *line, char *token_delim)
{

  char *token;
  struct _IP *ip_struct = malloc(sizeof(struct _IP));
  struct _IP *last_ip_struct;
  struct _DNSEntry* dnsEntry = malloc(sizeof(struct _DNSEntry));
  int firstIP = 1;


  //getting the domain name
  token = strtok(line, token_delim);
  strcpy(dnsEntry->domainName, token);
  dnsEntry->numberOfIPs = 0;

  //getting the Ip's
  while ((token = strtok(NULL, token_delim)) != NULL)
  {
    ip_struct = malloc(sizeof(struct _IP));
    inet_aton((const char*)token, &(ip_struct->IP));
    ip_struct->nextIP = NULL;
    (dnsEntry->numberOfIPs)++;
    if (firstIP == 1)
    {
      dnsEntry->first_ip = ip_struct;
      last_ip_struct = ip_struct;
      firstIP = 0;
    }
    else
    {
      last_ip_struct->nextIP = ip_struct;
      last_ip_struct = ip_struct;
    }
  }

    return dnsEntry;
}

/* Reads a file with the dns information and loads into a _DNSTable structure.
Each line of the file is a DNS entry.
RETURNS: the DNS table */
struct _DNSTable* loadDNSTableFromFile(char *fileName)
{
  FILE *file;
  char line[1024];
  struct _DNSEntry *dnsEntry;
  struct _DNSEntry *lastDNSEntry;
  struct _DNSTable *dnsTable = malloc(sizeof(struct _DNSTable));
  int firstDNSEntry = 1;

  file = fopen(fileName, "r");
  if (file==NULL)
  {
    perror("Problems opening the file");
    printf("Errno: %d \n", errno);
  }
  else
  {
    //reading the following entries in the file
    while(readLine(file, line, sizeof(line)) != NULL)
    {
      dnsEntry = buildADNSEntryFromALine(line, " ");
      dnsEntry->nextDNSEntry = NULL;
      if (firstDNSEntry == 1)
      {
        dnsTable->first_DNSentry = dnsEntry;
        lastDNSEntry = dnsEntry;
        firstDNSEntry = 0;
      }
      else
      {
        lastDNSEntry->nextDNSEntry = dnsEntry;
        lastDNSEntry = dnsEntry;
      }
    }


    fclose(file);
  }

  return dnsTable;
}


/**
 * Calculates the number of bytes of the DNS table as a byte array format.
 * It does not  include the message identifier.
 * @param dnsTable a pointer to the DNSTable in memory.
 */
int getDNSTableSize(struct _DNSTable* dnsTable)
{
  int table_size = 0;
  int numberOfIPs_BYTES_SIZE = sizeof(short);

  struct _DNSEntry *dnsEntry;

  dnsEntry = dnsTable->first_DNSentry;
  if(dnsEntry != NULL)
  {
    do
    {
      table_size +=  ( strlen(dnsEntry->domainName) + SPACE_BYTE_SIZE +
        numberOfIPs_BYTES_SIZE + (dnsEntry->numberOfIPs * sizeof (in_addr_t)) );
    }while((dnsEntry=dnsEntry->nextDNSEntry) != NULL);
  }


  return table_size;
}



/*Return a pointer to the last character copied in next_DNSEntry_ptr + 1 */
/**
 * Converts the DNSEntry passed as a parameter into a byte array pointed by
 * next_DNSEntry_ptr. The representation will be
 * domain_name\0number_of_ips[4byte_ip]*].
 * @param dnsEntry the DNSEntry to be converted to a Byte Array.
 * @param next_DNSEntry_ptr a pointer to Byte Array where to start copying
 * the DNSEntry. The pointer moves to the end of the ByteArray representation.
 */
void dnsEntryToByteArray(struct _DNSEntry* dnsEntry, char **next_DNSEntry_ptr)
{

  struct _IP* pIP;

  fflush(stdout);

  strcpy(*next_DNSEntry_ptr, dnsEntry->domainName);
  //we leave one 0 between the name and the number of IP's of the domain
  *next_DNSEntry_ptr += (strlen(dnsEntry->domainName) + 1);
  stshort(dnsEntry->numberOfIPs, *next_DNSEntry_ptr);
  *next_DNSEntry_ptr += sizeof(short);
  if((pIP = dnsEntry->first_ip) != NULL)
  {
    do
    {
      staddr(pIP->IP, *next_DNSEntry_ptr);
      *next_DNSEntry_ptr += sizeof(in_addr_t);
    }while((pIP = pIP->nextIP) != NULL);
  }

}


/*Dumps the dnstable into a byte array*/
/*@Return a pointer to the byte array representing the DNS table */
/*@param dnsTable the table to be serialized into an array of byes */
/*@param _tableSize reference parameter that will be filled with the table size*/
char *dnsTableToByteArray(struct _DNSTable* dnsTable, int *_tableSize)
{
  int tableSize = getDNSTableSize(dnsTable);
  *_tableSize = tableSize;
  char *dns_as_byteArray = malloc(tableSize);
  char *next_dns_entry_in_the_dns_byteArray_ptr = dns_as_byteArray;
  struct _DNSEntry *dnsEntry;


  bzero(dns_as_byteArray, tableSize);

  dnsEntry = dnsTable->first_DNSentry;
  do
  {
    dnsEntryToByteArray(dnsEntry, &next_dns_entry_in_the_dns_byteArray_ptr);
  }while((dnsEntry=dnsEntry->nextDNSEntry) != NULL);

  return dns_as_byteArray;

}

/**
 * Function that gets the dns_file name and port options from the program
 * execution.
 * @param argc the number of execution parameters
 * @param argv the execution parameters
 * @param reference parameter to set the dns_file name.
 * @param reference parameter to set the port. If no port is specified
 * the DEFAULT_PORT is returned.
 */
int getProgramOptions(int argc, char* argv[], char *dns_file, int *_port)
{
  int param;
   *_port = DEFAULT_PORT;

  // We process the application execution parameters.
	while((param = getopt(argc, argv, "f:p:")) != -1){
		switch((char) param){
			case 'f':
				strcpy(dns_file, optarg);
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
 * Function that generates the array of bytes with the dnsTable data and
 * sends it.
 * @param s the socket connected to the client.
 * @param dnsTable the table with all the domains
 */
void process_LIST_RQ_msg(int sock, struct _DNSTable *dnsTable)
{
  char *dns_table_as_byteArray;
  char *msg;
  int dns_table_size;
  int msg_size = sizeof(short);

  dns_table_as_byteArray = dnsTableToByteArray(dnsTable, &dns_table_size);
  msg_size += dns_table_size;

  msg = malloc(msg_size);
  //TODO: set the operation code and the table data
  stshort(MSG_LIST, msg);
  memcpy(msg+2, dns_table_as_byteArray, dns_table_size);
  //TODO: send the message
  if (send(sock, msg, msg_size,0) < 0){
	perror("ERROR send");
	exit(1);
  }
}

/**
 * Receives and process the request from a client.
 * @param s the socket connected to the client.
 * @param dnsTable the table with all the domains
 * @return 1 if the user has exit the client application therefore the
 * connection whith the client has to be closed. 0 if the user is still
 * interacting with the client application.
 */

void process_DOMAIN_RQ(int s, struct _DNSTable *dnsTable, char *buffer){
    char domain[MAX_BUFF_SIZE];
    struct _DNSEntry* dnsEntry = malloc(sizeof(struct _DNSEntry));
    struct _IP* pIP;
    char msg[MAX_BUFF_SIZE];
    int trobat = 0;
    int i = 0;

    memset(msg, '\0', sizeof(msg));
    memset(domain, '\0', sizeof(domain));
    memcpy(domain, buffer+2, MAX_BUFF_SIZE);

    dnsEntry = dnsTable->first_DNSentry;

    while((trobat == 0) && (dnsEntry != NULL)){
        if(strcmp(dnsEntry->domainName, domain) == 0){
            trobat = 1;
        }
        else{
            dnsEntry = dnsEntry->nextDNSEntry;
        }
    }
    if(trobat == 1){
        stshort(MSG_IP_LIST,msg);
        if((pIP = dnsEntry->first_ip) != NULL)
        {
            do
            {
                staddr(pIP->IP, msg+2+sizeof(in_addr_t)*i);
                i++;
            }while((pIP = pIP->nextIP) != NULL);
        }
        if (send(s, msg, MAX_BUFF_SIZE,0) < 0){
            perror("ERROR send");
            exit(1);
        }
    }
    else{
        stshort(MSG_OP_ERR, msg);
        stshort(ERR_2, msg+2);
        if (send(s, msg, MAX_BUFF_SIZE,0) < 0){
            perror("ERROR send");
            exit(1);
        }
    }
}

void process_ADD_DOMAIN_RQ(int s, struct _DNSTable *dnsTable, char *buffer){
    char domain[MAX_BUFF_SIZE];
    char zeroBuffer[MAX_BUFF_SIZE] = "0.0.0.0";
    char *IPs;
    struct _DNSEntry* dnsEntry = malloc(sizeof(struct _DNSEntry));
    struct _IP *ip_struct = malloc(sizeof(struct _IP));
    char msg[MAX_BUFF_SIZE];
    int trobat = 0;

    memset(msg, '\0', sizeof(msg));
    memset(domain, '\0', sizeof(domain));
    memcpy(domain, buffer+2, strlen(buffer+2));

    IPs = buffer + sizeof(unsigned short) + strlen(domain) + sizeof(char);

    dnsEntry = dnsTable->first_DNSentry;

    while((trobat == 0) && (dnsEntry != NULL)){
        if(strcmp(dnsEntry->domainName, domain) == 0){
            trobat = 1;
        }
        else{
            dnsEntry = dnsEntry->nextDNSEntry;
        }
    }

    if(trobat == 1){
        while(*IPs != '\0'){
            ip_struct = malloc(sizeof(struct _IP));
            if(dnsEntry->first_ip == NULL){
                ip_struct->nextIP = NULL;
                ip_struct->IP = ldaddr(IPs);
                dnsEntry->first_ip = ip_struct;
            }
            else{
                ip_struct->nextIP = dnsEntry->first_ip;
                ip_struct->IP = ldaddr(IPs);
                dnsEntry->first_ip = ip_struct;
            }
            dnsEntry->numberOfIPs++;
            IPs += sizeof(in_addr_t);
        }
    }
    else{
        dnsEntry = malloc(sizeof(struct _DNSEntry));
        dnsEntry->numberOfIPs = 0;
        memset(dnsEntry->domainName, '\0', sizeof(dnsEntry->domainName));
        memcpy(dnsEntry->domainName, domain, strlen(domain));
        while(strcmp(inet_ntoa(ldaddr(IPs)), zeroBuffer) != 0){
            ip_struct = malloc(sizeof(struct _IP));
            if(dnsEntry->first_ip == NULL){
                ip_struct->nextIP = NULL;
                ip_struct->IP = ldaddr(IPs);
                dnsEntry->first_ip = ip_struct;
            }
            else{
                ip_struct->nextIP = dnsEntry->first_ip;
                ip_struct->IP = ldaddr(IPs);
                dnsEntry->first_ip = ip_struct;
            }
            dnsEntry->numberOfIPs++;
            IPs += sizeof(in_addr_t);
        }
        dnsEntry->nextDNSEntry = dnsTable->first_DNSentry;
        dnsTable->first_DNSentry = dnsEntry;
    }

    sendOpCodeMSG(s, MSG_OP_OK);
}

void process_CHANGE_DOMAIN_RQ(int s, struct _DNSTable *dnsTable, char *buffer){
    char domain[MAX_BUFF_SIZE];
    char *IPs;
    struct _DNSEntry* dnsEntry = malloc(sizeof(struct _DNSEntry));
    struct _IP *ip_struct = malloc(sizeof(struct _IP));
    char msg[MAX_BUFF_SIZE];
    char oldIP[MAX_BUFF_SIZE];
    int trobat = 0;

    memset(msg, '\0', sizeof(msg));
    memset(domain, '\0', sizeof(domain));
    memcpy(domain, buffer+2, strlen(buffer+2));

    IPs = buffer + sizeof(unsigned short) + strlen(domain) + sizeof(char);

    dnsEntry = dnsTable->first_DNSentry;

    while((trobat == 0) && (dnsEntry != NULL)){
        if(strcmp(dnsEntry->domainName, domain) == 0){
            trobat = 1;
        }
        else{
            dnsEntry = dnsEntry->nextDNSEntry;
        }
    }

    if(trobat == 0){
        stshort(MSG_OP_ERR, msg);
        stshort(ERR_2, msg+2);
        if (send(s, msg, MAX_BUFF_SIZE,0) < 0){
            perror("ERROR send");
            exit(1);
        }
    }
    else{
        trobat = 0;
        strcpy(oldIP, inet_ntoa(ldaddr(IPs)));
        ip_struct = dnsEntry->first_ip;
        while((trobat == 0) && (ip_struct != NULL)){
            if(strcmp(oldIP, inet_ntoa(ip_struct->IP)) == 0){
                trobat = 1;
                ip_struct->IP = ldaddr(IPs + sizeof(in_addr_t));
            }
            else{
                ip_struct = ip_struct->nextIP;
            }
        }
        if(trobat == 0){
            stshort(MSG_OP_ERR, msg);
            stshort(ERR_1, msg+2);
            if (send(s, msg, MAX_BUFF_SIZE,0) < 0){
                perror("ERROR send");
                exit(1);
            }
        }
        else{
            sendOpCodeMSG(s, MSG_OP_OK);
        }
    }
}

void process_hello_RQ(int s){
  char buffer[MAX_BUFF_SIZE];
  unsigned short op_code = MSG_HELLO;
  memset(buffer, '\0', sizeof(buffer));
  stshort(op_code, buffer);
  memcpy(buffer+2, "Hello World\0", sizeof("Hello World\0"));

  if (send(s, buffer, MAX_BUFF_SIZE,0) < 0){
	perror("ERROR send");
	exit(1);
  }

}


int process_msg(int sock, struct _DNSTable *dnsTable)
{
  unsigned short op_code;
  char buffer[MAX_BUFF_SIZE];
  int done = 0;
  int recived;
  bzero(buffer,sizeof(buffer));

  recived=recv(sock, buffer, sizeof(buffer), 0);

  if ( recived == -1){
	perror("ERROR MSG_HELLO");
	exit(-1);
  }

  op_code = ldshort(buffer);
  switch(op_code)
  {
    case MSG_HELLO_RQ:
      process_hello_RQ(sock);
      break;
    case MSG_LIST_RQ:
      process_LIST_RQ_msg(sock, dnsTable);
      break;
    case MSG_DOMAIN_RQ:
      process_DOMAIN_RQ(sock, dnsTable, buffer);
      break;
    case MSG_ADD_DOMAIN:
        process_ADD_DOMAIN_RQ(sock, dnsTable, buffer);
        break;
    case MSG_CHANGE_DOMAIN:
        process_CHANGE_DOMAIN_RQ(sock, dnsTable, buffer);
        break;
    case MSG_FINISH:
      done = 1;
      break;
    default:
      perror("Message code does not exist.\n");
  }

  return done;
}


int main (int argc, char * argv[])
{
  struct _DNSTable *dnsTable;
  int port ;
  char dns_file[MAX_FILE_NAME_SIZE] ;
  int finish = 0;
  int pid;

  int server_socket, accept_socket;
  struct sockaddr_in addr;

  getProgramOptions(argc, argv, dns_file, &port);

  dnsTable = loadDNSTableFromFile(dns_file);

  printDNSTable(dnsTable);
  //TODO: setting up the socket for communication

  server_socket = socket(PF_INET, SOCK_STREAM,IPPROTO_TCP);
  if (server_socket==-1){
  	printf("No se ha podido crear el socket");
	exit(-1);
  }

  addr.sin_family=AF_INET;
  addr.sin_port=htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  socklen_t addrlen = sizeof(addr);

  if (bind(server_socket, (struct sockaddr*)&addr, addrlen) < 0){
    perror("ERROR bind");
    exit(-1);
  }

  if(listen(server_socket, MAX_QUEUED_CON) < 0){
    perror("ERROR listen");
    exit(-1);
  }


  while (1) {
    accept_socket=accept(server_socket, (struct sockaddr*)&accept_socket, &addrlen);

    if (accept_socket<0){
    	perror("ERROR accept");
   	exit(-1);
    }

    pid = fork();
    if(pid == 0){
        while(finish != 1){
            finish = process_msg(accept_socket, dnsTable);
        }
    close(accept_socket);
    exit(0);
    }

    close(accept_socket);
    }

  return 0;
}


