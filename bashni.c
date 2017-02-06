#include "bashni.h"
#include "config.h"
#include "libraries/getIP.h"
#include "performConnection.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define GAMEKINDNAME "Bashni"
#define PORTNUMBER 1357
#define HOSTNAME "sysprak.priv.lab.nm.ifi.lmu.de"

char gegnerFTurm = '*';
char gegnerFDame = '*';
char eigeneFTurm = '*';
char eigeneFDame = '*';

bool firstMove = true;
bool simpleZug = true;
int letzteRichtung = 0;
int neueRichtung = 0;

extern char spielName[];
extern int spielerNummer;
extern int spielerAnzahl;

extern char spielerName[];

extern char piecesList[2][22];
char lokalSpielBrett[SIZEOFBOARD][SIZEOFBOARD][MAXHEIGHTTOWER];
extern char board[8][8];

char spielNameStruct[20];
int spielerNummerStruct;
int spielerAnzahlStruct;

char spielerNameStruct[20];
int spielerNummerStruct;

// Variablen fuer Groessenberechnung SharedMemory
int groesseSpielStruct;
int sizeSinglePlayer;
int shmSize;

int sock;
char *ip; // IP-Address

int size;

int IDSharedMemory;
structSharedMemory *aktAddr;
int *safetyNet;

// Anlegen der ersten SHM Segmente
structSharedMemory spiel;
spieler meinSpieler;

int fd[2]; // for pipe

int neuerI = 0; // neue i position vom stein nach schlagen
int neuerJ = 1; // neue j position vom stein nach schlagen
bool kannSchlagen = false;
bool returnSchlagen = false;
bool zwischenStand;
bool einfachZiehen = true;

// int anzahlSchlaege = 0;
int schlag = 0;
char spielzug[64];

//....................................................................................//
/****************************
* calculating the move
*****************************/
void think() {
  einfachZiehen = true;
  returnSchlagen = false;
  kannSchlagen = false;
  simpleZug = true;
  letzteRichtung = 0;
  neueRichtung = 0;
  firstMove = true;
  schlag = 0;
  memset(spielzug, 0, sizeof spielzug);
  strcpy(spielzug, "PLAY ");
  letzteRichtung = 0;
  // printf("~ ~ ~ ~ ~ ~ ~~~~~~~~~~~~~~wir sind im think vorm attach..\n");

  shmat(IDSharedMemory, NULL, 0);

  // printf("~ ~ ~ ~ ~ ~ ~~~~~~~~~~~~~~wir sind im think..\n");

  // ludwigsMethodeSetzeFarbe();

  // 1 - daten aus dem shm holen.
  // 2 - spielzug berechnen .
  // 3 - spielzug in die pipe schreiben.

  // memcpy(lokalSpielBrett, aktAddr->spielBrett, sizeof(lokalSpielBrett));

  for (int i = 0; i <= SIZEOFBOARD; i++) {
    for (int j = 0; j <= SIZEOFBOARD; j++) {
      strcpy(lokalSpielBrett[i][j], aktAddr->spielBrett[i][j]);
    }
  }

  ludwigsMethodeSetzeFarbe();

  for (int i = 8; i > 0; i--) {
    for (int j = 1; j < 9; j++) {
      // printf("schlagenderZug aufrufen\n");
      returnSchlagen = schlagenderZug(i, j);
      if (returnSchlagen == true) {
        writeIntoPipe();
        return;
      }
    }
  }
  if (einfachZiehen == true) {
    printf(
        "kein schlagenderZug gefunden. wir gehen in die einfacherZugmethode\n");
    einfacherZug();
    writeIntoPipe();
    return;
  }
  // printf("ACHTUNG: *wenn das jemals geprintet wird..\n");
}

void writeIntoPipe() {
  strcat(spielzug, "\n");
  if ((write(fd[1], &spielzug, sizeof(spielzug))) != sizeof(spielzug)) {
    perror("fehler beim schreiben in die pipe.");
    exit(EXIT_FAILURE);
  } else {
    printf("~~ ~ ~ ~  ~wir haben geschrieben\n");
    return;
  }
}

