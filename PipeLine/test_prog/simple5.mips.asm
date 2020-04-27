
simple5.o:     file format elf32-tradbigmips


Disassembly of section .text:

00000000 <chk_prime>:
   0:	27bdffd0 	addiu	sp,sp,-48
   4:	afbf002c 	sw	ra,44(sp)
   8:	afbe0028 	sw	s8,40(sp)
   c:	03a0f025 	move	s8,sp
  10:	afc40030 	sw	a0,48(s8)
  14:	afc00020 	sw	zero,32(s8)
  18:	24020001 	li	v0,1
  1c:	afc2001c 	sw	v0,28(s8)
  20:	24020127 	li	v0,295
  24:	afc20030 	sw	v0,48(s8)
  28:	24040002 	li	a0,2
  2c:	8fc50030 	lw	a1,48(s8)
  30:	0c000000 	jal	0 <chk_prime>
  34:	00000000 	nop
  38:	afc20020 	sw	v0,32(s8)
  3c:	8fc30020 	lw	v1,32(s8)
  40:	24020001 	li	v0,1
  44:	10620005 	beq	v1,v0,5c <chk_prime+0x5c>
  48:	00000000 	nop
  4c:	afc0001c 	sw	zero,28(s8)
  50:	8fc2001c 	lw	v0,28(s8)
  54:	1000001e 	b	d0 <chk_prime+0xd0>
  58:	00000000 	nop
  5c:	24020003 	li	v0,3
  60:	afc20018 	sw	v0,24(s8)
  64:	10000011 	b	ac <chk_prime+0xac>
  68:	00000000 	nop
  6c:	8fc40018 	lw	a0,24(s8)
  70:	8fc50030 	lw	a1,48(s8)
  74:	0c000000 	jal	0 <chk_prime>
  78:	00000000 	nop
  7c:	afc20020 	sw	v0,32(s8)
  80:	8fc30020 	lw	v1,32(s8)
  84:	24020001 	li	v0,1
  88:	10620004 	beq	v1,v0,9c <chk_prime+0x9c>
  8c:	00000000 	nop
  90:	afc0001c 	sw	zero,28(s8)
  94:	1000000d 	b	cc <chk_prime+0xcc>
  98:	00000000 	nop
  9c:	8fc20018 	lw	v0,24(s8)
  a0:	00000000 	nop
  a4:	24420002 	addiu	v0,v0,2
  a8:	afc20018 	sw	v0,24(s8)
  ac:	8fc20030 	lw	v0,48(s8)
  b0:	00000000 	nop
  b4:	00021843 	sra	v1,v0,0x1
  b8:	8fc20018 	lw	v0,24(s8)
  bc:	00000000 	nop
  c0:	0043102a 	slt	v0,v0,v1
  c4:	1440ffe9 	bnez	v0,6c <chk_prime+0x6c>
  c8:	00000000 	nop
  cc:	8fc2001c 	lw	v0,28(s8)
  d0:	03c0e825 	move	sp,s8
  d4:	8fbf002c 	lw	ra,44(sp)
  d8:	8fbe0028 	lw	s8,40(sp)
  dc:	27bd0030 	addiu	sp,sp,48
  e0:	03e00008 	jr	ra
  e4:	00000000 	nop

000000e8 <gcd>:
  e8:	27bdffe0 	addiu	sp,sp,-32
  ec:	afbf001c 	sw	ra,28(sp)
  f0:	afbe0018 	sw	s8,24(sp)
  f4:	03a0f025 	move	s8,sp
  f8:	afc40020 	sw	a0,32(s8)
  fc:	afc50024 	sw	a1,36(s8)
 100:	8fc30020 	lw	v1,32(s8)
 104:	8fc20024 	lw	v0,36(s8)
 108:	00000000 	nop
 10c:	14620004 	bne	v1,v0,120 <gcd+0x38>
 110:	00000000 	nop
 114:	8fc20020 	lw	v0,32(s8)
 118:	10000019 	b	180 <gcd+0x98>
 11c:	00000000 	nop
 120:	8fc30020 	lw	v1,32(s8)
 124:	8fc20024 	lw	v0,36(s8)
 128:	00000000 	nop
 12c:	0043102a 	slt	v0,v0,v1
 130:	1040000b 	beqz	v0,160 <gcd+0x78>
 134:	00000000 	nop
 138:	8fc30020 	lw	v1,32(s8)
 13c:	8fc20024 	lw	v0,36(s8)
 140:	00000000 	nop
 144:	00621023 	subu	v0,v1,v0
 148:	00402025 	move	a0,v0
 14c:	8fc50024 	lw	a1,36(s8)
 150:	0c000000 	jal	0 <chk_prime>
 154:	00000000 	nop
 158:	10000009 	b	180 <gcd+0x98>
 15c:	00000000 	nop
 160:	8fc30024 	lw	v1,36(s8)
 164:	8fc20020 	lw	v0,32(s8)
 168:	00000000 	nop
 16c:	00621023 	subu	v0,v1,v0
 170:	00402025 	move	a0,v0
 174:	8fc50020 	lw	a1,32(s8)
 178:	0c000000 	jal	0 <chk_prime>
 17c:	00000000 	nop
 180:	03c0e825 	move	sp,s8
 184:	8fbf001c 	lw	ra,28(sp)
 188:	8fbe0018 	lw	s8,24(sp)
 18c:	27bd0020 	addiu	sp,sp,32
 190:	03e00008 	jr	ra
 194:	00000000 	nop
	...
