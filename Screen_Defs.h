/*
 * Screen_Defs.h
 *
 *  Created on: 05-01-2013
 *      Author: Dell Vostro V131
 */

#ifndef SCREEN_DEFS_H_
#define SCREEN_DEFS_H_

static char screenLeft[4][17] = { "    Grzejnik", "Temp. akt     --", "Temp. wym     --", " " };
static char screenLeftTemplate[4][17] = { "Grzejnik", "Temp. akt:  %s", "Temp. wym:  %s", "" };

static char screenCenter[4][17] = { "   System C.O.", "Grzejnik:    CH-", "Piec:        CH+",
				"Temp. akt     --" };
static const char screenCenterTemplate[4][17] = { "System C.O. %s", "Grzejnik: CH- %s",
		"Piec: CH+  %s", "Temp. akt   %s" };

static char screenRight[4][17] = { "      Piec", "Temp. akt     --", "Temp. Wl      --", "Temp. Wyl     --" };
static char screenRightTemplate[4][17] = { "Piec", "Temp. akt:  %s", "Temp. Wl:   %s", "Temp. Wyl:  %s" };

static const char screenDebugTemplate[4][17] = { "Stan RFM: %s %s", "Nosna: %s",
		"Odbior: %s", "Licznik %s" };
static char screenDebug[4][17] =
		{ "Stan RFM: ", "Nosna: ", "Odbior: ", "Licznik " };

#endif /* SCREEN_DEFS_H_ */
