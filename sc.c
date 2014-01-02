/*	$NetBSD: sc.c,v 1.4 2013/01/22 15:48:40 tsutsui Exp $	*/

/*
 * Copyright (c) 1992 OMRON Corporation.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)sc.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	@(#)sc.c	8.1 (Berkeley) 6/10/93
 */

/*
 * sc.c -- SCSI Protocole Controller (SPC)  driver
 * remaked by A.Fujita, MAR-11-199
 */


#define NSC	2

#include <sys/param.h>
#include <luna68k/stand/boot/samachdep.h>
#include <luna68k/stand/boot/scsireg.h>
#include <luna68k/stand/boot/device.h>
#include <luna68k/stand/boot/scsivar.h>

#define SCSI_IPL	2
#define SCSI_ID		7

static int scinit(void *);
static void screset(int);
static int issue_select(struct scsidevice *, u_char);
static void ixfer_start(struct scsidevice *, int, u_char, int);
static void ixfer_out(struct scsidevice *, int, u_char *);
static void ixfer_in(struct scsidevice *, int, u_char *);
static int scrun(int, int, u_char *, int, u_char *, int, volatile int *);
static int scfinish(int);
static void scabort(struct scsi_softc *, struct scsidevice *);

struct	driver scdriver = {
	scinit, "sc", scintr,
};

struct	scsi_softc scsi_softc[NSC];

/*
 * Initialize SPC & Data Structure
 */

int
scinit(void *arg)
{
	struct hp_ctlr *hc = arg;
	struct scsi_softc *hs;
	int unit;

	unit = hc->hp_unit;
	if (unit < 0 || unit >= NSC)
		return 0;

	hs = &scsi_softc[unit];

	hc->hp_ipl    = SCSI_IPL;
	hs->sc_hc     = hc;

	hs->sc_flags  = 0;
	hs->sc_phase  = BUS_FREE_PHASE;
	hs->sc_target = SCSI_ID;

	hs->sc_cdb    = NULL;
	hs->sc_cdblen = 0;
	hs->sc_buf    = NULL;
	hs->sc_len    = 0;
	hs->sc_lock   = NULL;

	hs->sc_stat   = 0;
	hs->sc_msg[0] = 0;

	screset(hc->hp_unit);
	return(1);
}

void
screset(int unit)
{
	struct scsi_softc *hs = &scsi_softc[unit];
	struct scsidevice *hd = (struct scsidevice *)hs->sc_hc->hp_addr;

	printf("sc%d: ", unit);

	/*
	 * Disable interrupts then reset the FUJI chip.
	 */

	hd->scsi_sctl = SCTL_DISABLE | SCTL_CTRLRST;
	hd->scsi_scmd = 0;
	hd->scsi_pctl = 0;
	hd->scsi_temp = 0;
	hd->scsi_tch  = 0;
	hd->scsi_tcm  = 0;
	hd->scsi_tcl  = 0;
	hd->scsi_ints = 0;

	/* We can use Asynchronous Transfer only */
	printf("async");

	/*
	 * Configure MB89352 with its SCSI address, all
	 * interrupts enabled & appropriate parity.
	 */
	hd->scsi_bdid = SCSI_ID;
	hd->scsi_sctl = SCTL_DISABLE | SCTL_ABRT_ENAB|
			SCTL_PARITY_ENAB | SCTL_RESEL_ENAB |
			SCTL_INTR_ENAB;
	printf(", parity");

	DELAY(400);
	hd->scsi_sctl &= ~SCTL_DISABLE;

	printf(", scsi id %d\n", SCSI_ID);
}


/*
 * SPC Arbitration/Selection routine
 */

int
issue_select(struct scsidevice *hd, u_char target)
{
	hd->scsi_pctl = 0;
	hd->scsi_temp = (1 << SCSI_ID) | (1 << target);

	/* select timeout is hardcoded to 250ms */
	hd->scsi_tch = 2;
	hd->scsi_tcm = 113;
	hd->scsi_tcl = 3;

	hd->scsi_scmd = SCMD_SELECT;

	return (1);
}


