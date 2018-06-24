/**
 * @file mylib.h
 * @author Marco Colognese
*/

#ifndef MYLIB

#include <stdbool.h>
#define MYLIB



#define BUFLEN 10
#define STDIN 0
#define STDOUT 1

///STRUTTURA CONTENENTE I CAMPI DEI MESSAGGI SCAMBIATI TRA PADRE E FIGLIO
typedef struct messaggio{
	///Primo operando
	int val1;
	///Operazione da svolgere
	char op;
	///Secondo operando
	int val2;
	///Risultato
	int res;
	///Booleano per segnalare il termine di un calcolo al padre
	bool finish;
}share_mem;

int calcolo_char(char array[], int i);
void itoa(int i, char s[]);
void reverse(char s[]);

#endif
