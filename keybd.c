#include "keybd.h"
#include "misc.h"

unsigned char Kb_GetCursKey(void)
{
unsigned char code;
while (TRUE)
	{
	while (code = getc_noecho())
		if (code == KB_ESC || code == KB_CRGRTN) return code;

	switch(code = KB_TOSPECIAL(getc_noecho()))
	       {
	       case KB_UP:
	       case KB_DOWN:
	       case KB_LEFT:
	       case KB_RIGHT: return code;
	       default: break;
	       }
	}
}

