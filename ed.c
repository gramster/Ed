/* ed.c */

#include <errno.h>
#include <time.h>
#include "misc.h"
#include "errors.h"
#include "keybd.h"
#include "screen.h"

/******************
* Editor Commands *
******************/

#define _Ed_Quit	0
#define _Ed_Up		1
#define _Ed_Down	2
#define _Ed_Right	3
#define _Ed_Left	4
#define _Ed_Home	5
#define _Ed_End		6
#define _Ed_PageUp	7
#define _Ed_PageDown	8
#define _Ed_LeftWord	9
#define _Ed_RightWord	10
#define _Ed_LeftBrace	11
#define _Ed_RightBrace	12
#define _Ed_Search	13
#define _Ed_Translate	14
#define _Ed_MarkBlock	15
#define _Ed_MarkColumn	16
#define _Ed_File	17
#define _Ed_Options	18
#define _Ed_Debug	19
#define _Ed_Undo	20
#define _Ed_Compile	21
#define _Ed_Execute	22
#define _Ed_Cut		23
#define _Ed_Copy	24
#define _Ed_Paste	25
#define _Ed_Backspc	26
#define _Ed_Edit	27
#define _Ed_TopFile	28
#define _Ed_EndFile	29
#define _Ed_NumKeyDefs	30

/************************
*     Editor Windows	*
************************/

static WINDOW		*Ed_Win 	= (WINDOW *) NULL;
static WINDOW		*Pos_Win 	= (WINDOW *) NULL;
static WINDOW		*Msg_Win 	= (WINDOW *) NULL;
static WINDOW		*Menu_Win 	= (WINDOW *) NULL;
char   Ed_FileName[80] =
	{ 'N', 'O', 'N', 'A', 'M', 'E', '.', 'E', 'S', 'T', 0 };

/********************************
* Editor Linked List Structure	*
********************************/

typedef struct bufline
	{
	struct bufline	*next,	/* Forward pointer		*/
			*prev;	/* Backward pointer		*/
	STRING		text;	/* Actual text of line		*/
	int		ID;	/* Unique line identifier	*/
	unsigned char	len;	/* Length 0-255	excluding NULL	*/
	}
	BUFLINE;

#define BUFELTSZ	sizeof(BUFLINE)
static BUFLINE		*Ed_BufNow	=(BUFLINE *)NULL; /* current line */
static BUFLINE		*Ed_ScrapBuf	=(BUFLINE *)NULL; /* scrap buffer */

/****************************************
* Macros for accessing the linked list	*
****************************************/

#define ED_NEXT		(Ed_BufNow->next)
#define ED_PREV		(Ed_BufNow->prev)
#define ED_TEXT		(Ed_BufNow->text)
#define ED_ID		(Ed_BufNow->ID)
#define ED_LEN		(Ed_BufNow->len)

#define ED_NEXTNEXT	(Ed_BufNow->next->next)
#define ED_NEXTPREV	(Ed_BufNow->next->prev)
#define ED_NEXTTEXT	(Ed_BufNow->next->text)
#define ED_NEXTID	(Ed_BufNow->next->ID)

/************************
* Key Definitions	*
************************/

static unsigned char 	Ed_Key[_Ed_NumKeyDefs];		/* Key characters */
static void 		(*Ed_Func[_Ed_NumKeyDefs])();	/* and functions  */

/*********************************
* Current file position and size *
*********************************/

static long		Ed_LineNow	=0L;
static short		Ed_ColNow	=0;
static unsigned long	Ed_FileSize	=0L;
static long		Ed_LastLine	=0L;
static short		Ed_MustRefresh	=FALSE;

/************************
* Line Identifiers	*
************************/

static int 		Ed_IDCnt	=1;
#define 		Ed_NewID	(Ed_IDCnt++)

/***********************************
* Current Screen Image:		   *
*	Line number of first line  *
*	Lengths of each line	   *
*	IDs of each line	   *
***********************************/

static struct Ed_ScreenImage
{
	long firstln;
	long lastln;
	short lastlnrow;
	short len[21];
	long id[21];
} Ed_ScrImage =
{
	0l,0l,0,
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l,0l }
};

/****************************
* Special Screen Image IDs  *
****************************/

#define ED_NOLINE	0l
#define ED_CONTINUATION	(-1l)

/************************************************/
/*						*/
/*   Low-level Edit Buffer Handling Functions 	*/
/*						*/
/************************************************/

/********************************
* Free an entire linked list	*
*	from current position	*
********************************/

static void Ed_FreeAll()
{
	BUFLINE *btmp;
	while (Ed_BufNow)
		{
		btmp=Ed_BufNow;
		Ed_BufNow=Ed_BufNow->next;
		_free(btmp->text);
		_free(btmp);
		}
}