/*
 * SPC Manual Transfer routines
 */

/* not yet */


/*
 * SPC Program Transfer routines
 */

void
ixfer_start(struct scsidevice *hd, int len, u_char phase, int wait)
{
	hd->scsi_tch  = ((len & 0xff0000) >> 16);
	hd->scsi_tcm  = ((len & 0x00ff00) >>  8);
	hd->scsi_tcl  =  (len & 0x0000ff);
	hd->scsi_pctl = phase;
	hd->scsi_scmd = SCMD_XFR | SCMD_PROG_XFR;
}

void
ixfer_out(struct scsidevice *hd, int len, u_char *buf)
{
	for(; len > 0; len--) {
		while (hd->scsi_ssts & SSTS_DREG_FULL) {
			DELAY(5);
		}
		hd->scsi_dreg = *buf++;
	}
}

void
ixfer_in(struct scsidevice *hd, int len, u_char *buf)
{
	for (; len > 0; len--) {
		while (hd->scsi_ssts & SSTS_DREG_EMPTY) {
			DELAY(5);
		}
		*buf++ = hd->scsi_dreg;
	}
}


/*
 * SPC drive routines
 */

int
scrun(int ctlr, int slave, u_char *cdb, int cdblen, u_char *buf, int len,
    volatile int *lock)
{
	struct scsi_softc *hs = &scsi_softc[ctlr];
	struct scsidevice *hd = (struct scsidevice *) hs->sc_hc->hp_addr;

	if (hd->scsi_ssts & (SSTS_INITIATOR|SSTS_TARGET|SSTS_BUSY))
		return(0);

	hs->sc_flags  = 0;
	hs->sc_phase  = ARB_SEL_PHASE;
	hs->sc_target = slave;

	hs->sc_cdb    = cdb;
	hs->sc_cdblen = cdblen;
	hs->sc_buf    = buf;
	hs->sc_len    = len;
	hs->sc_lock   = lock;

	hs->sc_stat   = 0;
	hs->sc_msg[0] = 0;

	*(hs->sc_lock) = SC_IN_PROGRESS;
	issue_select(hd, hs->sc_target);

	return(1);
}

int
scfinish(int ctlr)
{
	struct scsi_softc *hs = &scsi_softc[ctlr];
	int status = hs->sc_stat;

	hs->sc_flags  = 0;
	hs->sc_phase  = BUS_FREE_PHASE;
	hs->sc_target = SCSI_ID;

	hs->sc_cdb    = NULL;
	hs->sc_cdblen = 0;
	hs->sc_buf    = NULL;
	hs->sc_len    = 0;
	hs->sc_lock   = NULL;

	hs->sc_stat   = 0;
	hs->sc_msg[0] = 0;

	return(status);
}

