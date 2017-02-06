#include "config.h"
#include <stdio.h>  //fopen()
#include <stdlib.h> //atoi()
#include <string.h> //strtok()

typedef struct {
  char hostname[50];
  int port;
  char spiel[20];
} structGameInfo;

int idSuccess(char datei[]) {

  char fileName[30];

  if (strcmp(datei, "") == 0) {
    strcpy(fileName, "client.conf");
  } else {
    strcpy(fileName, datei);
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  FILE *fp;
  fp = fopen(fileName, "r");

  if (fp == NULL) {
    printf("Datei konnte NICHT geoeffnet werden.\n");
    return -1;
  } else {
    // printf("Datei konnte geoeffnet werden.\n");

    while ((read = getline(&line, &len, fp)) != -1) {

      char copyline[50];
      strcpy(copyline, line);

      char value[30];
      char key[30];
      splitWords(copyline, 1, key);

      structGameInfo info;

      if (strcmp(key, "hostname") == 0) {
        strcpy(copyline, line);
        splitWords(copyline, 2, value);
        strcpy(info.hostname, value);
      }
      if (strcmp(key, "port") == 0) {
        strcpy(copyline, line);
        splitWords(copyline, 2, value);
        info.port = atoi(value);
      }
      if (strcmp(key, "spiel") == 0) {
        strcpy(copyline, line);
        splitWords(copyline, 2, value);
        strcpy(info.spiel, value);
      }

      // printf("key und value:%s = %s\n", key, value);
    }

    free(line);
    // Datei schliessen
    fclose(fp);
    return 0;
  }
}

int splitWords(char *msg, int stelle, char *dortRein) {
  char delimiter[] = " = ";
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
