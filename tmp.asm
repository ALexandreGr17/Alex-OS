
bin/tmp.o:     file format binary


Disassembly of section .data:

00000000 <.data>:
   0:	fa                   	cli    
   1:	b8 00 10             	mov    ax,0x1000
   4:	8e d0                	mov    ss,ax
   6:	bc 00 b0             	mov    sp,0xb000
   9:	b8 00 00             	mov    ax,0x0
   c:	8e d8                	mov    ds,ax
   e:	8e c0                	mov    es,ax
  10:	fb                   	sti    
  11:	be 00 7c             	mov    si,0x7c00
  14:	bf 00 06             	mov    di,0x600
  17:	b9 00 02             	mov    cx,0x200
  1a:	f3 a4                	rep movs BYTE PTR es:[di],BYTE PTR ds:[si]
  1c:	ea 21 06 00 00       	jmp    0x0:0x621
  21:	be be 07             	mov    si,0x7be
  24:	38 04                	cmp    BYTE PTR [si],al
  26:	75 0b                	jne    0x33
  28:	83 c6 10             	add    si,0x10
  2b:	81 fe fe 07          	cmp    si,0x7fe
  2f:	75 f3                	jne    0x24
  31:	eb 16                	jmp    0x49
  33:	b4 02                	mov    ah,0x2
  35:	b0 01                	mov    al,0x1
  37:	bb 00 7c             	mov    bx,0x7c00
  3a:	b2 80                	mov    dl,0x80
  3c:	8a 74 01             	mov    dh,BYTE PTR [si+0x1]
  3f:	8b 4c 02             	mov    cx,WORD PTR [si+0x2]
  42:	cd 13                	int    0x13
  44:	ea 00 7c 00 00       	jmp    0x0:0x7c00
  49:	eb fe                	jmp    0x49
	...
 1b7:	00 0f                	add    BYTE PTR [bx],cl
 1b9:	d3 1f                	rcr    WORD PTR [bx],cl
 1bb:	ea 00 00 80 00       	jmp    0x80:0x0
 1c0:	01 10                	add    WORD PTR [bx+si],dx
 1c2:	83 03 20             	add    WORD PTR [bp+di],0x20
 1c5:	ff 00                	inc    WORD PTR [bx+si]
 1c7:	08 00                	or     BYTE PTR [bx+si],al
 1c9:	00 00                	add    BYTE PTR [bx+si],al
 1cb:	78 00                	js     0x1cd
	...
 1fd:	00 55 aa             	add    BYTE PTR [di-0x56],dl
