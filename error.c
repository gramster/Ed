/* error.c */

#include <stdio.h>
#include "misc.h"
#include "screen.h"
#include "errors.h"

#define INTERNAL	0
#define EDITOR		2
#define FATAL		3

static char *InternalErrTable[]=
{	
"Malloc failed in function %s",
"Illegal argument passed to function %s"
};

static char *EditorErrTable[]=
{
"Cannot save screen image",
"Cannot find COMMAND.COM",
"Insufficient memory",
"Error while trying to execute COMMAND.COM: %d",
"Cannot open file: %s"
};

static char *FatalErrTable[] =
{
""
};

void Error(errno /*,...*/ )
int errno;
{
char MsgBuf[80],Message[80];
va_list args;
WINDOW *Err_Win, *Old_Win=Win_Current;
char *(*tbl)[];
int type=INTERNAL;

tbl = &InternalErrTable;
if (errno>=100)
{
	errno -= 100;
	tbl = &EditorErrTable;
	type = EDITOR;
}
if (errno>=100)
{
	errno-=100;
	tbl = &FatalErrTable;
	type = FATAL;
}

va_start(args,errno);
vsprintf(MsgBuf,(*tbl)[errno],args);

if (type==FATAL) strcpy(Message,"Fatal error : ");
else if (type==INTERNAL) strcpy(Message,"Internal error : ");
else Message[0]=0;
strcat(Message,MsgBuf);
Win_1LineBorderSet();
Err_Win = Win_Make(0,1,10,78,1,0,0,SCR_NORML_ATTR,SCR_NORML_ATTR,WIN_NONDESTRUCT|WIN_BORDER);
if (Err_Win)
	{
	Win_Activate(Err_Win);
	Win_Clear();
	Win_FastPutS(0,1,Message);
	sleep(2);
	Win_Free(Err_Win);
	}
else	fprintf(stderr,"\07%s\n",Message);

Win_Activate(Old_Win);
if (type==FATAL) exit(-1);
}



/* trace - Debugging Function */


void trace(char *msg,int arg)
{
static FILE *tracefl=NULL;
#ifdef TRACE
if (tracefl== NULL)	tracefl=fopen("Trace.lst","w");
if (msg    == NULL)	fclose(tracefl);
else	{		fprintf(tracefl,msg,arg); fflush(tracefl); }
#endif
}



