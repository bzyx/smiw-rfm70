#ifndef GRZEJNIK_H
#define GRZEJNIK_H

/*
 * Deklaracje parametrów protokołu dla modułu Grzejnik
 *
 * Dopuszczalne wartości:
 * NODE_ID - 01 - 99
 * FUNC_ID:
 *  odczytujące 01 - 49
 *  zapisujące  50 - 99
 */

#define GRZEJNIK_NODE_ID 01
#define GRZEJNIK_READ_TEMP 01
#define GRZEJNIK_SET_ON_TEMP 50
#define GRZEJNIK_SET_OFF_TEMP 51

#endif // GRZEJNIK_H
