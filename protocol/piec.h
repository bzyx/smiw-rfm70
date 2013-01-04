#ifndef PIEC_H
#define PIEC_H

/*
 * Deklaracje parametrów protokołu dla modułu Piec
 *
 * Dopuszczalne wartości:
 * NODE_ID - 01 - 99
 * FUNC_ID:
 *  odczytujące 01 - 49
 *  zapisujące  50 - 99
 */

#define PIEC_NODE_ID 02
#define PIEC_READ_TEMP 01
#define PIEC_SET_ON_TEMP 50
#define PIEC_SET_OFF_TEMP 51

#endif // PIEC_H
