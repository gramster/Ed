/********************************
*   misc.c - General Functions	*
********************************/

#include "misc.h"
#include "screen.h"
#include "keybd.h"

int Gen_EditString(char c, char *start, char **end, char **now, int mode)
{
char *tptr;

if (c==KB_ESC) /* Escape - clear line */
       {
	*now=*end=start;
	*start=0;
       }
else if (c==KB_BACKSPC) /* Backspace */
      	{
	if ((tptr=*now-1) >= start)
		{
		tptr=*now-1;
		while (tptr<*end)
		        {
			*tptr = *(tptr+1);
			tptr++;
			}
		(*now)--; (*end)--;
		}
      	}
else if (c==0 || c==(unsigned char)0xE0) /* Extended key */
	{
	switch(KB_TOSPECIAL(getc_noecho()))
       		{
		case KB_DEL :	if (*now < *end) /* Delete under cursor */
	       				{
					tptr=*now;
					while (tptr<*end)
		        			{
						*tptr = *(tptr+1);
						tptr++;
						}
					(*end)--;
					if (*now > *end) *now=*end;
      					}
				break;
		case KB_INS:	mode=1-mode; /* Toggle insert mode */
				Scr_SetCursorShape(mode?SCR_BLOKCURSOR:SCR_LINECURSOR);
				break;
		case KB_RIGHT:	if (*now<*end) (*now)++;
				break;
		case KB_LEFT:	if (*now>start) (*now)--;
				break;
		case KB_HOME:	*now=start;
				break;
		case KB_END:	*now = *end;
				break;
		default:	beep();
				break;
		}
	}
else	{
	if (mode) /* If inserting, make space by shifting line along */
		{
		tptr = ++(*end);
		while (tptr>*now)
			{
			*tptr = *(tptr-1);
			tptr--;
			}
		}
	/* Put the character in at the appropriate position */
	*((*now)++) = c;
	*(*end)=0;
	}
return mode;
}

