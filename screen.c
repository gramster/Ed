/***************************************/
/*					*/
/* Name: screen.c			*/
/*					*/
/* Author: G. Wheeler			*/
/*					*/
/* Date: June 1988 (version 1)		*/
/*	  December 1988 (version 2)	*/
/*					*/
/***************************************/

#include "misc.h"
#include "screen.h"
#include "errors.h"
#include "keybd.h"

#include <alloc.h>
#define farmalloc	malloc
#define farfree(p)	free(p)

unsigned short far *scr_base; 		/* base address of video memory */
unsigned short scr_fbase;     		/* 'Fast' segment for assembly  */

static unsigned short scr_maxcursor; 	/* Maximum scan line for cursor -
					  7 for CGA; 13 for mono/EGA 	*/

static int DOSrow,DOScol,		/* Original cursor attributes	*/
	DOSshape;

WINDOW _far *Win_Current = (WINDOW _far *)NULL;

static char TLCORNER=218, TRCORNER=191,	/* Window Border drawing	*/
	BLCORNER=192,BRCORNER=217,	/*  character set		*/
	THLINE=196, BHLINE=196,
	RVLINE=179, LVLINE=179;

static struct _WinList			/* Linked list of windows	*/
	{
	WINDOW _far *w;
	struct _WinList _far *next;
	short open;
	} _far *_ActiveWindowList = (struct _WinList _far *)NULL;

#define LISTELTSZ	sizeof(struct _WinList)


static unsigned short _far *_Scr_Preimage = (unsigned short _far *)NULL;

/**********************************************************************

	Low level Functions

***********************************************************************/

void _far *Scr_BlockSave(unsigned short far *from, unsigned short _far *to,
				unsigned short rows, unsigned short cols)
{
	unsigned short _far *rtn;
	if (to == (void _far *)0)
		rtn = to = (void _far *) farmalloc(2 * rows * cols);

	if (to)
		while (rows--)
			{
			movedata(SEGMENT(from),OFFSET(from),
			       SEGMENT(to),OFFSET(to),2*cols);
			from += 80; to += cols;
			}
	else Error(ERR_MEMFAIL,"Scr_BlockSave");
	return (rtn);
}

void Scr_BlockRestore(unsigned short _far *from, unsigned short far *to,
		unsigned short rows, unsigned short cols)
{
	if (from)
		{
	 	while (rows--)
			{
			movedata(SEGMENT(from),OFFSET(from),
			       SEGMENT(to),OFFSET(to), 2*cols);
			from += cols; to += 80;
			}
		farfree(from);
		}
}



static void _Block_Clear(unsigned short ulx,unsigned short uly,
			unsigned short brx,unsigned short bry,char attr)
{
	union REGS reg;
	reg.h.ch = uly;
	reg.h.cl = ulx;
	reg.h.dh = bry;
	reg.h.dl = brx;
	reg.h.bh = attr;
	reg.x.ax = 0x0600;
	int86(0x10,&reg,&reg);
}

static void _BlockMove(unsigned short from_offset,unsigned short to_offset,
			unsigned short width, unsigned short height)
{
	from_offset *= 2;
	to_offset *= 2;
	if (height>0) while (height--)
		{
		fmovescrmem(from_offset,to_offset,width);
		from_offset += 160;
		to_offset += 160;
		}
}


/*****************************************************************

	Initialisation and Termination Functions

******************************************************************/

void Scr_InitSystem(void)
{
	union REGS rgs;
	static int firsttime=1;
	if (firsttime)
		{
		int86(0x11,&rgs,&rgs); /* BIOS equipment flag service */
		if ((rgs.x.ax & 0x0030) != 0x0030) /* CGA? */
			{
			scr_base=(unsigned short far *) SCR_CGABASE;
			scr_fbase=0xB800;
			scr_maxcursor = 7;
			}
		else	{
			scr_base=(unsigned short far *) SCR_MONOBASE;
			scr_fbase=0xB000;
			scr_maxcursor = 13;
			}
		firsttime = 0;
		Scr_GetCursor(&DOSrow,&DOScol,&DOSshape);
		Scr_SaveScreen(_Scr_Preimage);
		}
}