/********************************
* Position the buffer pointer	*
*	at a specific line,	*
*	normalising and 	*
*	returning the line	*
*	number			*
********************************/

static long Ed_Point(long line)
{
	line = max(0,min(line, Ed_LastLine));	/* Normalise		*/
	while (Ed_LineNow > line)	/* Move backward if necessary	*/
		{
		Ed_LineNow--;
		Ed_BufNow = ED_PREV;
		}
	while (Ed_LineNow < line && Ed_LineNow < Ed_LastLine)
		{			/* Move forward if necessary	*/
		Ed_LineNow++;
		Ed_BufNow = ED_NEXT;
		}
	return line;
}

/****************************************
* Locate a line on the screen with	*
*	the appropriate ID, if possible *
****************************************/

int Ed_FindLine(long ID)
{
	int rtn=Ed_Win->h;
	while (rtn-- > 0)
		if (Ed_ScrImage.id[rtn] ==ID) break;
	return (rtn);
}

/*************************
* Image / Screen Updates *
*************************/

static void Ed_ImgRefresh()
{
	int	scrline=0,
		wraps,
		end = Ed_Win->h;
	long	oldfline = Ed_LineNow,
		bline=Ed_ScrImage.firstln;

	/* Loop through whole screen image */

	while (scrline < end)
	       {
		/* Point to what must become the current screen line */

		Ed_Point(bline);
		if (bline<=Ed_LastLine)
			{
			Ed_ScrImage.lastln = bline;
			Ed_ScrImage.lastlnrow = scrline;
			}

		if (bline>Ed_LastLine) 	/* If past the end of the file */
			{
			/* ...then if line not blank, clear it */
			if (Ed_ScrImage.id[scrline]!=ED_NOLINE)
				{
				Ed_ScrImage.id[scrline]=ED_NOLINE;
				Ed_ScrImage.len[scrline]=0;
				Ed_MustRefresh=TRUE;
				}
			}
	      	/* ...else if the current image line is
			not the desired line, put the
			desired line in its place */
		else if (Ed_ScrImage.id[scrline] != ED_ID)
			{
			Ed_MustRefresh = TRUE;
			Ed_ScrImage.len[scrline] = (short)ED_LEN;
		      	Ed_ScrImage.id[scrline] = ED_ID;

			/* Clear the necessary number of lines */
			wraps = Ed_ScrImage.len[scrline] / Ed_Win->w;
	      	      	if (scrline + wraps >= Ed_Win->h)
		      	      	wraps=Ed_Win->h - scrline - 1;
		      	while (wraps>0)	Ed_ScrImage.id[scrline + wraps--] =
				ED_CONTINUATION;
			}
		/* Update the screen line and buffer line counters */
	       	scrline+=(1+Ed_ScrImage.len[scrline]/Ed_Win->w);
	       	bline++;
	       	}

	Ed_Point(oldfline);
}


static void Ed_WinRefresh()
{
	int	scrline=0,
		wraps,
		end = Ed_Win->h;
	long	oldfline = Ed_LineNow,
		bline=Ed_ScrImage.firstln;

	if (!Ed_MustRefresh) return;
	else Ed_MustRefresh = FALSE;

	/* Loop through whole screen image */

	while (scrline < end)
	       {
		/* Set buffer pointer to what must become the
			current screen line */

	       	Ed_Point(bline);
		if (Ed_ScrImage.id[scrline] == ED_NOLINE) Win_ClearLine(scrline);
		else	{
			wraps = Ed_ScrImage.len[scrline] / Ed_Win->w;
			/* Clear last screen row for line */
			if (scrline + wraps < Ed_Win->h)
				Win_ClearLine(scrline+wraps);
			/* Now write the actual line */
			Win_FastPutS(scrline,0,ED_TEXT);
			}
		/* Update the screen line and buffer line counters */
	       	scrline+=(1+wraps);
	       	bline++;
	       	}

	/* Restore the cursor coordinates */

	Ed_Point(oldfline);
	Win_PutCursor(Ed_FindLine(ED_ID),Ed_ColNow % Ed_Win->w);
}

/**************************************
*/

