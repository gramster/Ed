#include <stdarg.h>
#include "menu.h"

#define MNU_OPTS	(*(menu->Opts))
#define MNU_COL 	(menu->optx)
#define MNU_CHOICE(i)	MNU_OPTS[i].choice
#define MNU_CHOICES(i)	(MNU_OPTS[i].maxchoices)
#define MNU_VALUE(i)	MNU_OPTS[i].value


void Mnu_Process(menu /* ,... */)
MENU *menu;
{
WINDOW *Mnu_Win,*Old_Win=Win_Current;
int i=0,maxlen=0,maxops=0,tmplen=0,fixed=0;
unsigned char curs_key;
va_list args;

Mnu_Win=Win_Make(menu->title,menu->x,menu->y,menu->w,menu->h,
		SCR_TOP,SCR_CENTRE,SCR_NORML_ATTR,SCR_NORML_ATTR,
		WIN_BORDER | WIN_NONDESTRUCT);
Win_Activate(Mnu_Win);

va_start(args,menu);
for (i=0;MNU_OPTS[i].p_vals[0];i++)
	{
	maxops++;
	MNU_VALUE(i) = (unsigned short *) va_arg(args,unsigned short *);
        if (MNU_CHOICES(i))
	        {
		if (MNU_OPTS[i].a_vals[MNU_CHOICE(i)] != *(MNU_VALUE(i)))
		       	{
		        MNU_CHOICE(i) = 0;
		       	while ((MNU_OPTS[i].a_vals[MNU_CHOICE(i)] != *(MNU_VALUE(i)))
			    && (MNU_CHOICE(i) < MNU_CHOICES(i)))
				   ++MNU_CHOICE(i);
		    	}
		}
	else	{
		itoa(*(MNU_VALUE(i)),MNU_OPTS[i].p_vals[MNU_CHOICE(i)],10);
		fixed = 1; /* Menu items cannot be changed */
		}
	Win_FastPutS(i,1,MNU_OPTS[i].Item);
	Win_FastPutS(i,MNU_COL,MNU_OPTS[i].p_vals[MNU_CHOICE(i)]);
	tmplen = strlen(MNU_OPTS[i].p_vals[MNU_CHOICE(i)]);
	if (tmplen>maxlen) maxlen=tmplen;
	}
if (i <= menu->h)
	if (fixed)
		Win_FastPutS(i+1,MNU_COL-6,"<Esc to exit>");
	else Win_FastPutS(i,MNU_COL,"<Exit>");

va_end(args);
i=0;
curs_key=0;
if (fixed) while ((curs_key=Kb_GetCursKey()) != KB_ESC);
else while (i<maxops && curs_key!=KB_ESC)
	{
	tmplen=maxlen;
	while (tmplen--) Win_FastPutS(i,MNU_COL+tmplen," ");
	Win_FastPutS(i,MNU_COL,MNU_OPTS[i].p_vals[MNU_CHOICE(i)]);
	tmplen=strlen(MNU_OPTS[i].p_vals[MNU_CHOICE(i)]);
	if (tmplen>maxlen) maxlen=tmplen;
	Win_PutAttribute(SCR_INVRS_ATTR,MNU_COL,i,tmplen,1);
	curs_key = Kb_GetCursKey();
	Win_PutAttribute(SCR_NORML_ATTR,MNU_COL,i,maxlen,1);
	switch (curs_key)
	        {
	        case KB_LEFT:	if (MNU_CHOICE(i)-- == 0)
					MNU_CHOICE(i) = MNU_CHOICES(i)-1;
				break;
		case KB_RIGHT:	if (++MNU_CHOICE(i) >= MNU_CHOICES(i))
		       			MNU_CHOICE(i) = 0;
			 	break;
		case KB_DOWN:	i++; break;
		case KB_UP:	if (i) i--; break;
		default:	break;
	        }
	}
i=0;
if (curs_key!=KB_ESC)
	while (i<maxops)
		{
		*(MNU_VALUE(i)) = MNU_OPTS[i].a_vals[MNU_CHOICE(i)];
		i++;
		}
Win_Free(Mnu_Win);
Win_Activate(Old_Win);
}