void Scr_EndSystem(void)
{
	struct _WinList _far *wlnow = _ActiveWindowList;
	struct _WinList _far *wltmp;
	while (wlnow)
	       {
	       Win_Free(wlnow->w);
	       wlnow = wlnow->next;
	       }
	wlnow = _ActiveWindowList;
	while (wlnow)
		{
		wltmp = wlnow;
		wlnow = wlnow->next;
		farfree(wltmp);
		}
	Scr_RestoreScreen(_Scr_Preimage);
	Scr_PutCursor(DOSrow,DOScol);
	Scr_SetCursorShape(DOSshape);
}

/**************************************************************

	General Screen Functions

**************************************************************/

void Scr_ClearAll(void)
{
	ffillscrmem(0,2000,SCR_CHAR(' ',SCR_NORML_ATTR));
}



void Scr_SetCursorShape(int shape)
{
	union REGS rg;
	rg.h.ah=1;		/* BIOS service	*/
	switch(shape)
		{
		case SCR_BLOKCURSOR:	rg.h.ch = 0;
					break;
		case SCR_LINECURSOR:	rg.h.ch = scr_maxcursor-2;
					break;
		default:		rg.h.ch = shape;
		}
	rg.h.cl=scr_maxcursor-1;		/* End line	*/
	int86(0x10,&rg,&rg);
}




void Scr_PutCursor(int row, int col)
{
	union REGS rg;
	rg.h.ah=2;		/* BIOS service	*/
	rg.h.bh=0;		/* video page	*/
	rg.h.dh=row;		/* row		*/
	rg.h.dl=col;		/* column	*/
	int86(0x10,&rg,&rg);
}





void Scr_GetCursor(int *row, int *col, int *shape)
{
	union REGS rg;
	rg.h.ah=3;		/* BIOS service	*/
	rg.h.bh=0;		/* video page	*/
	int86(0x10,&rg,&rg);
	*row=rg.h.dh;
	*col=rg.h.dl;
	*shape = (rg.h.ch ? SCR_LINECURSOR : SCR_BLOKCURSOR);
}

/*******************************************************************

		Window Routines

*********************************************************************/


/********************************************
	Save window on heap or in
	user-supplied buffer area.
*********************************************/

static void _Win_Save(WINDOW _far *win)
{
	unsigned short far *start=win->start;
	unsigned short h = win->h, w = win->w;
	if (win->flags & WIN_BORDER)
		{
		start -= 81L;
		h += 2;
		w += 2;
		}
	win->preimage = Scr_BlockSave(start,win->preimage,h,w);
}

/******************************************
	Restore window from heap
	or user-supplied buffer.
*******************************************/

static void _Win_Restore(WINDOW _far *win)
{
	unsigned short far *start=win->start;
	unsigned short h = win->h, w = win->w;
	if (win->flags & WIN_BORDER)
		{
		start -= 81L;
		h += 2;
		w += 2;
		}
	 if (win)
		Scr_BlockRestore(win->preimage,start,h,w);
}

/***********************************************/
/* Window Border Drawing			*/
/***********************************************/

void Win_1LineBorderSet()
{
	TLCORNER=218; TRCORNER=191; BLCORNER=192; BRCORNER=217;
	THLINE=BHLINE=196;    	    RVLINE=LVLINE=179;
}



void Win_2LineBorderSet()
{
	TLCORNER=201; TRCORNER=187; BLCORNER=200; BRCORNER=188;
	THLINE=BHLINE=205;          RVLINE=LVLINE=186;
}