static void Ed_ShowPos(int showall)
{
	WINDOW *OldW = Win_Current;
	static long oldlnow, oldlast, oldsz;
	static short oldcol;
	char tmp[10];
	Win_Activate(Pos_Win);
	if (showall)
		{
		Win_Clear();
		Win_FastPutS(0,0,"  File:                Line: 0     of 0       Col: 0    Size: 0");
		Win_FastPutS(0,9,Ed_FileName);
		}
	if (showall || oldlnow!=Ed_LineNow)
		{
		ltoa(Ed_LineNow+1,tmp,10);
		Win_FastPutS(0,30,"     ");
		Win_FastPutS(0,30,tmp);
		oldlnow=Ed_LineNow;
		}
	if (showall || oldlast != Ed_LastLine)
		{
		ltoa(Ed_LastLine+1,tmp,10);
		Win_FastPutS(0,39,"     ");
		Win_FastPutS(0,39,tmp);
		oldlast = Ed_LastLine;
		}
	if (showall || oldcol!=Ed_ColNow)
		{
		itoa(Ed_ColNow+1,tmp,10);
		Win_FastPutS(0,52,"   ");
		Win_FastPutS(0,52,tmp);
		oldcol = Ed_ColNow;
		}
	if (showall || oldsz!=Ed_FileSize)
		{
		ltoa(Ed_FileSize,tmp,10);
		Win_FastPutS(0,63,"     ");
		Win_FastPutS(0,63,tmp);
		}
	Win_Activate(OldW);
}

/* May have to save /restore cursor shape */

static void Ed_GetResponse(STRING prompt,
	STRING response)
{
WINDOW *OldW = Win_Current;
int mode;
char *fptr,*eptr,c;
int col=strlen(prompt)+3;

fptr=eptr=response;
while (*eptr) eptr++;
Win_Activate(Msg_Win);
while (Win_Clear(),
	Win_FastPutS(0,2,prompt),
	Win_FastPutS(0,col,response),
	Win_PutCursor(0,col+(fptr-response)),
	c=getc_noecho(),
	c!=KB_CRGRTN)
		mode = Gen_EditString(c,response,&eptr,&fptr,mode);
Win_Clear();
Win_Activate(OldW);
}

/********************************************************
* Normalise the specified cursor position 		*
*	and update the actual cursor appropriately.	*
********************************************************/

static void Ed_FixCursor(long r, short c)
{
	short row;

	/* Normalise row and column coordinates */

	r=Ed_Point(r);
	Ed_ColNow = max(0, min(c, (short)ED_LEN));

	/* Make sure line appears on screen */

	if (Ed_LineNow<Ed_ScrImage.firstln)
		{
		Ed_ScrImage.firstln = Ed_LineNow;
		Ed_ImgRefresh();
		}
	else while (Ed_LineNow>Ed_ScrImage.lastln)
		{
		Ed_ScrImage.firstln++;
		Ed_ImgRefresh();
		}

	/* Ensure column appears on the screen */

	row = Ed_Win->h - Ed_ColNow / Ed_Win->w;
	while (Ed_FindLine(ED_ID) >= row)
		{
		Ed_ScrImage.firstln++;
		Ed_ImgRefresh();
		}

	/* Update the data structures and edit window */

	Ed_Win->r = Ed_FindLine(ED_ID) + Ed_ColNow / Ed_Win->w;
	Ed_Win->c = Ed_ColNow % Ed_Win->w;
	Ed_WinRefresh();
	Win_PutCursor(Ed_Win->r, Ed_Win->c);
}


static void Ed_ChgCol(short c)
{
	if (c<0 && Ed_LineNow>0L)
		{
		Ed_Point(Ed_LineNow-1);
		c = (short)ED_LEN;
		}
	else if (c>(short)ED_LEN && Ed_LineNow<Ed_LastLine)
		{
		Ed_Point(Ed_LineNow+1);
		c = 0;
		}
	Ed_FixCursor(Ed_LineNow,c);
}

#define Ed_ChgRow(r)	Ed_FixCursor(r,Ed_ColNow)
#define Ed_ReCursor()	Ed_FixCursor(Ed_LineNow, Ed_ColNow)

/****************
* Escape to DOS	*
*****************/

static int Ed_Suspend()
{
int result;
void _far *image;

if ((Scr_SaveScreen(image))==(void _far *)NULL)
	Error(ERR_NOSAVESCR);
Scr_ClearAll();
puts("Type 'exit' to return to the Estelle PDW");
result = system("command");
if (result==ENOENT) Error(ERR_NOSHELL);
else if (result==ENOMEM) Error(ERR_NOMEM);
else if (result)
	Error(ERR_BADSHELL,result);
sleep(1);
Scr_RestoreScreen(image);
return result;
}




/************************
* File Operations	*
*************************/

static void Ed_InsertCh(char c);
static void Ed_Down();
static void Ed_Right();
static void Ed_InsertCR();

static void Ed_ExpandTabs(STRING buff,short maxlen)
{
	STRING stmp;
	if ((stmp=malloc(maxlen))==NULL)
		Error(ERR_MEMFAIL,"Ed_ExpandTabs");
	else	{
		int posin=0, posout=0;
		while (buff[posin])
			{
			if (buff[posin]==KB_TAB)
				{
				int spcs=Ed_Win->tabsize -
						posin % Ed_Win->tabsize;
				while (spcs--) stmp[posout++]=' ';
				}
			else stmp[posout++] = buff[posin];
			posin++;
			}
		stmp[posout]=0;
		strcpy(buff,stmp);
		free(stmp);
		}
}


