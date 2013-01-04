#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
 * Prosta biblioteka/zbiór funkcji tworzący protokół komunikacyjny,
 * pomiędzy układami systemu sterowania kotłem C.O.
 *
 * Definicje struktur, globalnych ustawień konfiguracji, interfejs funkcji
 *
 */

#define BUFFER_LEN 32 // Wielkość bufora na transmiję w układach RFM70
#define VALUE_LEN 26  // Pojemość pola na dane w protokole
#define INVALID_NODE_ID 0 // Zabroniona wartość NODE_ID
#define INVALID_FUNCTION_ID 0 // Zabroniona wartość FUNC_ID

/*
 * Struktura danych do której dekodowany jest protokół
 */
typedef struct command {
    int nodeId;         // identyfikator układu
    int funcId;         // identyfikator funkcji w ramach układu
    char value[26];     // wartość pola danych
    int error;          // pole informujące o błędzie dekodowania
} command_t;

/*
 * Interfejs do użycia w programie
 */

void clearCommand(command_t *command);
void setCommandValues(command_t *command, int nodeId, int functionId, char* value);
int decodeMessage(char* message, command_t* command);
int encodeMessage(command_t* command, char* message);

#endif // PROTOCOL_H
