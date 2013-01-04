#include "protocol/protocol.h"

#include "protocol/grzejnik.h"
#include "protocol/piec.h"

int ACTIVE_NODES_ID[] = {PIEC_NODE_ID, GRZEJNIK_NODE_ID};
int NODES_COUNT = 2;
int MIN_READ_FUNC_ID = 1;
int MAX_READ_FUNC_ID = GRZEJNIK_READ_TEMP; // wartosc taka sama jak PIEC_READ_TEMP
int MIN_SET_FUNC_ID = 50;
int MAX_SET_FUNC_ID = PIEC_SET_OFF_TEMP;