void
scabort(struct scsi_softc *hs, struct scsidevice *hd)
{
	int len;
	u_char junk;

	printf("sc%d: abort  phase=0x%x, ssts=0x%x, ints=0x%x\n",
		hs->sc_hc->hp_unit, hd->scsi_psns, hd->scsi_ssts,
		hd->scsi_ints);

	if (hd->scsi_ints != 0)
		hd->scsi_ints = hd->scsi_ints;

	if (hd->scsi_psns == 0 || (hd->scsi_ssts & SSTS_INITIATOR) == 0)
		/* no longer connected to scsi target */
		return;

	/* get the number of bytes remaining in current xfer + fudge */
	len = (hd->scsi_tch << 16) | (hd->scsi_tcm << 8) | hd->scsi_tcl;

	/* for that many bus cycles, try to send an abort msg */
	for (len += 1024; (hd->scsi_ssts & SSTS_INITIATOR) && --len >= 0; ) {
		hd->scsi_scmd = SCMD_SET_ATN;

		while ((hd->scsi_psns & PSNS_REQ) == 0) {
			if (! (hd->scsi_ssts & SSTS_INITIATOR))
				goto out;
			DELAY(1);
		}

		if ((hd->scsi_psns & PHASE) == MESG_OUT_PHASE)
			hd->scsi_scmd = SCMD_RST_ATN;
		hd->scsi_pctl = hs->sc_phase = hd->scsi_psns & PHASE;

		if (hd->scsi_psns & PHASE_IO) {
			/* one of the input phases - read & discard a byte */
			hd->scsi_scmd = SCMD_SET_ACK;
			while (hd->scsi_psns & PSNS_REQ)
				DELAY(1);
			junk = hd->scsi_temp;
		} else {
			/* one of the output phases - send an abort msg */
			hd->scsi_temp = MSG_ABORT;
			hd->scsi_scmd = SCMD_SET_ACK;
			while (hd->scsi_psns & PSNS_REQ)
				DELAY(1);
		}

		hd->scsi_scmd = SCMD_RST_ACK;
	}
out:
	/*
	 * Either the abort was successful & the bus is disconnected or
	 * the device didn't listen.  If the latter, announce the problem.
	 * Either way, reset the card & the SPC.
	 */
	if (len < 0 && hs)
		printf("sc%d: abort failed.  phase=0x%x, ssts=0x%x\n",
			hs->sc_hc->hp_unit, hd->scsi_psns, hd->scsi_ssts);
}


/*
 * SCSI Command Handler
 */

int
scsi_test_unit_rdy(int ctlr, int slave, int unit)
{
	static struct scsi_cdb6 cdb = { CMD_TEST_UNIT_READY };
	int status;
	volatile int lock;

#ifdef DEBUG
	printf("scsi_test_unit_rdy( %d, %d, %d): Start\n", ctlr, slave, unit);
#endif

	cdb.lun = unit;

	if (!(scrun(ctlr, slave, (void *)&cdb, 6, NULL, 0, &lock))) {
#ifdef DEBUG
		printf("scsi_test_unit_rdy: Command Transfer Failed.\n");
#endif
		return(-1);
	}

	while ((lock == SC_IN_PROGRESS) || (lock == SC_DISCONNECTED))
		DELAY(10);

	status = scfinish(ctlr);

	if (lock == SC_IO_COMPLETE) {
#ifdef DEBUG
		printf("scsi_test_unit_rdy: Status -- 0x%x\n", status);
#endif
		return(status);
	} else {
		return(lock);
	}
}

int
scsi_request_sense(int ctlr, int slave, int unit, u_char *buf, unsigned int len)
{
	static struct scsi_cdb6 cdb = {	CMD_REQUEST_SENSE };
	int status;
	volatile int lock;

#ifdef DEBUG
	printf("scsi_request_sense: Start\n");
#endif

	/* Request Sense$N>l9g!"E>Aw$5$l$k%G!<%?D9$O%?!<%2368H$K0MB8$7!"        */
	/* %;%s%9%G!<%?$N#8/usr/src/sys/luna68k/stand/SCCS/s.sc.c$%HL\$NAddtional Sens Length$K$h$jF0E*$K7hDj$9$k!#*/
	/* $3$3$G$O%G!<%?!<E>Aw?t$rcdb$NAllocation Length$K:GDcD9$G$"$k#8/usr/src/sys/luna68k/stand/SCCS/s.sc.c$%H */
	/* $r8GDj$7$F!"#S#P#C$N=hM}%7!<%1%s%9$rJx$5$J$$$h$&$K$7$F$$$k!#         */

	/* %F!<@(#)sc.c	8.1f%K373H$N>uBV$rD4$Y$k$?$a!"Addtional Sens Field$r%"%/%;%9$9$k */
	/* I,MW$,$"$k$N$G6/10/93P%$%98.1i%$%PB&$Glen$r7hDj$9$k$3$H$K$9$k            */

	cdb.lun = unit;
	cdb.len = len;

	if (!(scrun(ctlr, slave, (void *)&cdb, 6, buf, len, &lock))) {
#ifdef DEBUG
		printf("scsi_request_sense: Command Transfer Failed.\n");
#endif
		return(-1);
	}

	while ((lock == SC_IN_PROGRESS) || (lock == SC_DISCONNECTED))
		DELAY(10);

	status = scfinish(ctlr);

	if (lock == SC_IO_COMPLETE) {
#ifdef DEBUG
		printf("scsi_request_sense: Status -- 0x%x\n", status);
#endif
		return(status);
	} else {
		return(lock);
	}
}

