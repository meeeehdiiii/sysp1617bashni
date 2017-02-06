#include "performConnection.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF 300
char spielName[20];
int spielerNummer;
int spielerAnzahl;
char spielerName[20];
// char spielBrett[SIZEOFBOARD][SIZEOFBOARD][MAXHEIGHTTOWER];
char piecesList[2][22];
extern structSharedMemory *aktAddr;

int size;
int countSrvMsg = 1;
int msgCounter = 0;

// 1: prolog
// 2: spielverlauf/spielzug
int protokollPhase = 1;

int fd[2]; // for pipe

bool spVerlauf = false;

/***************************************************
* send() to server
***************************************************/
int sendToServer(int sock, char *string, int len) {
  if (send(sock, string, len, 0) == -1) {
    perror("send() failure\n");
  }
  printf("Client: %s\n", string);

  return 0;
}

/***********************************
* receive() from server
************************************/
int recvFromServer(int sock) {

  char serverMessage[BUF] = "";

  size = recv(sock, serverMessage, BUF, 0);
  if (size > 0) {
    msgCounter++;
    // printf("nachricht %i:\n%s", msgCounter, serverMessage);

    checkSrvrMsg(sock, serverMessage, size);

  } else {
    perror("Receive() failed!");
  }

  return 0;
}

/************************************************
*  -receiving messages from server
*  -sending messages to the server
************************************************/
int performConnection(int sock, char *id, int playerNummer, int shm) {
  shmat(shm, NULL, 0);
  printf("\n\n\nIntSpielerNummer aus dem shm: %i\n", aktAddr->IntSpielerNummer);

  printf("performConnection wird aufgerufen\n");
  char *version;
  version = "VERSION 2.0\n";
  char gameId[16];
  strcpy(gameId, "ID ");
  strcat(gameId, id);
  strcat(gameId, "\n");
  printf("game id ist: %s\n", gameId);

  char *anzahlSp;

  if (playerNummer == 3) {
    anzahlSp = "PLAYER\n";
  } else if (playerNummer == 0) {
    anzahlSp = "PLAYER 0\n";
  } else if (playerNummer == 1) {
    anzahlSp = "PLAYER 1\n";
  }

  printf("\n\n*********PROLOG_PHASE*********\n\n");

  recvFromServer(sock); // 1. server message
  sendToServer(sock, version, strlen(version));
  recvFromServer(sock); // 2. server message
  sendToServer(sock, gameId, strlen(gameId));
  recvFromServer(sock); // 3. server msg
  sendToServer(sock, anzahlSp, strlen(anzahlSp));

  recvFromServer(sock); // 4. server msg - goes into Spielverlauf.

  /********************
  SPIELVERLAUF-PHASE
  **********************/
  printf("\n\n*********SPIELVERLAUF_PHASE***********\n\n");

  // fclose(z);
  // printf("%d\n",spiel.papa);
  return 0;
}

/****************************************************************************
* if server message ends with /n, the whole message has been received correctly
*****************************************************************************/
void checkSrvrMsg(int sock, char *msg, int size) {
  if ((msg[size - 1] == '\n')) { // if message ends with /n
    checkPlusMinus(sock, msg);
  } else {
    srvMsgError();
  }
}

/*********************************************
* errorMessage if message is not
**********************************************/
void srvMsgError() {
  perror("Nachricht vom Server wurde nicht vollstaendig zugestellt.");
}

void checkPlusMinus(int sock, char *msg) {
  if (msg[0] == '+') { // if the message starts with +
    posMsgHandling(sock, msg);
  } else { // if the message starts with -
    negMsgHandling(msg);
  }
}

/*****************************
* handling each positive Message
******************************/
void posMsgHandling(int sock, char *msg) {

  char copyMessage[300];
  strcpy(copyMessage, msg);

  if (spVerlauf == false) {

    if (countSrvMsg == 1) { // first Server Message
      countSrvMsg++;
      firstMessageHandling(msg);
      return;

    } else if (countSrvMsg == 2) { // second Server Message
      countSrvMsg++;
      secondMessageHandling(msg);
      return;

    } else if (countSrvMsg == 3) { // third Server Message
      countSrvMsg++;
      thirdMessageHandling(msg);
      return;

    } else if (countSrvMsg == 4) { // fourth Server Message
      countSrvMsg++;
      fourthMessageHandling(msg, sock);
    }
  } else {

    spielVerlaufAndSpielZugPhase(sock, copyMessage);
    return;
  }
}