void Win_BorderSet(char tl, char tr, char bl, char br,
	char th, char bh, char rv, char lv)
{
	TLCORNER=tl; TRCORNER=tr; BLCORNER=bl; BRCORNER=br;
	THLINE=th;   BHLINE=bh;   RVLINE=rv;   LVLINE=lv;
}




static void _Win_Draw_Border(WINDOW _far *win)
{
	register int i;
	int d1=win->w + 1;
	int d2=80 - d1;
	register char a = win->ba;
	register unsigned short far *scr_tmp =
	       (unsigned short far *)(win->start)-81L;

	*scr_tmp++ = SCR_CHAR(TLCORNER,a);
	for (i=0; i< win->w; ++i) *scr_tmp++ = SCR_CHAR(THLINE,a);
	*scr_tmp = SCR_CHAR(TRCORNER,a);
	scr_tmp += d2;
	for (i=0; i< win->h; ++i)
		{
		*scr_tmp = SCR_CHAR(LVLINE,a); scr_tmp += d1;
		*scr_tmp = SCR_CHAR(RVLINE,a); scr_tmp += d2;
		}
	*scr_tmp++ = SCR_CHAR(BLCORNER,a);
	for (i=0; i< win->w; ++i) *scr_tmp++ = SCR_CHAR(BHLINE,a);
	*scr_tmp = SCR_CHAR(BRCORNER,a);
}


/********************************/
/* Create and Free Windows	*/
/********************************/

WINDOW _far *Win_Make(STRING title, int left, int top, int width, int height,
	int titleplace, int titlealign,
	unsigned char boxattr, unsigned char attr, unsigned short flags)
{
	WINDOW _far *win=(WINDOW _far *)0;
	unsigned short far *scr_tmp;
	struct _WinList _far *wl_tmp = (struct _WinList _far *)0;

	if ((win=(WINDOW _far *)farmalloc(sizeof(WINDOW))) &&
	       (wl_tmp = (struct _WinList _far *)
			farmalloc(LISTELTSZ)))
		{
		/* Insert at FRONT of list */
		wl_tmp->next = _ActiveWindowList;
		wl_tmp->w = win;
		wl_tmp->open =1;
		_ActiveWindowList = wl_tmp;

		win->x = left;
		win->y = top;
		win->h = height;
		win->w = width;
		win->start = (unsigned short far *)
		       FAR_PTR( scr_fbase , 160*top + left*2);
		win->preimage = (void _far *)0;
		win->r = win->c = 0; /* Initially cursor is at top left */
	        win->ba = boxattr;
		win->ia = win->ca = attr;
		win->tabsize = 8;
		win->flags = flags;

		if ((flags & WIN_NONDESTRUCT) == WIN_NONDESTRUCT)
		       _Win_Save(win);

		if ((flags & WIN_BORDER) == WIN_NOBORDER) return win;
		}
	else Error(ERR_MEMFAIL,"Win_Make");

/* Draw the window border */

	_Win_Draw_Border(win);

/* Print window title */

	scr_tmp = (unsigned short far *)(win->start);

	switch(titleplace)
		{
		case SCR_TOP:	scr_tmp -= 80L; break;
		case SCR_BOTTOM:scr_tmp += height*80;
		default:	break;
       	}
	switch(titlealign)
		{
		case SCR_LEFT:	break;
		case SCR_RIGHT: scr_tmp += width-strlen(title);
				break;
		case SCR_CENTRE:
		default:       	scr_tmp += (width - strlen(title))/2;
				break;
	       }

	while (*title)	*scr_tmp++ = SCR_CHAR(*title++,boxattr);

	return win;
}

/*******************************************************/

void Win_Free(WINDOW _far *win)
{
	struct _WinList _far *wl_tmp = _ActiveWindowList;
	while (wl_tmp)
		{
		if (wl_tmp->w == win)
			{
			if (wl_tmp->open)
				{
				_Win_Restore(win);
				farfree(win);
				wl_tmp->open = 0;
				}
			break;
			}
		wl_tmp = wl_tmp->next;
		}
}

