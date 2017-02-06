#ifndef PERFORMCONNECTION_H
#define PERFORMCONNECTION_H
#include <stdio.h>
#include <sys/types.h>
#define MAXHEIGHTTOWER 25
#define SIZEOFBOARD 10


typedef struct {
  char spielerNameStruct[20];
  int spielerNummerStruct;
  int bereit;
} spieler;


extern char spielName[];
extern int spielerNummer;
extern int spielerAnzahl;
extern char spielerName[];
extern int spielerNummer;
//extern char spielBrett[SIZEOFBOARD][SIZEOFBOARD][MAXHEIGHTTOWER];
extern char piecesList[2][22];

typedef struct {
  char spielBrett[SIZEOFBOARD][SIZEOFBOARD][MAXHEIGHTTOWER];
  char spielName[20];
  int IntSpielerNummer;
  int spielerAnzahl;
  pid_t papa;
  pid_t kind;
  spieler spieler1;
  spieler spieler2;
  char piecesList[2][22];
} structSharedMemory;

extern structSharedMemory *aktAddr;

int performConnection(int sock, char *id, int playerNummer, int shm);
int recvFromServer(int);
int sendToServer(int sock, char* string, int len);
void checkSrvrMsg(int, char *p, int s);
void srvMsgSuccess();
void srvMsgError();
void checkPlusMinus(int, char *msg);
void posMsgHandling(int, char*);
void negMsgHandling();
int split(char *msg, int stelle, char *dortRein);
int splitForAt(char *msg, int stelle, char *dortRein);
int splitZeile(char *msg, int stelle, char *dortRein);
int findAllocSize();
void createSharedMemory();
void deleteSharedMemory(int SHMID);
void spielVerlauf(int, char*);
void moveMethod(int, char*);
void gameOverMethod(char *msg);
void waitMethod(int);
void lastMessage(int);
void spielVerlaufAndSpielZugPhase(int, char*);
void printspielBrett();
void spielfeldSchwarzWeissMachen ();
void gameSchleife(int);
void reactAfterOKTHINK(int sock);
void firstMessageHandling(char *msg);
void secondMessageHandling(char *msg);
void thirdMessageHandling(char *msg);
void fourthMessageHandling(char *msg, int sock);
void checkPiecesList(char *moveStringMessage);
void zeilenbehandlung(char *line);
char* splitByAt(char *line);
int letterToNumber(char piece);
char numberToLetter(int x);
int getlast( int x, int y);
int getlast2(int x, int y);
#endif
