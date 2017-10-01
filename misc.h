/**************************************/
/* misc.h - Miscellaneous Definitions */
/**************************************/

#include <stdio.h>
#include <ctype.h>
#include <dos.h>

/*********************/
/* Boolean Constants */
/*********************/

#define TRUE	1
#define FALSE	0

/**********************************/
/* Macro for declaration handling */
/**********************************/

#ifdef DECLARE
#define INIT(v,i)	v=i
#else
#define INIT(v,i)	extern v
#endif

/************************/
/* Far Pointer Handling */
/************************/

#define SEGMENT(p)  (unsigned short)(((unsigned long)((char far *)p)) >>16)
#define OFFSET(p)   (unsigned short)(((unsigned long)((char far *)p)) &0xFFFF)
#define FAR_PTR(s,o) (char far *) (( (unsigned long)(s) << 16) + o)

/***************/
/* Basic Types */
/***************/

#define _far	far			/* For pointers that need not necessarily be
								far, we use _far, allowing us if necessary
								to override their far definitions	*/
typedef char 	*STRING;

/*********************/
/* Basic 'functions' */
/*********************/

#define DOS_C_SRV(n)       	((char)0xFF & (char)bdos(n,0,0))
#define getc_noecho()      	DOS_C_SRV(0x7)
#define check_keyboard()   	DOS_C_SRV(0xB)
#define max(a,b)		((a>b)?a:b)
#define min(a,b)		((a>b)?b:a)
#define ctrl(x)			('x' & 037)
#define beep()			putchar(7)
#define _free(p)	if ((p)!=NULL) free(p)

