/***********************************************/
/* Menu structures and function template	*/
/***********************************************/

/* A Mnu_Opt corresponds to a single menu parameter */

typedef struct Mnu_Opt
{
	char	*Item;		/* The parameter name (eg "Baud Rate")	*/
	short	maxchoices;	/* The number of possible legal values	*/
	short	choice;		/* The array index of the current value	*/
	unsigned short *value;	/* The pointer to the current value	*/
	char	*p_vals[16];	/* The (printable) set of values	*/
	unsigned short a_vals[16]; /* The (actual) set of values	*/
} MENUOPT;

typedef struct Menu
{
	int	y,		/* Top row of menu window		*/
		x,		/* Left column of menu window		*/
		h,		/* Height of menu window		*/
		w,		/* Width column of menu window		*/
		optx;		/* Column at which to print value field	*/
	char	*title;		/* Menu title				*/
	MENUOPT (*Opts)[];	/* Pointer to the array of options	*/
} MENU;



void Mnu_Process(MENU *menu  , ... /* unsigned short *optval1, ... */);