void spielVerlaufAndSpielZugPhase(int sock, char *msg) {
  // hier sollte abgefragt werden, ob gameOver, wait oder move empfangen wurde
  // und was zu tun ist.

  char msgCopy[400];
  strcpy(msgCopy, msg);

  printf("verlaufUndZugmethod: %d , %s\n", sock, msg);

  if (strncmp(msgCopy, "+ WAIT", 5) == 0) {
    printf("wait schicken....:\n");
    waitMethod(sock);
  } else if (strncmp(msgCopy, "+ MOVE ", 7) == 0) {
    printf("jetzt hama move...\n");
    // hier initializeSpielfeld() aufrufen;
    //
    moveMethod(sock, msg);
  } else if (strncmp(msgCopy, "+ GAMEOVER", 9) == 0) {
    printf("jetzt hama gameover...\n");
    gameOverMethod(msg);
  } else if (strncmp(msgCopy, "+ OKTHINK", 8) == 0) {
    printf("jetzt hama okthink...\n");
    reactAfterOKTHINK(sock);
  } else if (strncmp(msgCopy, "+ MOVEOK", 7) == 0) {
    printf("jetzt hama moveok...\n");
    recvFromServer(sock);
  }
}

/*********************************
* handling every negative Message
**********************************/
void negMsgHandling(char *msg) {
  printf("Negative Nachricht vom Server: Verbindung abgebrochen.\n");
  printf("Die zu behandelnde Nachricht lautet: \n%s\n", msg);
  // TODO eine if-Abfrage, die den gameId-Fehler behandelt, um dem User
  // mitzuteilen, dass die GameID falsch ist. Verbindug wird serverseits
  // abgebrochen. Ein exit failure waere dann noch perfekt, um das Programm
  // vollstaendig zu schliessen.
  char copyMessage[300];
  strcpy(copyMessage, msg);

  if (strncmp(copyMessage, "- Game does not exist", 20) == 0) {
    printf("Das Spiel existiert nicht - Wahrscheinliche Fehlerquelle: Falsche "
           "Game ID\n");
  } else if (strncmp(copyMessage, "- Not a valid game ID", 20) == 0) {
    printf("Die eingegebene Game ID ist nicht gueltig.\n");
  } else if (strncmp(copyMessage, "- No free player", 16) == 0) {
    printf("Es gibt keinen freien Spieler.\n");
  } else if (strncmp(copyMessage, "- Invalid Move: ", 16) == 0) {
    printf("Ungultiger Zug. Hoffentlich tritt dieser Fall nie auf. Sonst ist "
           "unsere KI zu doof\n");
    printf("TODO hier: reconnect zu Server! und hoffen, dass KI gueltigen Move "
           "schickt!\n");

  } else if (strncmp(copyMessage, "- TIMEOUT Be faster next time", 29) == 0) {
    printf("Timeout! todo: reconnect\n");
  } else if (strncmp(copyMessage, "- Internal error. Sorry & Bye", 29) == 0) {
    printf("Interner Fehler am Game Server. Sorry und Bye sagt er.\n");
  }

  printf("Die Verbindung zum Game Server wurde abgebrochen.\n");
  printf("Bashni wird beendet..\n\n");
  exit(EXIT_FAILURE);
}

/*****************************************************
* splitting a string with " " as delimeter to get words
*****************************************************/
int split(char *msg, int stelle, char *dortRein) {
  char delimiter[] = " ";
  char *word;
  int stell = 1;

  // initialisieren und ersten Abschnitt erstellen
  word = strtok(msg, delimiter);

  while (word != NULL) {
    // printf("abschnitt: %s\n", word);
    if (stell == stelle) {
      strcpy(dortRein, word);
      return 0;
    }
    stell++;
    word = strtok(NULL, delimiter);
  }
  return -1;
}
int splitForAt(char *msg, int stelle, char *dortRein) {
  char delimiter[] = "@";
  char *word;
  int stell = 1;

  // initialisieren und ersten Abschnitt erstellen
  word = strtok(msg, delimiter);

  while (word != NULL) {
    // printf("abschnitt: %s\n", word);
    if (stell == stelle) {
      strcpy(dortRein, word);
      return 0;
    }
    stell++;
    word = strtok(NULL, delimiter);
  }
  return -1;
}