void ludwigsMethodeSetzeFarbe() {
  printf("aktAddr->IntSpielerNummer: %i\n", aktAddr->IntSpielerNummer);
  if (aktAddr->IntSpielerNummer == 0) {
    gegnerFTurm = 'b';
    gegnerFDame = 'B';
    eigeneFTurm = 'w';
    eigeneFDame = 'W';
  } else if (aktAddr->IntSpielerNummer == 1) {
    gegnerFTurm = 'w';
    gegnerFDame = 'W';
    eigeneFTurm = 'b';
    eigeneFDame = 'B';
  }
}

/*********************************
* gives back the value of the
* neighbouring field on the board
*********************************/
// gibt den Wert des nächsten Felds in einer Richtung aus
char getNext(int i, int j, int richtung) {

  if (richtung <= 4 && richtung >= 1) {
    char inhalt;
    int y = -50;
    switch (richtung) {
    case 1:
      y = getlast(i - 1, j + 1);
      inhalt = lokalSpielBrett[i - 1][j + 1][y];
      break;
    case 2:
      y = getlast(i + 1, j + 1);
      inhalt = lokalSpielBrett[i + 1][j + 1][y];
      break;
    case 3:
      y = getlast(i - 1, j - 1);
      inhalt = lokalSpielBrett[i - 1][j - 1][y];
      break;
    case 4:
      y = getlast(i + 1, j - 1);

      inhalt = lokalSpielBrett[i + 1][j - 1][y];
      break;
    }
    return inhalt;
  } else {
    // printf("Es gibt nur 4 Richtungen - Fehler. getNext!! \n");
    return 'f';
  }
}

void einfacherZug() {
  // printf("\n\n\njetzt samma in der einfacherZug-Methode\n");
  // printf(" einfacherZug methode ---- \n");

  for (int i = 9; i >= 0; i--) {
    for (int j = 0; j < 10; j++) {
      int y = getlast(i, j);
      printf("%c ", lokalSpielBrett[j][i][y]);
    }
    printf("\n");
  }

  simpleZug = true;
  int i;
  int j;

  printf("eigeneFTurm: %c\n", eigeneFTurm);

  while (simpleZug == true) { // 3

    for (i = 9; i >= 0; i--) {   // 1
      for (j = 0; j < 10; j++) { // 2
        // printf("blabla...\n");
        int y = getlast(i, j); // der oberste stein

        if (lokalSpielBrett[i][j][y] == eigeneFTurm) {
          //   printf("im if lokalSpielBrett[i][j][y] == eigeneFTurm\n";
          int x = 1;
          while (x < 5) { // 5
            // abprüfen, wo "vorne" ist
            if (eigeneFTurm == 'b') {
              if (x == 1 || x == 2) {
                x = 3;
              }
            } else if (eigeneFTurm == 'w') {
              if (x == 3 || x == 4) {
                x = 7;
              }
            }

            if (getNext(i, j, x) == '.') { // 6

              char erstes[3];                // eigene Position
              erstes[0] = numberToLetter(i); // buchstabe erste Position
              erstes[1] = j + '0';           // zahl erste Position
              erstes[2] = '\0';
              strcat(spielzug, erstes); // eigene Position

              char zweites[3]; // wohin er gehen will
              zweites[0] = ':';

              int iValue = getNextI(i, x);
              zweites[1] = numberToLetter(iValue); // buchstabe wohin
              int jValue = getNextJ(j, x);         // zahl wohin
              zweites[2] = jValue + '0';

              strcat(spielzug, zweites);

              //   printf("bis hier hin\n");
              printf("spielzug1: %s\n", spielzug);
              return;
            }
            x++; // 6
          }
          simpleZug = false;
        } else if (lokalSpielBrett[i][j][0] == eigeneFDame) { // 7
          int x = 1;
          while (x < 5) {
            if (getNext(i, j, x) == '.') {

              char erstes[2];
              erstes[0] = numberToLetter(i);
              erstes[1] = j;
              strcat(spielzug, erstes);

              char zweites[4];
              zweites[0] = ':';

              int iValue = getNextI(i, x);

              zweites[1] = numberToLetter(iValue);
              int jValue = getNextJ(j, x);

              zweites[2] = jValue;
              zweites[3] = '\0';
              strcat(spielzug, zweites);

              //   printf("bis hier hin\n");
              printf("spielzug: %s\n", spielzug);
              x++;
              return;
            }
          }

          simpleZug = false;

        } // 7 zu
      }   // 3 zu
    }     // 2 zu
  }       // 1
}