static void debugit()
{
	char x,y,*t;
	t = &x;
	*t = 0;
}


static void Ed_LoadFile(STRING name)
{
	FILE *fp;
	char buff[512];
	STRING stmp;
	Win_Activate(Ed_Win);
	if ((fp=fopen(name,"rt"))!=NULL)
		{
		Ed_Point(0);  		Ed_FreeAll();
		Ed_BufNow=Ed_ScrapBuf;	Ed_FreeAll();
		if ((Ed_BufNow = (BUFLINE *)malloc(BUFELTSZ))==NULL)
				Error(ERR_MEMFAIL,"Ed_InsertCh");
		ED_PREV = ED_NEXT = NULL;
		Ed_FileSize=Ed_LineNow=0l;
		Ed_ColNow=0;
		while (!feof(fp))
			{
			short len;
			fgets(buff,512,fp);
			Ed_ExpandTabs(buff,512);
			if ((stmp=malloc(len=strlen(buff)))==NULL)
				Error(ERR_MEMFAIL,"Ed_LoadFile");
			strncpy(stmp,buff,--len);
			stmp[len]=0;
			ED_TEXT = stmp;
			ED_LEN = (unsigned char)len;
			ED_ID = Ed_NewID;
			Ed_FileSize += len+2;
			if (!feof(fp))
				{
				if ((ED_NEXT = (BUFLINE *)malloc(BUFELTSZ))==NULL)
					Error(ERR_MEMFAIL,"Ed_InsertCh");
				ED_NEXTNEXT = NULL;
				ED_NEXTPREV = Ed_BufNow;
				Ed_BufNow = ED_NEXT;
				Ed_LineNow++;
				Ed_LastLine++;
				}
			if (Ed_LineNow==159) debugit();
			}
		fclose(fp);
		Ed_ScrImage.firstln=0;
		Ed_ImgRefresh();
		Ed_FixCursor(0l,0);
		}
	else Error(ERR_NOOPENFL,name);
	Win_Activate(Menu_Win);
}

static void Ed_ReadFile()
{

}

static void Ed_WriteFile()
{

}

/****************************************/
/*					*/
/*     Key-assigned Editor Functions	*/
/*					*/
/****************************************/

static void Ed_Quit()
{
	Ed_Point(0);  		Ed_FreeAll();
	Ed_BufNow=Ed_ScrapBuf;	Ed_FreeAll();
	Win_Free(Ed_Win);
	Win_Free(Pos_Win);
	Win_Free(Msg_Win);
	Win_Free(Menu_Win);
	trace(NULL,0);
}

/************************
* Basic Cursor Movement *
************************/

static void Ed_Up()		{ Ed_ChgRow(Ed_LineNow-1);		}
static void Ed_Down()   	{ Ed_ChgRow(Ed_LineNow+1);      	}
static void Ed_Right()  	{ Ed_ChgCol(Ed_ColNow+1);       	}
static void Ed_Left()   	{ Ed_ChgCol(Ed_ColNow-1);       	}
static void Ed_Home()   	{ Ed_ChgCol(0);                 	}
static void Ed_End()    	{ Ed_ChgCol((short)ED_LEN);   		}
static void Ed_TopFile()	{ Ed_FixCursor(0l,0);			}
static void Ed_EndFile()	{ Ed_ChgRow(Ed_LastLine);
					Ed_ChgCol((short)ED_LEN); 	}
static void Ed_PageUp()
{
	long start = Ed_LineNow;
	short col = Ed_ColNow;
	if (start != Ed_ScrImage.firstln)
		Ed_ChgRow(Ed_ScrImage.firstln);
	else while (Ed_ScrImage.lastln != start && Ed_LineNow>0l)
		Ed_Up();
	Ed_FixCursor(Ed_LineNow,col);
}

static void Ed_PageDown()
{
	long start = Ed_LineNow;
	short col = Ed_ColNow;
	if (start != Ed_ScrImage.lastln)
		Ed_ChgRow(Ed_ScrImage.lastln);
	else while (Ed_ScrImage.firstln != start && Ed_LineNow<Ed_LastLine)
		Ed_Down();
	Ed_FixCursor(Ed_LineNow,col);
}

/***********************
* Word Cursor Movement *
************************/

static void Ed_LeftWord()
{
	Ed_Left();
	do	Ed_Left();
	while ((ED_TEXT[Ed_ColNow] != ' ') && (Ed_LineNow+Ed_ColNow >0));
	if (Ed_LineNow + Ed_ColNow) Ed_Right();
}

