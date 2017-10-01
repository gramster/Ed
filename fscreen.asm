;************************************************
;*						*
;* Name: fscreen.asm				*
;*						*
;* Author: G. Wheeler				*
;*						*
;* Date: July 1988				*
;* 						*
;* Version: 1.0					*
;*						*
;* Description: This file contains two routines	*
;*	written in 8086 assembly language for	*
;*	speed purposes. The routines are used	*
;*	for filling screen memory with specific	*
;*	character/attribute pairs, and for	*
;*	moving screen memory.			*
;*						*
;************************************************

	name fscreen

	extrn	_scr_fbase:word
_TEXT	segment byte public 'CODE'
DGROUP	group	_DATA,_BSS
	assume cs:_TEXT,ds:DGROUP,ss:DGROUP
_TEXT	ends

_DATA	segment word public 'DATA'
_DATA	ends
_BSS	segment word public 'BSS'
_BSS	ends

; Prototypes for the functions in this file:
;
;	ffillscrmem(unsigned short start,nchars,chrattr)
;	fmovescrmem(unsigned short to,from,nchars)
;

_TEXT	segment byte public 'CODE'

	public _ffillscrmem
	public _fmovescrmem

_ffillscrmem	proc near
	push	bp
	mov	bp,	sp
	push	si
	push	di
	push	ds
;
; Get arguments
;
	mov	di,word ptr [bp+4]
	mov	cx,word ptr [bp+6]
	jcxz	short _TEXT:done
	mov	ax,	word ptr DGROUP:_scr_fbase
	mov	es,	ax
	mov	ax,	word ptr [bp+8]
	cld

	rep	stosw

done:
	pop	ds
	pop	di
	pop	si
	mov	sp,	bp
	pop	bp
	ret
_ffillscrmem	endp


_fmovescrmem	proc near
	push	bp
	mov	bp,	sp
	push	si
	push	di
	push	ds

	mov	si,	word ptr [bp+4]		; from offset
	mov	di,	word ptr [bp+6]		; to offset
	mov	cx,	word ptr [bp+8]		; nchars

	cld
	cmp	di,	si
	jl	domove

       	std
	mov	ax,	cx
	dec	ax
	shl	ax,	1
	add	di,	ax
	add	si,	ax

domove:
	mov	ax,	word ptr DGROUP:_scr_fbase
	mov	ds,	ax
	mov	es,	ax
	rep	movsw
	pop	ds
	pop	di
	pop	si
	mov	sp,	bp
	pop	bp
	ret
_fmovescrmem	endp

_TEXT	ends
	end