bool schlagenderZug(int i, int j) {

  // printf("%i. mal in der schlag-methode.\n", anzahlSchlaege++);

  /* der spielzug, der hier in die pipe geht, ist global, weil er weitere
     felder dran gehaengt bekommen kann.und ausserdem rekursiver aufruf wuerde
     ihn leeren, falls er lokal waere
  */
  neueRichtung = richtungsWechsel(letzteRichtung);
  // printf("neueRichtung: %i\n", neueRichtung);

  int y = getlast(i, j);

  // printf("spielBrett[]...%c\n", lokalSpielBrett[i][j][y - 1]);

  /******************************************
  * case Turm making hit move:
  ********************************************/
  if (lokalSpielBrett[i][j][y] == eigeneFTurm) {
    // printf("wir haben einen eigenen turm\n");
    int x = 1;
    while (x < 5) { // fuer die richtungen
      if (x == neueRichtung) {
        x++;
      }
      // ob diagonal ein gegner steht:
      if (getNext(i, j, x) == gegnerFDame || getNext(i, j, x) == gegnerFTurm) {
        int iValue = getNextI(i, x); // VON Gegner
        int jValue = getNextJ(j, x); // von gegner
        char tmp;
        tmp = getNext(iValue, jValue, x); // obs danach leer ist
        if (tmp == '.') {

          kannSchlagen = true; // wichtig. damit er in die pipe schreibt
          schlag++;
          if (schlag == 1) {

            char eigen[3];
            eigen[0] = numberToLetter(i);
            eigen[1] = j + '0';
            eigen[2] = '\0';
            strcat(spielzug, eigen);
          }

          int i2;
          int j2;
          char two[3];
          i2 = getNextI(iValue, x);    // x vom leeren feld
          j2 = getNextJ(jValue, x);    // y vom leeren feld
          two[1] = numberToLetter(i2); // leeres feld x in string rein schreiben
          two[2] = j2 + '0';           // y leeres feld in string

          two[0] = ':';
          //   two[1] = numberToLetter(iValue); // x von gegner
          //   two[2] = jValue + '0';           // y von gegner
          strcat(spielzug, two);

          printf("Schlag gefunden %s\n", two); // position vom gegner: ausgabe
          steinSchlagen(iValue, jValue);       // gegner, obersten loeschen
          steinVersetzen(i, j, i2, j2);
          if (eigeneFTurm == 'b' && j2 == 8) {
            dameSetzen(i2, j2);
          } else if (eigeneFTurm == 'w' && j2 == 1) {
            dameSetzen(i2, j2);
          }
          printf("wir ziehen nach: %s\n", two); // ausgabe leeres feld
          printf("alle zuege bisher: %s\n", spielzug);

          letzteRichtung = x;
          // REKURSION:
          einfachZiehen = false;
          schlagenderZug(i2, j2); // i und j muss das vom leeren feld sein.
        }
      }
      x++;
    }
    /******************************************
    * case Dame making hit move:
    ********************************************/
  } else if (lokalSpielBrett[i][j][y] == eigeneFDame) {
    // fuer die richtungen
    neueRichtung = richtungsWechsel(letzteRichtung);
    int x = 1;

    if (x == letzteRichtung) {
      x++;
    } // fuer die richtungen

    while (x < 5) {
      int neuerI = i;
      int neuerJ = j;
      for (int moving = 0; moving < 8; moving++) {
        // printf("HIER IST X IM SCHLAGENDEN ZUG: %i\n", x);
        char nextChar = getNext(neuerI, neuerJ, x);
        // printf("HIER IST X IM SCHLAGENDEN ZUG: %i\n", x);
        if (nextChar == '.') {
          // nächstes Feld
          neuerI = getNextI(i, x);
          neuerJ = getNextJ(j, x);
        } else if (nextChar == gegnerFTurm || nextChar == gegnerFDame) {
          // Variablen fuer Gegnerposition deklarieren und setzen
          int iValue;
          int jValue;
          iValue = getNextI(i, x);

          jValue = getNextJ(j, x);

          if (getNext(iValue, jValue, x) == '.') {
            if (schlag == 0) {
              char erster[3];
              erster[0] = numberToLetter(i);
              erster[1] = j + '0';
              erster[2] = '\0';
              strcat(spielzug, erster);
            }
            // freies Feld hinter zu schlagendem Gegner
            char two[4];
            neuerI = getNextI(iValue, x);
            neuerJ = getNextJ(jValue, x);
            // speichert die Variablen der danach zu "besuchenden" Position in
            // Array
            two[0] = ':';
            two[1] = numberToLetter(neuerI);
            two[2] = neuerJ + '0';
            two[3] = '\0';

            // haengt neuen Zug an
            strcat(spielzug, two);

            // nimmt die noetigen Veraenderungen am Spielbrett vor
            steinSchlagen(iValue, jValue);
            steinVersetzen(i, j, neuerI, neuerJ);
            kannSchlagen = true; // wichtig. damit er in die pipe schreibt
            schlag++;
            letzteRichtung = x;
            einfachZiehen = false;

            // rekursiver Funktionsaufruf
            schlagenderZug(neuerI, neuerJ);
          }
        } else if (nextChar == 'r') {
          x++;
        }
      }
      x++;
    }
  }

  // geht hier rein, wenn es mindestens einen Zug gibt.
  if (kannSchlagen == true) {

    return true;
  }
  return false; // falls nix zutrifft;
}

