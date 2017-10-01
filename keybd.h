/* keycodes.h */

#define KB_TOSPECIAL(k)	(k+128)	 	/* Special key		*/
#define KB_IS_SPEC(k)	(k>127)
#define KB_TONORMAL(k)	(k-128)

#define KB_ESC		0x1B
#define KB_BACKSPC	0X8
#define KB_TAB		0x9
#define KB_NEWLINE	0xA
#define KB_CRGRTN	0xD
#define KB_BEEP		0x7
#define KB_UP		KB_TOSPECIAL(0x48)
#define KB_DOWN		KB_TOSPECIAL(0x50) 	/* Cursor down	*/
#define KB_RIGHT	KB_TOSPECIAL(0x4D) 	/* Cursor right	*/
#define KB_LEFT		KB_TOSPECIAL(0x4B) 	/* Cursor left	*/
#define KB_HOME		KB_TOSPECIAL(0x47) 	/* Cursor home	*/
#define KB_END		KB_TOSPECIAL(0x4F) 	/* Cursor end	*/
#define KB_PGUP		KB_TOSPECIAL(0x49) 	/* Scroll up	*/
#define KB_PGDN		KB_TOSPECIAL(0x51) 	/* Scroll down	*/
#define KB_DEL		KB_TOSPECIAL(0x53) 	/* Delete	*/
#define KB_INS		KB_TOSPECIAL(0x52) 	/* Insert scrap	*/
#define KB_CTRL_HOME	KB_TOSPECIAL(0x77) 	/* Top of file	*/
#define KB_CTRL_END	KB_TOSPECIAL(0X75) 	/* End of file	*/
#define KB_CTRL_LEFT	KB_TOSPECIAL(0X73) 	/* Left word	*/
#define KB_CTRL_RIGHT	KB_TOSPECIAL(0X74) 	/* Right word	*/
#define KB_CTRL_PGUP	KB_TOSPECIAL(0X84) 	/* Left brace	*/
#define KB_CTRL_PGDN	KB_TOSPECIAL(0X76) 	/* Right brace	*/
#define KB_ALT_S	KB_TOSPECIAL(0X1F) 	/* Search	*/
#define KB_ALT_T	KB_TOSPECIAL(0X14) 	/* Translate	*/
#define KB_ALT_M	KB_TOSPECIAL(0X32) 	/* Mark		*/
#define KB_ALT_N	KB_TOSPECIAL(0X31)	/* Next file	*/
#define KB_ALT_E	KB_TOSPECIAL(0X12)	/* Edit new file*/
#define KB_ALT_R	KB_TOSPECIAL(0X13)	/* Read file	*/
#define KB_ALT_W	KB_TOSPECIAL(0X11)	/* Write file	*/
#define KB_ALT_Q	KB_TOSPECIAL(0X10)	/* */
#define KB_ALT_P	KB_TOSPECIAL(0X19)	/* */
#define KB_ALT_O	KB_TOSPECIAL(0X18)
#define KB_ALT_C	KB_TOSPECIAL(0X2e)	/* Compile	*/
#define KB_ALT_X	KB_TOSPECIAL(0X2d)	/* Execute	*/
#define KB_ALT_Y	KB_TOSPECIAL(0X15)	/* Yank scrap	*/
#define KB_ALT_D	KB_TOSPECIAL(0X20)	/* Delete scrap	*/
#define KB_ALT_U	KB_TOSPECIAL(0X16)	/* Undo		*/
#define KB_ALT_F	KB_TOSPECIAL(0X21)	/* File info	*/
#define KB_ALT_Z 	KB_TOSPECIAL(0X2C)	/* Quit		*/
#define KB_ALT_G	KB_TOSPECIAL(0X22)
#define KB_ALT_EQUAL	KB_TOSPECIAL(0X83)
#define KB_ALT_MINUS 	KB_TOSPECIAL(0x82)

#define KB_F1	KB_TOSPECIAL(0x3B) 	/* 	*/
#define KB_F2	KB_TOSPECIAL(0x3C) 	/* 	*/
#define KB_F3	KB_TOSPECIAL(0x3D) 	/* 	*/
#define KB_F4	KB_TOSPECIAL(0x3E) 	/* 	*/
#define KB_F5	KB_TOSPECIAL(0x3F) 	/* 	*/
#define KB_F6	KB_TOSPECIAL(0x40) 	/* 	*/
#define KB_F7	KB_TOSPECIAL(0x41) 	/* 	*/
#define KB_F8	KB_TOSPECIAL(0x42) 	/* 	*/
#define KB_F9	KB_TOSPECIAL(0x43) 	/* 	*/
#define KB_F10	KB_TOSPECIAL(0x44) 	/* 	*/

unsigned char Kb_GetCursKey(void);
