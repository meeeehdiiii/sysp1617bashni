#include <arpa/inet.h>
#include <errno.h>  //For errno - the error number
#include <netdb.h>  //hostent
#include <stdio.h>  //printf
#include <stdlib.h> //for exit(0);
#include <string.h> //memset
#include <sys/socket.h>

#define GAMEKINDNAME "Bashni"
#define PORTNUMBER 1357
#define HOSTNAME "sysprak.priv.lab.nm.ifi.lmu.de"

int hostname_to_ip(char *, char *);

/*
 * Get ip from domain name
 */
int hostname_to_ip(char *hostname, char *ip) {
  struct hostent *he;
  struct in_addr **addr_list;
  int i;

  if ((he = gethostbyname(hostname)) == NULL) {
    herror("Error: No such Host.");
    return -1;
  }

  addr_list = (struct in_addr **)he->h_addr_list;
  printf("%s resolved to ", hostname);
  for (i = 0; addr_list[i] != NULL; i++) {
    strcpy(ip, inet_ntoa(*addr_list[i]));
    printf("%s", inet_ntoa(*addr_list[i]));
  }

  printf("\n");

  return 0;
}
