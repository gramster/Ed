/************************/
/* Name: screen.h	*/
/*			*/
/* Author: G. Wheeler	*/
/*			*/
/* Date: Dec 1988	*/
/*			*/
/* Version: 1.0		*/
/*  (but based on older */
/*   bits of code)	*/
/*			*/
/************************/

/*********************/
/* Screen Attributes */
/*********************/

/* Attribute Classes */

#define SCR_BLANK_ATTR	0x00  /* Black on black (CGA); blank (mono) */
#define SCR_ULINE_ATTR	0x01  /* Blue on black (CGA); underlined (mono) */
#define SCR_NORML_ATTR	0x07  /* White on black (CGA); normal (mono) */
#define SCR_INVRS_ATTR	0x70  /* Black on white (CGA); inverse (mono) */

/* Attribute Modifiers */

#define SCR_BLINK_CHAR	0x80
#define SCR_BOLD_CHAR	0x08

/* Macro to create screen words from char, attr pairs */

#define SCR_CHAR(c,a)	((((unsigned short)a)<<8) + ((unsigned short)c))

/* Cursor Shapes */

#define SCR_BLOKCURSOR	1
#define SCR_LINECURSOR	2

/* Text Positions */

#define SCR_LEFT	1
#define SCR_RIGHT	2
#define SCR_TOP		3
#define SCR_BOTTOM	4
#define SCR_CENTRE	5

/* Basic Constants	*/

#define SCR_COLUMNS	80
#define SCR_LINES	24
#define SCR_MONOBASE	0xB0000000L
#define SCR_CGABASE	0xB8000000L

/****************/
/* Window Flags	*/
/****************/

#define WIN_NOBORDER	0	/* No border			*/
#define WIN_BORDER	1	/* Bit mask for border	*/
#define WIN_BLOKCURSOR	2	/* Block cursor mode	*/
#define WIN_SCROLL	4	/* Scroll mode			*/
#define WIN_WRAP	8	/* Line wrap mode		*/
#define WIN_CLIP	16	/* Border 'clip' mode	*/
#define WIN_NONDESTRUCT	32	/* Save window preimage?*/

typedef struct Window
	{
	unsigned short x,y,w,h;	/* Top left position, width, height	*/
	void _far *preimage;	/* Pointer to save area for  		*/
				/* non-destructive windows		*/

	unsigned short far      /* Pointer to top-left corner in	*/
		*start;		/* video memory				*/

	unsigned short r,c;	/* Current row, column position 	*/
	unsigned short ba,ia; 	/* Border, inside colour attributes	*/
				/*	at clear time			*/
	unsigned char ca;	/* Current attribute			*/
	unsigned short flags;	/* Various booleans and border type	*/
	unsigned short tabsize;	/* Tab stop size			*/
	} WINDOW;

/* Pointer to current active window */

extern WINDOW _far *Win_Current;

/* Pointers to start of screen memory */

extern unsigned short scr_fbase;
extern unsigned short far *scr_base;	/* base address of video memory */

#define WIN_X		(Win_Current->x)
#define WIN_Y		(Win_Current->y)
#define WIN_WIDTH	(Win_Current->w)
#define WIN_HEIGHT	(Win_Current->h)
#define WIN_ROW		(Win_Current->r)
#define WIN_COL		(Win_Current->c)
#define WIN_START	(Win_Current->start)

/* Screen & Window macro 'functions' */

#define Win_SetTabs(t)		Win_Current->tabsize = t
#define Win_GetTabs()		(Win_Current->tabsize)
#define Win_SetAttribute(a)	Win_Current->ca = a
#define Win_PlaceCursor()	Scr_PutCursor(WIN_Y+WIN_ROW, WIN_X+WIN_COL)
#define Win_BlockCursor()	Win_SetFlags(WIN_BLOKCURSOR);
#define Win_LineCursor()	Win_ClearFlags(WIN_BLOKCURSOR);
#define Scr_SaveScreen(p)	p = Scr_BlockSave(scr_base,0l,25,80)
#define Scr_RestoreScreen(p)	Scr_BlockRestore(p,scr_base,25,80)

/* Screen function prototypes */

void _far *Scr_BlockSave(unsigned short far *from, unsigned short _far *to,
				unsigned short rows, unsigned short cols);
void Scr_BlockRestore(unsigned short _far *from, unsigned short far *to,
		unsigned short rows, unsigned short cols);
void Scr_InitSystem(void);
void Scr_EndSystem(void);
void Scr_ClearAll(void);
void Scr_SetCursorShape(int shape);
void Scr_PutCursor(int row, int col);
void Scr_GetCursor(int *row, int *col, int *shape);

/* Window Function Prototypes */

void Win_1LineBorderSet();
void Win_2LineBorderSet();
void Win_BorderSet(char tl, char tr, char bl, char br,
	char th, char bh, char rv, char lv);
WINDOW _far *Win_Make(char *title, int left, int top, int width, int height,
		int titleplace, int titlealign,
		unsigned char boxattr, unsigned char attr,
		unsigned short flags);
void Win_Free(WINDOW _far *win);
void Win_PutCursor(int row, int col);
void Win_Activate(WINDOW _far *win);
void Win_SetFlags(unsigned short mask);
void Win_ClearFlags(unsigned short mask);
void Win_PutChar(unsigned char c);
void Win_FastPutS(int row, int col, char *str);
void Win_PutString(char *str);
char Win_EchoGetChar(void);
void Win_Clear(void);
void Win_ClearLine(int l);
void Win_DelLine(int l);
void Win_InsLine(int l);
void Win_InsColumn(int row, int col, int height);
void Win_InsChar(void);
void Win_DelColumn(int row, int col, int height);
void Win_DelChar(void);
void Win_ScrollUp(void);
void Win_ScrollDown(void);
void Win_ScrollLeft(void);
void Win_ScrollRight(void);
void Win_PutAttribute(unsigned char attr, int fromx, int fromy, int w, int h);