int richtungsWechsel(int r) {
  if (r > 0 && r < 5) {
    switch (r) {
    case 1:
      return 4;
    case 2:
      return 3;
    case 3:
      return 2;
    case 4:
      return 1;
    }
  }
  return -50;
}

// gibt den nächsten I-Wert in eine Richtung aus
int getNextI(int i, int richtung) {
  if (richtung <= 4 && richtung >= 1) {
    int next;
    switch (richtung) {
    case 1:
      next = i - 1;
      break;
    case 2:
      next = i + 1;
      break;
    case 3:
      next = i - 1;
      break;
    case 4:
      next = i + 1;
      break;
    }
    return next;
  } else {
    printf("Es gibt nur 4 Richtungen - Fehler. get NextI \n");
    return -50;
  }
  return -50;
}

// gibt den nächsten J-Wert in eine Richtung aus
int getNextJ(int j, int richtung) {
  if (richtung <= 4 && richtung >= 1) {
    int next;
    switch (richtung) {
    case 1:
      next = j + 1;
      break;
    case 3:
      next = j - 1;
      break;
    case 2:
      next = j + 1;
      break;
    case 4:
      next = j - 1;
      break;
    }
    return next;
  } else {
    printf("Es gibt nur 4 Richtungen - Fehler. getNextJ \n");
    return -50;
  }
}

// geht den turm durch und returned den int, wo oberste stein steht
int getlast(int x, int y) {
  for (int i = 0; i <= MAXHEIGHTTOWER; i++) {
    if (lokalSpielBrett[x][y][i] == '\0') {
      return i - 1;
    }
  }
  return 1;
}

// loescht den Stein aus dem ersten Feld (Parameter: i, j) und setzt in in
// das
// Zweite (iValue, jValue)
void steinVersetzen(int i, int j, int iValue, int jValue) {
  // Kopie unseres Spielbretts
  char zwischenKopie[MAXHEIGHTTOWER];
  // String kopieren
  strcpy(zwischenKopie, lokalSpielBrett[i][j]);
  // urspr. Feld loeschen
  lokalSpielBrett[i][j][0] = '.';
  lokalSpielBrett[i][j][1] = '\0';
  // neues Feld setzen
  strcpy(lokalSpielBrett[iValue][jValue], zwischenKopie);
}

