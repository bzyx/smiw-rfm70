/*
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "protocol.h"
#include "piec.h"


int main(void)
{
    char bufor[32];
    command_t commandStruct;
    int returnValue = 0;

    //strcpy(&bufor, "1;00;read"); // FAIL bo 0 jako funkcja jest zabrionione
    strcpy(&bufor, "08;49;read"); // OK
    //strcpy(&bufor, "ab;49;read"); // FAIL node ma być od 01 - 99
    //strcpy(&bufor, "08;49;readBardzoDlugiSrtringZebyMialPowyzej27"); // FAIL bufor musi mieć więcej niż 32 żeby to skompilować

    // Dekoduje wiadomość z bufora na strukturę
    decodeMessage(&bufor, &commandStruct);
    // Wypisanie struktury
    printf("Node : %d \n", commandStruct.nodeId);
    printf("Func : %d \n", commandStruct.funcId);
    printf("Val  : %s \n", commandStruct.value);
    printf("Error: %d \n", commandStruct.error);
    printf("\n");

    // Czyszczenie przed dalszą pracą
    clearCommand(&commandStruct);
    // Ustawienie wartości dla pieca
    setCommandValues(&commandStruct,PIEC_NODE_ID,PIEC_READ_TEMP,"readTEMP12.32");
    // Stworzenie komunikatu
    returnValue = encodeMessage(&commandStruct, &bufor);
    // Wypisanie stworzonego komunikatu
    printf("(%d) %s \n",returnValue, bufor);

    printf("\n");
    system("pause");
    return 0;
}
 */