static void Ed_RightWord()
{
	Ed_Right();
	do	Ed_Right();
	while ((ED_TEXT[Ed_ColNow] != ' ') &&
		(Ed_LineNow<Ed_LastLine || Ed_ColNow<(short)ED_LEN));
	while ((ED_TEXT[Ed_ColNow] == ' ') &&
		(Ed_LineNow<Ed_LastLine || Ed_ColNow<(short)ED_LEN))
			Ed_Right();

}

/****************************************
* Bracket, brace and parenthesis moves	*
*****************************************/

static void Ed_LeftBrace()
{
	long	OldLine = Ed_LineNow;
	short	OldCol = Ed_ColNow,
		found = 0;

	Ed_Left(); /* To prevent matching at current cursor position */

	while (Ed_LineNow!=0 || Ed_ColNow!=0)
		{
		switch (ED_TEXT[Ed_ColNow])
			{
			case '{': case '}':
			case '(': case ')':
			case '[': case ']':
					found = 1;
					break;

			default : 	if ( --Ed_ColNow < 0)
						{
						Ed_Point(Ed_LineNow-1);
						Ed_ColNow = (short)ED_LEN;
						}
					continue;
			}
		break;
		}

	if (!found)	/* Check line 0, col 0 */
		switch(ED_TEXT[Ed_ColNow])
			{
			case '[': case ']':
			case '(': case ')':
			case '{': case '}': found=1;
			default : break;
			}
	if (!found)
		Ed_FixCursor(OldLine,OldCol);
	else
		Ed_ReCursor();
}

static void Ed_RightBrace()
{
	long	OldLine = Ed_LineNow;
	short	OldCol = Ed_ColNow,
		found = 0,
		len;

	Ed_Right();

	len = (short)ED_LEN;
	while (Ed_LineNow!=Ed_LastLine || Ed_ColNow!=len)
		{
		switch (ED_TEXT[Ed_ColNow])
			{
			case '}': case '{':
			case ')': case '(':
			case ']': case '[':
					found = 1;
					break;

			default : 	Ed_ColNow++;
					if (Ed_ColNow>len)
						{
						Ed_ColNow = 0;
						Ed_Point(Ed_LineNow+1);
						len = (short)ED_LEN;
						}
					continue;
			}
		break;
		}

	if (!found)
		Ed_FixCursor(OldLine,OldCol);
	else
		Ed_ReCursor();
}

/************************
* Search and Translate	*
************************/

static void Ed_Search()
{

}

static void Ed_Translate()
{

}

/********
* Undo	*
********/

static void Ed_Undo()
{

}

/************************
* Scrap Operations	*
************************/

static void Ed_MarkBlock()
{

}

static void Ed_MarkColumn()
{

}

static void Ed_Cut()
{

}

static void Ed_Copy()
{

}

static void Ed_Paste()
{

}

/***********************************************/

#define MM_FILE		0
#define MM_EDIT		1
#define MM_COMPILE	2
#define MM_EXECUTE	3
#define MM_OPTIONS	4
#define MM_DEBUG	5

#define MAINMENU	"   File     Edit     Compile      eXecute      Options      Debug"

static short Ed_MMenu=MM_FILE;

static void MM_Bold(short s, short w)
{
	Win_PutAttribute(SCR_INVRS_ATTR,s,0,w,1);
}

static void MM_UnBold(short s, short w)
{
	Win_PutAttribute(SCR_NORML_ATTR,s,0,w,1);
}

