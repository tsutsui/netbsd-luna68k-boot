/*	$NetBSD$	*/

/*
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)samachdep.h	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <m68k/frame.h>
#include <lib/libsa/stand.h>

#define	NSCSI		2
#define NSD		8

#define MHZ_8		1
#define MHZ_16		2
#define MHZ_25		3
#define MHZ_33		4
#define MHZ_50		6

/* autoconf.c */
extern int cpuspeed;
void configure(void);

/* clock.c */
/* not yet */

/* cons.c */
void cninit(void);
int cngetc(void);
int cnputc(int);

/* devopen.c */
extern	u_int opendev;
int atoi(char *);

/* exec.c */
void exec_hp300(char *, u_long, int);

/* machdep.c */
extern	int userom;
char *getmachineid(void);
void romputchar(int);
void transfer(char *, int, int, int, char *, char *);
int trap(struct trapframe *);

/* prf.c */
int tgetchar(void);

/* locore.S */
extern	u_int bootdev;
extern	int howto;

/* machdep.c */
int badaddr(void *);

/* tgets.c */
int tgets(char *);


#define DELAY(n)							\
do {									\
	register int __N = cpuspeed * (n);				\
	do {								\
		__asm("subql #1, %0" : "=r" (__N) : "0" (__N));		\
	} while (__N > 0);						\
} while (/* CONSTCOND */ 0)

extern	struct fs_ops file_system_ufs[];
extern	struct fs_ops file_system_nfs[];

extern	char bootprog_name[], bootprog_rev[], bootprog_kernrev[];