int
scsi_immed_command(int ctlr, int slave, int unit, struct scsi_fmt_cdb *cdb,
    u_char *buf, unsigned int len)
{
	int status;
	volatile int lock;

#ifdef DEBUG
	printf("scsi_immed_command( %d, %d, %d, cdb(%d), buf, %d): Start\n",
	       ctlr, slave, unit, cdb->len, len);
#endif

	cdb->cdb[1] |= unit << 5;

	if (!(scrun(ctlr, slave, (void *)&cdb->cdb[0], cdb->len, buf, len, &lock))) {
#ifdef DEBUG
		printf("scsi_immed_command: Command Transfer Failed.\n");
#endif
		return(-1);
	}

	while ((lock == SC_IN_PROGRESS) || (lock == SC_DISCONNECTED))
		DELAY(10);

	status = scfinish(ctlr);

	if (lock == SC_IO_COMPLETE) {
#ifdef DEBUG
		printf("scsi_immed_command: Status -- 0x%x\n", status);
#endif
		return(status);
	} else {
		return(lock);
	}
}

int
scsi_format_unit(int ctlr, int slave, int unit)
{
	static struct scsi_cdb6 cdb = { CMD_FORMAT_UNIT, 0, 0, 0, 0, 0 };
	int status;
	volatile int lock;
#ifdef DEBUG
	int count = 0;
#endif

#ifdef DEBUG
	printf("scsi_format_unit( %d, %d, %d): Start\n", ctlr, slave, unit);
#endif

	cdb.lun = unit;

	if (!(scrun(ctlr, slave, (void *)&cdb, 6, (u_char *) 0, 0, &lock))) {
#ifdef DEBUG
		printf("scsi_format_unit: Command Transfer Failed.\n");
#endif
		return(-1);
	}

	while ((lock == SC_IN_PROGRESS) || (lock == SC_DISCONNECTED)) {
		DELAY(1000000);
#ifdef DEBUG
		if ((++count % 60) == 0)
			printf("scsi_format_unit: %d\n", count / 60);
#endif
	}

	status = scfinish(ctlr);

	if (lock == SC_IO_COMPLETE) {
#ifdef DEBUG
		printf("scsi_format_unit: Status -- 0x%x\n", status);
#endif
		return(status);
	} else {
		return(lock);
	}
}


/*
 * Interrupt Routine
 */