/*******************************************************
* splitting a string with "\n" as delimeter to get a line
********************************************************/
int splitZeile(char *msg, int stelle, char *dortRein) {
  char delimiter[] = "\n";
  char *word;
  int stell = 1;

  // initialisieren und ersten Abschnitt erstellen
  word = strtok(msg, delimiter);

  while (word != NULL) {
    // printf("abschnitt: %s\n", word);
    if (stell == stelle) {
      strcpy(dortRein, word);
      return 0;
    }
    stell++;
    word = strtok(NULL, delimiter);
  }
  return -1;
}

void moveMethod(int sock, char *msg) {

  // TODO: //NOTE: // TODO: //NOTE:// TODO: //NOTE:// TODO: //NOTE:
  //-1.  checkPiecesList // <- da wird spielBrett-Array gesetzt.
  // 0.  nach checkPiecesList() den spielBrett-Array ins shm speichern!
  // 1.  Spielzug - Signal an thinker, damit er Spielzug schickt.
  // 2.  spielzug aus der pipe lesen
  // 3.  send thinking
  // 4.  receive okthink
  // 5.  send spielzug
  // 6.  last receiveFromServer();
  // Zusatz: Move-Counter - nach 200 Zuegen endet das Spiel automatisch!!!
  // Irgendwo noch Bedingung einfuegen! --> brauch ma nicht, bekommen wir vom
  // Server mitgeteilt und unser Client reagiert schon drauf!

  // initializeSpielfeld()
  spielfeldSchwarzWeissMachen();
  checkPiecesList(msg); //<- ruft zeilenbehandlung() auf
  // printfSpielfeld();

  printspielBrett();

  sendToServer(sock, "THINKING\n", strlen("THINKING\n"));
  recvFromServer(sock);

  // gameSchleife(sock);

  // printf("Hallo\n");
}

void reactAfterOKTHINK(int sock) {
  char spielZug[15];

  // select() Methode aufrufen, um permanent die Infos aus der Pipe zu lesen und
  // gleichzeitig
  // Nachrichten vom Gameserver zu empfangen und darauf zu reagieren
  kill(getppid(), SIGUSR1);
  printf("signal geschickt.\n");

  fd_set readfd;
  FD_ZERO(&readfd);
  FD_SET((sock), &readfd);
  int max = sock;
  FD_SET(fd[0], &readfd);

  if (fd[0] > max) {
    max = fd[0];
  }

  if (select(max + 1, &readfd, NULL, NULL, NULL) <= 0) {
    printf("errorMessage\n");
  } else {
    if (FD_ISSET(sock, &readfd)) {
      printf("sock daten behandeln\n");

      recvFromServer(sock);

    } else {
      printf("pipe daten behandeln\n");
      read(fd[0], spielZug, 100);
      printf("received from pipe: %s\n", spielZug);
      sendToServer(sock, spielZug, strlen(spielZug));
      if (spVerlauf == false) {
        spVerlauf = true;
      }
      recvFromServer(sock);
    }
  }
}

void gameOverMethod(char *msg) {

  char copyPlayer[300];
  strcpy(copyPlayer, msg);

  char ersteLinie[50];
  char zweiteLinie[50];
  char player1[20];
  char player2[20];
  char winner1[5];
  char winner2[5];

  int player1Stelle = strlen(copyPlayer) - 3;
  int player2Stelle = strlen(copyPlayer) - 2;

  splitZeile(copyPlayer, player1Stelle, ersteLinie);
  split(ersteLinie, 2, player1);
  split(ersteLinie, 3, winner1);

  splitZeile(copyPlayer, player2Stelle, zweiteLinie);
  split(zweiteLinie, 2, player2);
  split(zweiteLinie, 3, winner2);

  if (strcmp(winner1, "YES") >= 0 && strcmp(winner2, "YES") >= 0) {
    printf("Die Partie hat mit einem Unentschieden geendet\n");

  } else if (strcmp(winner1, "YES") >= 0 && strcmp(winner2, "NO") >= 0) {
    printf("PLAYER0 hat gewonnen\n");

  } else {
    printf("PLAYER1 hat gewonnen\n");
  }

  // game over
  printf("Game oveeeeeer. hihi\n");
}