/*********************************************************************/

void Win_Activate(WINDOW _far *win)
{
	Win_Current = win;
	if ((Win_Current->flags & WIN_BLOKCURSOR) == WIN_BLOKCURSOR)
	       Scr_SetCursorShape(SCR_BLOKCURSOR);
	else Scr_SetCursorShape(SCR_LINECURSOR);
	Win_PlaceCursor();
}

/*****************************************************/

void Win_PutCursor(int r, int c)
{
	WIN_ROW = r;
	WIN_COL = c;
	Win_PlaceCursor();
}

void Win_SetFlags(unsigned short mask)
{
	Win_Current -> flags |= mask;
	if (mask & (WIN_NONDESTRUCT | WIN_BORDER))
	       Error(ERR_BAD_ARGUMENT,"Win_SetFlags");
	if ((mask & WIN_BLOKCURSOR) == WIN_BLOKCURSOR)
		Scr_SetCursorShape(SCR_BLOKCURSOR);
}

void Win_ClearFlags(unsigned short mask)
{
	Win_Current -> flags ^= mask;
	if (mask & (WIN_NONDESTRUCT | WIN_BORDER))
	       Error(ERR_BAD_ARGUMENT,"Win_SetFlags");
	if ((mask & WIN_BLOKCURSOR) == WIN_BLOKCURSOR)
		Scr_SetCursorShape(SCR_LINECURSOR);
}

/*****************************************************/

void Win_PutChar(unsigned char c)
{
	register unsigned short far *scr_tmp =
	       (unsigned short far *)(WIN_START);
	switch(c)
		{
		case KB_NEWLINE:	WIN_ROW++; break;
		case KB_CRGRTN:		WIN_COL=0; break;
		default:		scr_tmp += 80*WIN_ROW + WIN_COL;
					*scr_tmp =SCR_CHAR(c,Win_Current->ca);
					WIN_COL ++;
		}
	if (Win_Current->flags & WIN_WRAP)
		if (WIN_WIDTH <= WIN_COL)
			{
			WIN_COL = 0;
			WIN_ROW ++;
			}
	if (Win_Current->flags & WIN_SCROLL)
		if (WIN_ROW >= WIN_HEIGHT)
			{
			WIN_ROW = WIN_HEIGHT - 1;
			Win_ScrollUp();
			}
	if (Win_Current->flags & WIN_CLIP)
		{
		if (WIN_WIDTH <= WIN_COL)
			WIN_COL = WIN_WIDTH - 1;
		if (WIN_HEIGHT <= WIN_ROW)
			WIN_ROW = WIN_HEIGHT - 1;
		}
	else if (WIN_WIDTH <= WIN_COL)
	       	{
		Win_ScrollLeft();
		WIN_COL = WIN_WIDTH-1;
		}
	Win_PlaceCursor();
}

/*****************************************************/

/* The Win_FastPuts function is a slightly more efficient version
	of Win_PutsString, but does not allow all of the possible
	flag combinations, nor does it affect the window cursor position. */

void Win_FastPutS(int r, int c, STRING str)
{
	register unsigned short far *scr_tmp =
	       (unsigned short far *)(WIN_START);
	short space, slen, stmp;
	char ca = Win_Current->ca;
	slen = strlen(str);
	scr_tmp += 80*r + c;
	space = WIN_WIDTH - c;
	while (slen)
		{
		stmp = min(space, slen);
		slen -= stmp;
		while (stmp--) *scr_tmp++ =SCR_CHAR(*str++,ca);
		if (Win_Current->flags & WIN_WRAP != WIN_WRAP) break;
		scr_tmp = WIN_START + 80 * (++r);
		space = 80;
		if (r >= WIN_HEIGHT)
			if (Win_Current->flags & WIN_SCROLL) Win_ScrollUp();
			else break;
		}
}