static void Ed_MainMenu()
{
	int active=1, oldactive;
	char code;
	Win_Activate(Menu_Win);
	while (TRUE)
		{
		MM_Bold(0,80);
		switch(Ed_MMenu)
			{
			case MM_FILE:	MM_UnBold(3,4);
					if (active)
						{
						Ed_GetResponse("File?",Ed_FileName);
						Ed_LoadFile(Ed_FileName);
						MM_Bold(3,4);
						MM_UnBold(12,4);
						Win_Activate(Ed_Win);
						Ed_ShowPos(1);
						return;
						}
					break;
			case MM_EDIT:	MM_UnBold(12,4);
					if (active)
						{
						Win_Activate(Ed_Win);
						return;
						}
					break;
			case MM_COMPILE:MM_UnBold(21,7);
					break;
			case MM_EXECUTE:MM_UnBold(34,7);
					break;
			case MM_OPTIONS:MM_UnBold(47,7);
					break;
			case MM_DEBUG:	MM_UnBold(60,5);
					break;
			default: Error(ERR_BAD_ARGUMENT,"Ed_MainMenu");
			}
		if (code = getc_noecho())
			switch(code)
				{
				case KB_CRGRTN:	active = 1;
						break;
				case KB_ESC:	active = 0;
						break;
				default:	break;
				}
		else
			{
			oldactive = active;
			active = 1;
			switch(KB_TOSPECIAL(getc_noecho()))
			       	{
			       	case KB_LEFT:	Ed_MMenu--;
						if (Ed_MMenu<0)
							Ed_MMenu=MM_DEBUG;
						active = 0;
						break;

			       	case KB_RIGHT:	Ed_MMenu++;
						if (Ed_MMenu>MM_DEBUG)
							Ed_MMenu=MM_FILE;
						active = 0;
						break;
				case KB_ALT_F:	Ed_MMenu = MM_FILE;
						break;
				case KB_ALT_E:	Ed_MMenu = MM_EDIT;
						break;
				case KB_ALT_C:	Ed_MMenu = MM_COMPILE;
						break;
				case KB_ALT_X:	Ed_MMenu = MM_EXECUTE;
						break;
				case KB_ALT_O:	Ed_MMenu = MM_OPTIONS;
						break;
				case KB_ALT_D:	Ed_MMenu = MM_DEBUG;
						break;
				default: 	active = oldactive;
						break;
				}
			}
		}
}

static void Ed_InvokeMenu(short menu)
{
	Ed_MMenu = menu;
	Ed_MainMenu();
}

/************************
* Menu Operations	*
************************/

static void Ed_Edit()		{ Ed_InvokeMenu(MM_EDIT);	}
static void Ed_Compile()        { Ed_InvokeMenu(MM_COMPILE);    }
static void Ed_Execute()        { Ed_InvokeMenu(MM_EXECUTE);    }
static void Ed_File()           { Ed_InvokeMenu(MM_FILE);       }
static void Ed_Options()        { Ed_InvokeMenu(MM_OPTIONS);    }
static void Ed_Debug()          { Ed_InvokeMenu(MM_DEBUG);      }

/**********************************************************************/

static void Ed_InsertCh(char ch)
{
	STRING stmp;
	if (Ed_BufNow==NULL)
		{
		if ((Ed_BufNow = (BUFLINE *)malloc(sizeof(BUFLINE)))==NULL)
			Error(ERR_MEMFAIL,"Ed_InsertCh");
		ED_PREV = ED_NEXT = ED_TEXT = NULL;
		ED_LEN = (unsigned char)0;
		}

	if ((stmp=(STRING)malloc(2+(int)ED_LEN))==0)
		Error(ERR_MEMFAIL,"Ed_InsertCh");
	strncpy(stmp,ED_TEXT,Ed_ColNow);
	stmp[Ed_ColNow]=ch;
	strcpy(stmp+Ed_ColNow+1,ED_TEXT+Ed_ColNow);

	_free(ED_TEXT);
	ED_TEXT = stmp;
	ED_LEN++;
	ED_ID	= Ed_NewID;
	Ed_FileSize++;

	Ed_Right();
}


static void Ed_BackSpace()
{
	STRING stmp;
	if (Ed_BufNow != NULL)
		{
		if (Ed_ColNow>0)
			{
			if ((stmp=(STRING)malloc((int)ED_LEN))==0)
				Error(ERR_MEMFAIL,"Ed_DeleteCh");
			strncpy(stmp,ED_TEXT,Ed_ColNow-1);
			strcpy(stmp+Ed_ColNow-1,ED_TEXT+Ed_ColNow);
			_free(ED_TEXT);
			ED_TEXT = stmp;
			ED_LEN--;
			ED_ID	= Ed_NewID;
			Ed_FileSize--;
			Ed_Left();
			}
		else if (Ed_LineNow>0)
			{
			BUFLINE *btmp;
			int len = (short)ED_LEN;
			Ed_Up();
			Ed_End();
			len += (short)ED_LEN;
			if ((stmp=(STRING)malloc(len+1))==0)
				Error(ERR_MEMFAIL,"Ed_DeleteCh");
			strcpy(stmp,ED_TEXT);
			strcat(stmp,ED_NEXTTEXT);
			_free(ED_NEXTTEXT);
			_free(ED_TEXT);
			ED_TEXT = stmp;
			ED_LEN = (unsigned char)len;
			ED_ID	= Ed_NewID;
			btmp = ED_NEXT;
			if (ED_NEXT = ED_NEXTNEXT) ED_NEXTPREV = Ed_BufNow;
			_free(btmp);
			Ed_FileSize-=2;
			Ed_LastLine--;
			}
	Ed_ImgRefresh();
	Ed_WinRefresh();
	}
}

