#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "protocol.h"

/*
 * Prywatna funkcja sprawdzająca czy w polu danych
 * znajdują się tylko interesujące dane.
 * Zwraca 0 jeśli dane są poprawne lub
 * -1 w przypadku niepoprawnych
 */
int _validateStringValue(char* value){
    size_t i = 0;

    if (strlen(value) == 0) // Pole danych powinno mieć jakąś zawartość
        return -1;

    while (i < strlen(value))
    {
        if ( (isalnum(value[i]) == 0) &&   // znaki a-zA-Z0-9 są poprawne
             (value[i] != '.')        &&   // kropka dziesiętna jest poprawna
             (isspace(value[i]) == 0)   ){ // spacja jest poprawna //isblank
            return -1;
        }
        else
            i++;
    }
    return 0;
}

/*
 * Funkcja zerująca strukturę danych.
 */
void clearCommand(command_t *command){
    if (command == NULL)
        return;

    command->nodeId = INVALID_NODE_ID;
    command->funcId = INVALID_FUNCTION_ID;
    strcpy(command->value, "");
    command->error = 0;
}

/*
 * Funkcja ustawiająca wartości w strukturze danych protokołu.
 * Przeprowadzane jest również sprawdzenie poprwności danych
 */
void setCommandValues(command_t *command, int nodeId, int functionId, char* value){
    if (command == NULL)    //Podajemy istniejący obiekt struktury
        return;

    if(_validateStringValue(value) != 0)
        strcpy(command->value, "WRONG_VAL"); //Obsługa stuacji błednych danych
    else{
        strncpy(command->value, value, VALUE_LEN-1); //Długość bufora danych
        command->value[VALUE_LEN-1] = '\0';
    }

    command->nodeId = nodeId;
    command->funcId = functionId;
    command->error = 0;
}

/*
 * Fukcja dekodująca komunikat odebrany przez układ.
 * W rezultacie uzupełnia strukturę protokołu.
 * Informacjia o błędzie jest zapisana w strukturze, a także
 * zwrócona jako wartość wyjściowa funkcji.
 * W przypadku powodzenia jest to 0
 */
int decodeMessage(char* message, command_t* command){
    int i = 0;
    char* bufWsk;

    // Czyszczenie z starych danych
    clearCommand(command);

    // Sprawdzanie istnienia buforów
    if ( (message == NULL) || (command == NULL) )
        return -1;

    // Odczytana wiadomość jest zbyt długa
    if ( strlen(message) > BUFFER_LEN ){
        command->error = -1;
        return -1;
    }

    // Podział wiadomości wg. średników
    bufWsk = strtok(message,";");
    while (bufWsk != NULL){
        if (i == 0){
            if (bufWsk != NULL){
                command->nodeId = atoi(bufWsk);
            } else
                command->error = -2;
        } else if (i == 1){
            if (bufWsk != NULL){
                command->funcId = atoi(bufWsk);
            } else
                command->error = -3;
        } else if (i == 2){
            if (bufWsk != NULL){
                strncpy(command->value, bufWsk, VALUE_LEN);
                command->value[VALUE_LEN] = '\0';
            } else
                command->error = -4;
        }

        bufWsk = strtok(NULL, ";");
        i++;
    }

    // Powinny być 3 elementy
    if (i < 3){
        command->error = -5;
    }

    if (command->nodeId == INVALID_NODE_ID)
        command->error = -6;

    if(command->funcId == INVALID_FUNCTION_ID)
        command->error = -7;

    if(_validateStringValue(command->value) != 0)
        command->error = -8;

    return command->error;
}


/*
 * Fukcja tworząca wiadomość dla układu RFM70 z dostarczonej
 * struktury protokołu.
 */
int encodeMessage(command_t* command, char* message){

    if ( (message == NULL) || (command == NULL) )
        return -1;

    strcpy(message, "");

    sprintf(message, "%02d;%02d;%s", command->nodeId, command->funcId, command->value);
    //snprintf(message, BUFFER_LEN, "%02d;%02d;%s", command->nodeId, command->funcId, command->value);

    return 0;


}
