/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


static int speedcheat = 0;	/* a well known hack allows to make JrPac Man run at four times */
				/* his usual speed. When we start the emulation, we check if the */
				/* hack can be applied, and set this flag accordingly. */


void jrpacman_init_machine(void)
{
	/* check if the loaded set of ROMs allows the Pac Man speed hack */
	if (RAM[0x180b] == 0xbe || RAM[0x180b] == 0x01)
		speedcheat = 1;
	else speedcheat = 0;
}



int jrpacman_interrupt(void)
{
	/* speed up cheat */
	if (speedcheat)
	{
		if (osd_key_pressed(OSD_KEY_LCONTROL) || osd_joy_pressed(OSD_JOY_FIRE))
			RAM[0x180b] = 0x01;
		else
			RAM[0x180b] = 0xbe;
	}

	return interrupt();
}