static void Ed_InsertCR()
{
	BUFLINE *oldnext=ED_NEXT;
	int tlen;
	STRING 	ctmp=ED_TEXT;

	if (Ed_BufNow==NULL)
		{
		if ((Ed_BufNow = (BUFLINE *)malloc(sizeof(BUFLINE)))==NULL)
			Error(ERR_MEMFAIL,"Ed_InsertCR");
		ED_PREV = ED_NEXT = ctmp = NULL;
		}

	/* Insert new buffer record */

	if ((ED_NEXT = (BUFLINE *)malloc(sizeof(BUFLINE)))==NULL)
			Error(ERR_MEMFAIL,"Ed_InsertCR");
	ED_NEXTPREV 	= Ed_BufNow;
	ED_NEXTNEXT 	= oldnext;
	if (oldnext) oldnext->prev = ED_NEXT;
	ED_ID		= Ed_NewID;
	ED_NEXTID 	= Ed_NewID;

	tlen=max(0,strlen(ctmp)-Ed_ColNow); /* text length of new line */
	if ((ED_TEXT = (STRING)malloc(Ed_ColNow+1))==NULL)
	       Error(ERR_MEMFAIL,"Ed_InsertCR");
	if ((ED_NEXTTEXT = (STRING)malloc(tlen+1))==NULL)
	       Error(ERR_MEMFAIL,"Ed_InsertCR");

       	strncpy(ED_TEXT,ctmp,Ed_ColNow);
	ED_TEXT[Ed_ColNow]=0;
	ED_LEN = (unsigned char)Ed_ColNow;
	strncpy(ED_NEXTTEXT,ctmp+Ed_ColNow,tlen);
	ED_NEXTTEXT[tlen]=0;
	ED_NEXT->len = (unsigned char)tlen;

	Ed_FileSize+=2;
	Ed_LastLine++;

	Ed_FixCursor(Ed_LineNow+1,0);
}


/***********************************************/
/* Execute the command specified by the	*/
/* control characters c1 and c2.		*/
/***********************************************/

unsigned char Ed_Command(c1,c2)
{
	int i;
	unsigned char rtn=c1;
	if (c1==0 || c1==0xE0) rtn=KB_TOSPECIAL(c2);
	for (i=0;i<_Ed_NumKeyDefs;i++)
	       if (rtn == Ed_Key[i])
		      {
		      (*Ed_Func[i])();
		      break;
		      }
	if (i>=_Ed_NumKeyDefs)
		if (rtn==KB_CRGRTN)
			Ed_InsertCR();
		else if (rtn==KB_TAB)
			{
			short spcs = Ed_Win->tabsize
					- Ed_ColNow % Ed_Win->tabsize;
			while (spcs--) Ed_InsertCh(' ');
			}
		else Ed_InsertCh(rtn);
	return rtn;
}


/**************************/
/* Set up the Edit Window */
/**************************/

static void Ed_ScrInitialise()
{
	void Ed_AssignKeys();
	Msg_Win = Win_Make(0,0,24,80,1,0,0,SCR_NORML_ATTR,SCR_NORML_ATTR,0);
	Win_Activate(Msg_Win);
	Win_Clear();
	Menu_Win = Win_Make(0,0,0,80,1,0,0,SCR_NORML_ATTR,SCR_INVRS_ATTR,0);
	Win_Activate(Menu_Win);
	Win_Clear();
	Win_FastPutS(0,0,MAINMENU);
	Win_BorderSet('³','³','À','Ù',' ','Ä','³','³');
	Ed_Win = Win_Make(0,1,3,78,20,0,0,SCR_NORML_ATTR,SCR_NORML_ATTR,
			  	WIN_WRAP|WIN_CLIP|WIN_BORDER);
	Win_Activate(Ed_Win);
	Win_ClearFlags(WIN_BLOKCURSOR);
	Win_Clear();
	Win_BorderSet('Ú','¿','³','³','Ä',' ','³','³');
	Pos_Win = Win_Make(0,1,2,78,1,0,0,SCR_NORML_ATTR,SCR_NORML_ATTR,
				WIN_BORDER);
	Ed_ShowPos(1);
	Ed_AssignKeys();
}

/************************************************************************

	The main program simply sets up the screen, then repeatedly
	reads key presses and acts on them by calling the 'command'
	keystroke interpreter. If the character read is a null, then
	a special key has been pressed, and a second character must
	be read as well.

*************************************************************************/

void Ed_Main()
{
	char c;
	Ed_ScrInitialise();
	while (TRUE)
		{
		Ed_ShowPos(0);
		c=getc_noecho();
		if (Ed_Command(c, (c==0 ? getc_noecho() : 0))==Ed_Key[_Ed_Quit])
			break;
		}
}




/**********************************/
/* Set Up Default Key Assignments */
/**********************************/

