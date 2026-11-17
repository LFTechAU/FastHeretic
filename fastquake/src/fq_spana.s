//
// fq_spana.s: FastQuake 486 span segment drawer (LFTech). GPL v2.
//
// void FQ_SpanSeg (byte *pdest, byte *psrc, int sfrac, int tfrac,
//                  int sstepfrac, int count)
// fq_tstepfrac and fq_advancetable[2] must be set up by the caller.
// count is 1..64
//
#include "asm_i386.h"
#include "quakeasm.h"

	.data
	.align 4
.globl C(fq_advancetable), C(fq_tstepfrac)
C(fq_advancetable):	.long 0, 0
C(fq_tstepfrac):	.long 0

	.text
#define PIX(i) \
	movb	(%esi),%al ; \
	addl	C(fq_tstepfrac),%edx ; \
	sbbl	%ecx,%ecx ; \
	addl	%ebp,%ebx ; \
	adcl	C(fq_advancetable)+4(,%ecx,4),%esi ; \
	movb	%al,i(%edi)

	.align 4
.globl C(FQ_SpanSeg)
C(FQ_SpanSeg):
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	20(%esp),%edi
	movl	24(%esp),%esi
	movl	28(%esp),%ebx
	movl	32(%esp),%edx
	movl	36(%esp),%ebp
	movl	40(%esp),%ecx
	movl	%ecx,%eax
	subl	$64,%eax
	addl	%eax,%edi		// edi = pdest - (64 - count)
	jmp		*Lentry(,%ecx,4)

	.align 4
LP0:	PIX(0)
LP1:	PIX(1)
LP2:	PIX(2)
LP3:	PIX(3)
LP4:	PIX(4)
LP5:	PIX(5)
LP6:	PIX(6)
LP7:	PIX(7)
LP8:	PIX(8)
LP9:	PIX(9)
LP10:	PIX(10)
LP11:	PIX(11)
LP12:	PIX(12)
LP13:	PIX(13)
LP14:	PIX(14)
LP15:	PIX(15)
LP16:	PIX(16)
LP17:	PIX(17)
LP18:	PIX(18)
LP19:	PIX(19)
LP20:	PIX(20)
LP21:	PIX(21)
LP22:	PIX(22)
LP23:	PIX(23)
LP24:	PIX(24)
LP25:	PIX(25)
LP26:	PIX(26)
LP27:	PIX(27)
LP28:	PIX(28)
LP29:	PIX(29)
LP30:	PIX(30)
LP31:	PIX(31)
LP32:	PIX(32)
LP33:	PIX(33)
LP34:	PIX(34)
LP35:	PIX(35)
LP36:	PIX(36)
LP37:	PIX(37)
LP38:	PIX(38)
LP39:	PIX(39)
LP40:	PIX(40)
LP41:	PIX(41)
LP42:	PIX(42)
LP43:	PIX(43)
LP44:	PIX(44)
LP45:	PIX(45)
LP46:	PIX(46)
LP47:	PIX(47)
LP48:	PIX(48)
LP49:	PIX(49)
LP50:	PIX(50)
LP51:	PIX(51)
LP52:	PIX(52)
LP53:	PIX(53)
LP54:	PIX(54)
LP55:	PIX(55)
LP56:	PIX(56)
LP57:	PIX(57)
LP58:	PIX(58)
LP59:	PIX(59)
LP60:	PIX(60)
LP61:	PIX(61)
LP62:	PIX(62)
LP63:	PIX(63)
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

	.data
	.align 4
Lentry:
	.long LP63, LP63, LP62, LP61, LP60, LP59, LP58, LP57, LP56, LP55, LP54, LP53, LP52, LP51, LP50, LP49, LP48
	.long LP47, LP46, LP45, LP44, LP43, LP42, LP41, LP40, LP39, LP38, LP37, LP36, LP35, LP34, LP33, LP32
	.long LP31, LP30, LP29, LP28, LP27, LP26, LP25, LP24, LP23, LP22, LP21, LP20, LP19, LP18, LP17, LP16
	.long LP15, LP14, LP13, LP12, LP11, LP10, LP9, LP8, LP7, LP6, LP5, LP4, LP3, LP2, LP1, LP0