void waitMethod(int sock) {
  // confirm wait with okwait
  sendToServer(sock, "OKWAIT\n", strlen("OKWAIT\n"));
  recvFromServer(sock);
}

void printspielBrett() {
  int z = 8;

  printf("\n\n\n");
  printf("   A B C D E F G H   \n");
  printf(" +-----------------+ \n");

  for (int i = 8; i > 0; i--) {
    printf("%i| ", z);
    for (int j = 1; j < 9; j++) {
      int y = getlast2(i, j);
      printf("%c ", aktAddr->spielBrett[j][i][y]);
    }
    printf("|%i", z);
    printf("\n");
    z--;
  }
  printf(" +-----------------+ \n");
  printf("   A B C D E F G H   \n");
  printf("\n");

  printf("White Towers\n\n");
  printf("============\n\n");

  // printf("%s\n", ptr);

  for (int i = 1; i < 9; i++) {
    for (int j = 1; j < 9; j++) {

      char *ptr;
      ptr = &aktAddr->spielBrett[i][j][0];

      if (ptr[0] == 'w') {
        printf("%c%i: %s\n", numberToLetter(i), j, ptr);

      } else if (ptr[0] == 'W') {
        printf("%c%i: %s\n", numberToLetter(i), j, ptr);
      }
    }
  }

  printf("\n");

  printf("Black Towers\n\n");
  printf("============\n\n");

  for (int i = 1; i < 9; i++) {
    for (int j = 1; j < 9; j++) {

      char *ptr;
      ptr = &aktAddr->spielBrett[i][j][0];

      if (ptr[0] == 'b') {
        printf("%c%i: %s\n", numberToLetter(i), j, ptr);

      } else if (ptr[0] == 'B') {
        printf("%c%i: %s\n", numberToLetter(i), j, ptr);
      }
    }
  }
}

void spielfeldSchwarzWeissMachen() {

  char hspielBrett[SIZEOFBOARD][SIZEOFBOARD][MAXHEIGHTTOWER] = {
      {"r", "r", "r", "r", "r", "r", "r", "r", "r", "r"},
      {"r", ".", "_", ".", "_", ".", "_", ".", "_", "r"},
      {"r", "_", ".", "_", ".", "_", ".", "_", ".", "r"},
      {"r", ".", "_", ".", "_", ".", "_", ".", "_", "r"},
      {"r", "_", ".", "_", ".", "_", ".", "_", ".", "r"},
      {"r", ".", "_", ".", "_", ".", "_", ".", "_", "r"},
      {"r", "_", ".", "_", ".", "_", ".", "_", ".", "r"},
      {"r", ".", "_", ".", "_", ".", "_", ".", "_", "r"},
      {"r", "_", ".", "_", ".", "_", ".", "_", ".", "r"},
      {"r", "r", "r", "r", "r", "r", "r", "r", "r", "r"}};
  memcpy(aktAddr->spielBrett, hspielBrett, sizeof(aktAddr->spielBrett));
}

void firstMessageHandling(char *msg) {
  char copyMsg[100];
  strcpy(copyMsg, msg);
  char accepting[9];
  char version[10];
  char copy[size];

  strcpy(copy, msg);

  split(msg, 5, accepting);

  if (strncmp(copyMsg, "+ MNM Gameserver", 14) == 0 &&
      strcmp(accepting, "accepting") == 0) {
    split(copy, 4, version);
    printf("Der MNM Gameserver %s hat die Verbindung angenommen.\n\n", version);
  }
}

void secondMessageHandling(char *msg) {

  if (strncmp(msg, "+ Client version accepted - please send Game-ID to join",
              54) == 0) {
    printf("Der Server hat Ihre Client Version akzeptiert. Bitte geben Sie "
           "eine gueltige Game-ID ein.\n\n");
  }
}

void thirdMessageHandling(char *msg) {

  if (strncmp(msg, "+ PLAYING Bashni", 15) == 0) {
    printf("Wir spielen Bashni.\n");
    char copy[size];
    char gameName[40];
    splitZeile(msg, 2, copy);
    split(copy, 2, gameName);
    printf("Dein Spiel heißt %s\n\n", gameName);
  } else {
    printf("Wir spielen leider kein Bashni. Spiel wird beendet.");
    exit(EXIT_FAILURE);
  }
}