void Ed_AssignKeys()
{
 	Ed_Key[_Ed_Quit	      ] = KB_ALT_Z;
 	Ed_Key[_Ed_Up	      ] = KB_UP;
 	Ed_Key[_Ed_Down	      ] = KB_DOWN;
 	Ed_Key[_Ed_Right      ] = KB_RIGHT;
 	Ed_Key[_Ed_Left	      ] = KB_LEFT;
 	Ed_Key[_Ed_Home	      ] = KB_HOME;
 	Ed_Key[_Ed_End	      ] = KB_END;
 	Ed_Key[_Ed_PageUp     ] = KB_PGUP;
 	Ed_Key[_Ed_PageDown   ] = KB_PGDN;
 	Ed_Key[_Ed_LeftWord   ] = KB_CTRL_LEFT;
 	Ed_Key[_Ed_RightWord  ] = KB_CTRL_RIGHT;
 	Ed_Key[_Ed_LeftBrace  ] = KB_CTRL_PGUP;
 	Ed_Key[_Ed_RightBrace ] = KB_CTRL_PGDN;
 	Ed_Key[_Ed_Search     ] = KB_ALT_S;
 	Ed_Key[_Ed_Translate  ] = KB_ALT_T;
 	Ed_Key[_Ed_MarkBlock  ] = KB_ALT_M;
 	Ed_Key[_Ed_MarkColumn ] = KB_ALT_N;
 	Ed_Key[_Ed_File       ] = KB_ALT_F;
 	Ed_Key[_Ed_Undo	      ] = KB_ALT_U;
 	Ed_Key[_Ed_Compile    ] = KB_ALT_C;
 	Ed_Key[_Ed_Execute    ] = KB_ALT_X;
	Ed_Key[_Ed_Options    ] = KB_ALT_O;
	Ed_Key[_Ed_Debug      ] = KB_ALT_D;
 	Ed_Key[_Ed_Cut	      ] = KB_DEL;
	Ed_Key[_Ed_Backspc    ] = KB_BACKSPC;
	Ed_Key[_Ed_Copy	      ] = KB_ALT_MINUS;
 	Ed_Key[_Ed_Paste      ] = KB_ALT_P;
	Ed_Key[_Ed_Edit       ] = KB_ALT_E;
	Ed_Key[_Ed_TopFile    ] = KB_CTRL_HOME;
	Ed_Key[_Ed_EndFile    ] = KB_CTRL_END;
 	Ed_Func[_Ed_Quit      ] = Ed_Quit;
 	Ed_Func[_Ed_Up	      ] = Ed_Up;
 	Ed_Func[_Ed_Down      ] = Ed_Down;
 	Ed_Func[_Ed_Right     ] = Ed_Right;
 	Ed_Func[_Ed_Left      ] = Ed_Left;
 	Ed_Func[_Ed_Home      ] = Ed_Home;
 	Ed_Func[_Ed_End	      ] = Ed_End;
 	Ed_Func[_Ed_PageUp    ] = Ed_PageUp;
 	Ed_Func[_Ed_PageDown  ] = Ed_PageDown;
 	Ed_Func[_Ed_LeftWord  ] = Ed_LeftWord;
 	Ed_Func[_Ed_RightWord ] = Ed_RightWord;
 	Ed_Func[_Ed_LeftBrace ] = Ed_LeftBrace;
 	Ed_Func[_Ed_RightBrace] = Ed_RightBrace;
 	Ed_Func[_Ed_Search    ] = Ed_Search;
 	Ed_Func[_Ed_Translate ] = Ed_Translate;
 	Ed_Func[_Ed_MarkBlock ] = Ed_MarkBlock;
 	Ed_Func[_Ed_MarkColumn] = Ed_MarkColumn;
 	Ed_Func[_Ed_File      ] = Ed_File;
 	Ed_Func[_Ed_Undo      ] = Ed_Undo;
 	Ed_Func[_Ed_Compile   ] = Ed_Compile;
 	Ed_Func[_Ed_Execute   ] = Ed_Execute;
 	Ed_Func[_Ed_Options   ] = Ed_Options;
 	Ed_Func[_Ed_Debug     ] = Ed_Debug;
 	Ed_Func[_Ed_Cut	      ] = Ed_Cut;
	Ed_Func[_Ed_Backspc   ] = Ed_BackSpace;
 	Ed_Func[_Ed_Copy      ] = Ed_Copy;
 	Ed_Func[_Ed_Paste     ] = Ed_Paste;
	Ed_Func[_Ed_Edit      ] = Ed_Edit;
	Ed_Func[_Ed_TopFile   ] = Ed_TopFile;
	Ed_Func[_Ed_EndFile   ] = Ed_EndFile;
}