void Win_PutString(STRING str)
{
	while (*str) Win_PutChar(*str++);
}

/******************************************************/

char Win_EchoGetChar(void)
{
	char c = getc_noecho();
	Win_PutChar(c);
	return c;
}



/*************************************************************
	Basic Editing Functions
**************************************************************/

void Win_Clear()
{
	_Block_Clear(WIN_X, WIN_Y,
		WIN_X+WIN_WIDTH-1,
		WIN_Y+WIN_HEIGHT-1,
		Win_Current->ia);
}

void Win_ClearLine(int l)
{
	_Block_Clear(WIN_X, WIN_Y + l,
		WIN_X + WIN_WIDTH-1,
		WIN_Y + l, Win_Current->ia);
}

void Win_DelLine(int l)
{
	_BlockMove((WIN_Y + l)*80 + WIN_X + 80,
		(WIN_Y + l)*80 + WIN_X,
		WIN_WIDTH, WIN_HEIGHT-1-l);
	_Block_Clear(WIN_X,WIN_Y+WIN_HEIGHT-1,
		WIN_X+WIN_WIDTH-1,
		WIN_Y+WIN_HEIGHT-1,
		Win_Current->ia);
}

void Win_InsLine(int l)
{
	_BlockMove((WIN_Y + l)*80 + WIN_X,
		(WIN_Y + l)*80 + WIN_X + 80,
		WIN_WIDTH, WIN_HEIGHT-l-1);
	_Block_Clear(WIN_X,WIN_Y + l,
		WIN_X+WIN_WIDTH-1,WIN_Y + l,
		Win_Current->ia);
}

void Win_InsColumn(int row, int col, int height)
{
	_BlockMove((WIN_Y + row)*80+WIN_X + col,
		(WIN_Y + row)*80+WIN_X + col + 1,
		WIN_WIDTH-col-1, height);
	 _Block_Clear(WIN_X+col, WIN_Y+row,
		WIN_X+col,WIN_Y+row+height-1,
		Win_Current->ia);
}

void Win_InsChar(void)
{
	Win_InsColumn(WIN_ROW,WIN_COL,1);
}


void Win_DelColumn(int row, int col, int height)
{
	_BlockMove((WIN_Y + row)*80+WIN_X + col + 1,
		(WIN_Y + row)*80 + WIN_X + col,
		WIN_WIDTH-col-1, height);
	 _Block_Clear(WIN_X+WIN_WIDTH-1, WIN_Y+col,
		WIN_X+WIN_WIDTH-1, WIN_Y+col+height-1,
		Win_Current->ia);
}

void Win_DelChar(void)
{
	Win_DelColumn(WIN_ROW, WIN_COL, 1);
}

/*****************************************/
/*	Window Movement			*/
/****************************************/

void Win_ScrollUp(void)
{
	Win_DelLine(0);
}

void Win_ScrollDown(void)
{
	Win_InsLine(0);
}

void Win_ScrollLeft(void)
{
	Win_DelColumn(0,0,WIN_HEIGHT);
}

void Win_ScrollRight(void)
{
	Win_InsColumn(0,0,WIN_HEIGHT);
}

/*****************************************
	Change attributes in window
******************************************/

void Win_PutAttribute(unsigned char attr, int fromx, int fromy, int w, int h)
{
	char far *scr_tmp;
	int wtmp;
	if (fromx + w > WIN_WIDTH) w= WIN_WIDTH - fromx;
	if (fromy + h > WIN_HEIGHT) h= WIN_HEIGHT - fromy;
	scr_tmp = (char far *)(WIN_START + 80*fromy + fromx) + 1;
	while (h--)
		{
		wtmp=w;
		while (wtmp--)
	       		{
			*scr_tmp = attr;
			scr_tmp += 2;
			}
		scr_tmp += 160-w*2;
		}
}


/***********************************************************************/