void fourthMessageHandling(char *msg, int sock) {

  char msgFullAgain[size];
  strcpy(msgFullAgain, msg);

  int countLineOfMsg4 = 1;

  if (countLineOfMsg4 == 1) {

    char copyMsg[size];
    strcpy(copyMsg, msg);
    char copy0[size];
    splitZeile(msg, 1, copy0);
    char copy[size];
    char copy2[size];
    char copy3[size];
    strcpy(copy, copy0);
    strcpy(copy2, copy0);
    strcpy(copy3, copy0);

    char you[30];
    char number[20];

    char yourName[30];
    split(copy, 2, you);
    split(copy2, 3, number);
    split(copy3, 4, yourName);
    spielerNummer = atoi(number);
    aktAddr->IntSpielerNummer = spielerNummer;

    if (strcmp(you, "YOU") >= 0) {
      printf("Du bist %s und deine Spielnummer lautet: %s\n", yourName, number);
      strcpy(spielerName, yourName);
    }
    countLineOfMsg4++;
  }
  if (countLineOfMsg4 == 2) {

    char copy0[size];
    char copy[size];
    char copy2[size];
    strcpy(copy0, msgFullAgain);
    splitZeile(copy0, 2, copy0);
    strcpy(copy, copy0);
    strcpy(copy2, copy0);

    char total[5];
    split(copy, 2, total);

    if (strcmp(total, "TOTAL") >= 0) {
      char anzahl[2];
      split(copy2, 3, anzahl);
      spielerAnzahl = atoi(anzahl);
      printf("Anzahl der Spieler betraegt: %s\n", anzahl);
    }
    countLineOfMsg4++;
  }
  if (countLineOfMsg4 == 3) {
    /// third line of fourth message:
    char copyMsg[size];
    strcpy(msg, msgFullAgain);

    splitZeile(msg, 3, copyMsg);

    char nummerSp2[2];
    char nameSp2[15];
    char bereit[2];

    split(copyMsg, 2, nummerSp2);
    strcpy(copyMsg, msg);
    splitZeile(copyMsg, 3, copyMsg);
    split(copyMsg, 3, nameSp2);
    strcpy(copyMsg, msg);
    splitZeile(copyMsg, 3, copyMsg);
    split(copyMsg, 4, bereit);
    int bereitInt = atoi(bereit);

    split(copyMsg, 3, nameSp2);

    if (bereitInt == 0) {
      printf("Spieler %s (%s) ist noch nicht bereit.\n", nummerSp2, nameSp2);
      /*recvFromServer(sock);
      sendToServer(sock, "OKWAIT", strlen("OKWAIT"));
      recvFromServer(sock);
      */

    } else if (bereitInt == 1) {
      printf("Spieler %s(%s) ist  bereit.\n", nummerSp2, nameSp2);
    }

    char msgCopy[size];
    strcpy(msgCopy, msgFullAgain);

    strcpy(copyMsg, msgFullAgain);
    char fullAgain[250];
    strcpy(fullAgain, msgFullAgain);

    char *fourthLine;
    fourthLine = malloc(sizeof(char) * 50);
    char *endplayersYN;
    endplayersYN = malloc(sizeof(char) * 12);

    splitZeile(copyMsg, 4, fourthLine);
    split(copyMsg, 2, endplayersYN);

    if (strcmp(endplayersYN, "ENDPLAYERS") >= 0) {
      protokollPhase++;

      printf("**********ENDPLAYERS************\n\n");
    }

    free(fourthLine);
    free(endplayersYN);

    // check if wait, gameover or move:
    char lineFive[20];
    splitZeile(fullAgain, 5, lineFive);
    printf("lineFive: %s\n", lineFive);
    char mGw[10];
    split(lineFive, 2, mGw);

    spVerlauf = true;

    if (strcmp(mGw, "MOVE") >= 0) {

      printf("4.n: jetzt hamma MOVE, jetzt müssma thinking schicken\n");
      moveMethod(sock, msgCopy);

    } else if (strcmp(mGw, "WAIT") >= 0) {

      printf("4.n: jetzt hamma WAIT, jetzt müssma OKWAIT schicken\n");
      waitMethod(sock);

    } else if (strcmp(mGw, "GAMEOVER") >= 0) {

      printf("4.n: jetzt hamma GAMEOVER, jetzt müssma darauf reagieren\n");
      gameOverMethod(fullAgain);
    }
  }
}

