#ifndef BASHNI_H
#define BASHNI_H

#include <stdbool.h>

#define SIZEOFBOARD 10
#define MAXHEIGHTTOWER 25

char numberToLetter(int nbr);
int NumberToNumber(char *piece);
int findAllocSize();
char getNext( int i, int j, int richtung);
void einfacherZug();
bool schlagenderZug( int i, int j);
int getNextI(int j, int richtung);
int getNextJ(int j, int richtung);
void goThroughPieces();
int getLast( int x, int y);
void ludwigsMethodeSetzeFarbe();
void dameSetzen( int i, int j);
void steinSchlagen( int iGegner, int jGegner);
int richtungsWechsel(int r);
void steinVersetzen(int i, int j, int iValue, int jValue);
void writeIntoPipe();
#endif