/***************************************************************************
* call think() method if this (parent) process gets a signal from his child.
****************************************************************************/
void signal_handler(int signal) {

  if (signal == SIGUSR1) {
    printf("signal angekommen. think() wird aufgerufen:\n");
    think();
  }
}

/*************************************
* Connecting with Server
**************************************/
void connecting() {

  // Socket vorbereiten
  struct sockaddr_in address;

  address.sin_family = AF_INET;
  address.sin_port = htons(PORTNUMBER);

  sock = socket(AF_INET, SOCK_STREAM, 0);
  inet_aton(ip, &address.sin_addr);

  if (connect(sock, (struct sockaddr *)&address, sizeof(address)) == 0) {
    printf("Verbindung mit %s hergestellt.\n", inet_ntoa(address.sin_addr));
  } else {
    perror("Connection Error");
  }
}

/******************************************************************
* Creating Second Process with fork()
* The Child Process is the Connector and calls performConnection()
* The Parent Process is the Thinker (and have to wait for Child.)
*******************************************************************/
void forking(char *id, int playerNummer) {
  pid_t pid;
  pid = fork();
  if (pid < 0) { // fork- Error
    fprintf(stderr, "Fehler bei fork().\n");
  } else if (pid == 0) { // child: Connector; calls performConnection()
    printf("kind mit pid: %i\n", getpid());

    close(fd[1]); // close write side of the pipe
    performConnection(sock, id, playerNummer, IDSharedMemory);
    spiel.kind = pid;
    if ((int)shmdt(aktAddr) != 0) {
      printf("Fehler beim Detachen des Shared Memory\n");
    } else {
      shmdt(aktAddr);
    }
    exit(1);
  } else { // Parent process: soll erstmal nichts machen
    printf("papa mit pid: %i\n", getpid());
    close(fd[0]); // close the reading side of pipe

    int returnStatus;
    aktAddr = shmat(IDSharedMemory, NULL, 0);
    if (aktAddr == NULL) {
      perror("shmat fail");
    }
    // noch nicht genutzt: TODO: structSharedMemory lokalKopie;
    // TODO: Lokale Kopie von Spiel anlegen
    spiel.papa = pid;

    if (shmdt(aktAddr) != 0) {
      fprintf(stderr, "Fehler beim Detachen des Shared Memory\n");
      exit(EXIT_FAILURE);
    }
    waitpid(pid, &returnStatus, 0); // Parent process waits here for child

    deleteSharedMemory(IDSharedMemory); // deleting shm only in parent process
                                        // possible! because we created shm in
                                        // parent. child do not know about
                                        // creating shm
  }
}

void howToCallOurProgram() {
  printf("________________________________________________________________\n");
  printf("Hinweise zur Benutzung des Programms\n");
  printf("Programmaufruf: prog <gameID> <.conf>\n");
  printf("\nBeschreibung der Parameter:\n");
  printf("  <gameID> : eine gueltige 13-stellige GameID   (obligatorisch)\n");
  printf("  <.conf>  : eine config Datei                  (optional)\n");
  printf("________________________________________________________________\n");
}

/******************************************************
* main():
* Game Id (and config file) or nothing as parameters
******************************************************/
int main(int argc, char *argv[]) {

  ip = malloc(sizeof(int) * 50);

  char id[13];
  int playerNummer = 3;
  // wenn nur ein parameter(also die game id angegeben wurde)
  if (argc == 2 && (strlen(argv[1]) == 13)) {
    printf("_*_*_fall 1\n");
    idSuccess(""); // open default config file
    strcpy(id, argv[1]);
    printf("Ihre eingegebene ID lautet %s\n", id);
  } else if (argc == 3 && (strlen(argv[1]) == 13) &&
             (strncmp(argv[2], "1", 1) == 0 || strncmp(argv[2], "0", 1) == 0)) {
    printf("_*_*_fall 2\n");

    idSuccess(""); // open default config file
    strcpy(id, argv[1]);
    printf("Ihre eingegebene ID lautet %s\n", id);

    if (strncmp(argv[2], "0", 1) == 0) {
      playerNummer = 0;
    } else {
      playerNummer = 1;
    }

    // printf("die eingegebene spielernummer lautet: %c\n", &argv[2]);

    char file[20];
    strcpy(file, argv[2]);

    if (idSuccess(file)) {
      printf("Configdatei konnte geoeffnet werden.\n");
    }
    // wenn unser Programm nicht wie die obigen zwei Faelle aufgerufen wurde
  } else {
    printf("_*_*_fall 3\n");

    howToCallOurProgram();
    return EXIT_FAILURE;
  }
  createSharedMemory();

  // signal Handler for getting signals from other processes
  signal(SIGUSR1, signal_handler);

  // creating unnamed pipe. The child process inherits this pipe from his
  // dad
  // :D
  if (pipe(fd) < 0) {
    perror("Fehler beim Einrichten der Pipe.");
    exit(EXIT_FAILURE);
  } else {
    printf("pipe erstellt.\n");
  }

  /*********************************************************
  * If our program is called correctly, then go ahead here:
  **********************************************************/

  hostname_to_ip(HOSTNAME, ip); // calculate ip-addres
  connecting();                 // connect to Server
  free(ip);

  // memcpy(&aktAddr, &spiel, shmSize);
  forking(id, playerNummer); // create second process

  return 0;
}