int
scintr(void)
{
	struct scsi_softc *hs;
	struct scsidevice *hd;
	u_char ints, temp;
	int i;
	u_char *buf;
	int len;

	for (i = 0; i < NSC; i++) {
		hs = &scsi_softc[i];
		hd = (struct scsidevice *) hs->sc_hc->hp_addr;
		if ((ints = hd->scsi_ints) != 0)
			goto get_intr;
	}

	/* Unknown Interrupt occured */
	return -1;


	/*
	 * Interrupt
	 */

 get_intr:
#ifdef DEBUG
	printf("scintr: INTS 0x%x, SSTS 0x%x,  PCTL 0x%x,  PSNS 0x%x    0x%x\n",
	        ints, hd->scsi_ssts, hd->scsi_pctl, hd->scsi_psns,
	        hs->sc_phase);
#endif
	if (ints & INTS_RESEL) {
		if (hs->sc_phase == BUS_FREE_PHASE) {
			temp = hd->scsi_temp & ~(1 << SCSI_ID);
			for (i = 0; temp != 1; i++) {
				temp >>= 1;
			}
			hs->sc_target = i;
			*(hs->sc_lock) = SC_IN_PROGRESS;
		} else
			goto abort;
	} else if (ints & INTS_DISCON) {
		if ((hs->sc_msg[0] == MSG_CMD_COMPLETE) || (hs->sc_msg[0] == MSG_DISCONNECT)) {
			hs->sc_phase  = BUS_FREE_PHASE;
			hs->sc_target = SCSI_ID;
			if (hs->sc_msg[0] == MSG_CMD_COMPLETE)
				/* SCSI IO complete */
				*(hs->sc_lock) = SC_IO_COMPLETE;
			else
				/* Cisconnected from Target */
				*(hs->sc_lock) = SC_DISCONNECTED;
			hd->scsi_ints = ints;
			return 0;
		} else
			goto abort;
	} else if (ints & INTS_CMD_DONE) {
		if (hs->sc_phase == BUS_FREE_PHASE)
			goto abort;
		else if (hs->sc_phase == MESG_IN_PHASE) {
			hd->scsi_scmd = SCMD_RST_ACK;
			hd->scsi_ints = ints;
			hs->sc_phase  = hd->scsi_psns & PHASE;
			return 0;
		}
		if (hs->sc_flags & SC_SEL_TIMEOUT)
			hs->sc_flags &= ~SC_SEL_TIMEOUT;
	} else if (ints & INTS_SRV_REQ) {
		if (hs->sc_phase != MESG_IN_PHASE)
			goto abort;
	} else if (ints & INTS_TIMEOUT) {
		if (hs->sc_phase == ARB_SEL_PHASE) {
			if (hs->sc_flags & SC_SEL_TIMEOUT) {
				hs->sc_flags &= ~SC_SEL_TIMEOUT;
				hs->sc_phase  = BUS_FREE_PHASE;
				hs->sc_target = SCSI_ID;
				/* Such SCSI Device is not conected. */
				*(hs->sc_lock) = SC_DEV_NOT_FOUND;
				hd->scsi_ints = ints;
				return 0;
			} else {
				/* wait more 250 usec */
				hs->sc_flags |= SC_SEL_TIMEOUT;
				hd->scsi_temp = 0;
				hd->scsi_tch  = 0;
				hd->scsi_tcm  = 0x06;
				hd->scsi_tcl  = 0x40;
				hd->scsi_ints = ints;
				return 0;
			}
		} else
			goto abort;
	} else
		goto abort;

	hd->scsi_ints = ints;

	/*
	 * Next SCSI Transfer
	 */

	while ((hd->scsi_psns & PSNS_REQ) == 0) {
		DELAY(1);
	}

	hs->sc_phase = hd->scsi_psns & PHASE;

	if ((hs->sc_phase == DATA_OUT_PHASE) || (hs->sc_phase == DATA_IN_PHASE)) {
		len = hs->sc_len;
		buf = hs->sc_buf;
	} else if (hs->sc_phase == CMD_PHASE) {
		len = hs->sc_cdblen;
		buf = hs->sc_cdb;
	} else if (hs->sc_phase == STATUS_PHASE) {
		len = 1;
		buf = &hs->sc_stat;
	} else {
		len = 1;
		buf = hs->sc_msg;
	}

	ixfer_start(hd, len, hs->sc_phase, 0);
	if (hs->sc_phase & PHASE_IO)
		ixfer_in(hd, len, buf);
	else
		ixfer_out(hd, len, buf);

	return 0;

	/*
	 * SCSI Abort
	 */
 abort:
	/* SCSI IO failed */
	scabort(hs, hd);
	hd->scsi_ints = ints;
	*(hs->sc_lock) = SC_IO_FAILED;
	return -1;
}
