#include "misc.h"
#include "screen.h"

extern void Ed_Main();

main()
{
	Scr_InitSystem();
	Ed_Main();
	Scr_EndSystem();
}