void checkPiecesList(char *moveStringMessage) {
  //printf("nachricht ist folgende: %s\n", moveStringMessage);
  char delimeter[] = "\n";

  char *ptr;
  // int zeile = 1;

  ptr = strtok(moveStringMessage, delimeter);
  while (ptr != NULL) {
    char copy[30];
    strcpy(copy, ptr);
    // printf("zeile %i: %s\n", zeile++, copy);
    zeilenbehandlung(copy);
    ptr = strtok(NULL, delimeter);
  }
}

void zeilenbehandlung(char *line) {

  if (strncmp(line, "+ b@", 4) == 0) {

    int i = letterToNumber(line[4]);
    char zwei = line[5];
    int j = atoi(&zwei);

    char *ptr;
    ptr = &aktAddr->spielBrett[i][j][0];

    if (ptr[0] == '.') {
      ptr[0] = 'b';
    } else {
      strcat(ptr, "b");
    }
  }

  if (strncmp(line, "+ B@", 4) == 0) {
    int i = letterToNumber(line[4]);
    char zwei = line[5];
    int j = atoi(&zwei);

    char *ptr;
    ptr = &aktAddr->spielBrett[i][j][0];

    if (ptr[0] == '.') {
      ptr[0] = 'B';
    } else {
      strcat(ptr, "B");
    }
  }
  if (strncmp(line, "+ w@", 4) == 0) {

    int i = letterToNumber(line[4]);
    char zwei = line[5];
    int j = atoi(&zwei);

    char *ptr;
    ptr = &aktAddr->spielBrett[i][j][0];

    if (ptr[0] == '.') {
      ptr[0] = 'w';
    } else {
      strcat(ptr, "w");
    }
  }
  if (strncmp(line, "+ W@", 4) == 0) {

    int i = letterToNumber(line[4]);
    char zwei = line[5];
    int j = atoi(&zwei);

    char *ptr;
    ptr = &aktAddr->spielBrett[i][j][0];

    if (ptr[0] == '.') {
      ptr[0] = 'W';
    } else {
      strcat(ptr, "W");
    }
  }
}

char *splitByAt(char *line) {
  // // int zeile = 1;
  //
  // ptr = strtok(line, delimeter);
  // while (ptr != NULL) {
  //   // printf("zeile %i: %s\n", zeile++, ptr);
  //
  //   zeilenbehandlung(ptr);
  //   ptr = strtok(NULL, delimeter);
  // }

  char *zweiter;
  zweiter = malloc(sizeof(char) * 10);

  splitForAt(line, 2, zweiter);

  //printf("return von splitbyat: %s\n", zweiter);

  return zweiter;
}

int letterToNumber(char piece) {
  int first;

  switch (piece) {
  case 'A':
    first = 1;
    break;
  case 'B':
    first = 2;
    break;
  case 'C':
    first = 3;
    break;
  case 'D':
    first = 4;
    break;
  case 'E':
    first = 5;
    break;
  case 'F':
    first = 6;
    break;
  case 'G':
    first = 7;
    break;
  case 'H':
    first = 8;
    break;
  }
  return first;
}
char numberToLetter(int number) {

  char letter;

  switch (number) {
  case 1:
    letter = 'A';
    break;
  case 2:
    letter = 'B';
    break;
  case 3:
    letter = 'C';
    break;
  case 4:
    letter = 'D';
    break;
  case 5:
    letter = 'E';
    break;
  case 6:
    letter = 'F';
    break;
  case 7:
    letter = 'G';
    break;
  case 8:
    letter = 'H';
    break;
  default:
    return 'Z';
  }

  return letter;
}

// geht den turm durch und returned den int, wo oberste stein steht
int getlast2(int x, int y) {
  for (int i = 0; i <= MAXHEIGHTTOWER; i++) {
    if (aktAddr->spielBrett[x][y][i] == '\0') {
      return i - 1;
    }
  }
  return 1;
}