// function findAllocSize to determine size of Shared Memory
int findAllocSize() {
  int temp;
  temp = sizeof(structSharedMemory);
  return temp;
}

void deleteSharedMemory(int SHMID) {
  if (shmctl(SHMID, IPC_RMID, NULL) < 0) {
    perror("\n\nShared Memory konnte nicht geloescht werden\n\n");
  } else {
    shmctl(SHMID, IPC_RMID, 0);
  }
}
// Function to create SharedMemory
void createSharedMemory() {
  // Weiteres Anlegen des spiel-Structs mithilfe spielerAnzahl
  // strcpy(spiel.spielNameStruct, spielName);
  // spiel.spielerAnzahlStruct = spielerAnzahl;
  // spiel.IntSpielerNummer = spielerNummer;
  // meinSpieler.spielerNummerStruct = spielerNummer;
  // strcpy(meinSpieler.spielerNameStruct, spielerName);
  // meinSpieler.bereit = 0;
  // spiel.spieler1 = meinSpieler;
  // // TODO: Bisher keine Infos ueber Spieler 2, daher erstmal diese etwas
  // // sinnfreie Variante
  // spiel.spieler2 = meinSpieler;
  // shmSize = findAllocSize();
  printf("%i benoetigt fuer SharedMemory\n", shmSize);
  IDSharedMemory = shmget(IPC_PRIVATE, 4000, IPC_CREAT | SHM_R | SHM_W);

  if (IDSharedMemory < 0) {
    perror("shmat fail");
  } else {
    aktAddr = shmat(IDSharedMemory, NULL, 0);
    printf("Spielbrett erfolgreich gespeichert\n");
  }
}

// Funktion, die aus einem Turm eine Dame macht
void dameSetzen(int i, int j) {
  if (eigeneFTurm == 'w') {
    // Hilfsvariable, in die die Stelle des letzten Zeichens gespeichert
    // wird
    int temp;
    // letztes Zeichen finden
    temp = getlast(i, j);
    // Dame setzen
    lokalSpielBrett[i][j][temp] = 'W';
  } else if (eigeneFTurm == 'b') {
    // Hilfsvariable, in die die Stelle des letzten Zeichens gespeichert
    // wird
    int temp;
    // letztes Zeichen finden
    temp = getlast(i, j);
    // Dame setzen
    lokalSpielBrett[i][j][temp] = 'B';
  }
}

// Löscht den letzten Stein von einer Position
// Speichern "unter" den eigenen Stein ist nicht noetig für die Spiellogik
void steinSchlagen(int iGegner, int jGegner) {
  // Hilfsvariable für Position des letzten stein
  int posLast;
  posLast = getlast(iGegner, jGegner);
  // abgefrage, ob Brett dann leer ist
  if (posLast == 0) {
    lokalSpielBrett[iGegner][jGegner][0] = '.';
    lokalSpielBrett[iGegner][jGegner][1] = '\0';
  } else {
    lokalSpielBrett[iGegner][jGegner][posLast] = '\0';
  }
}
