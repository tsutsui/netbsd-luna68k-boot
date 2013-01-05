#	$NetBSD$
#	@(#)Makefile	8.2 (Berkeley) 8/15/93

NOMAN= # defined

.include <bsd.own.mk>
.include <bsd.sys.mk>

S= ${.CURDIR}/../../../..

INCLUDES= -I${.CURDIR} -I${.CURDIR}/$S 
COPTS=	${INCLUDES} ${IDENT} -DKERNEL
CPPFLAGS+=	-nostdinc -D_STANDALONE
CPPFLAGS+=	-I${.OBJDIR} -I${S} -I${S}/arch

CPPFLAGS+=	-DSUPPORT_DISK
#CPPFLAGS+=	-DSUPPORT_TAPE
#CPPFLAGS+=	-DSUPPORT_ETHERNET
#CPPFLAGS+=	-DSUPPORT_DHCP -DSUPPORT_BOOTP
#CPPFLAGS+=	-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG
#CPPFLAGS+=	-DRPC_DEBUG -DRARP_DEBUG -DNET_DEBUG -DDEBUG -DPARANOID

CFLAGS=		-Os -msoft-float
CFLAGS+=	-ffreestanding
CFLAGS+=	-Wall -Werror
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
CFLAGS+=	-Wno-pointer-sign

LDSCRIPT=	${.CURDIR}/boot.ldscript
LINKFORMAT=	-static -N -T ${LDSCRIPT}

SRCS=	locore.S
SRCS+=	init_main.c autoconf.c ioconf.c
SRCS+=	trap.c
SRCS+=	devopen.c
SRCS+=	conf.c
SRCS+=	machdep.c
SRCS+=	getline.c parse.c 
SRCS+=	boot.c
SRCS+=	cons.c prf.c
SRCS+=	romcons.c
SRCS+=	sio.c
SRCS+=	bmc.c bmd.c screen.c font.c kbd.c
SRCS+=	scsi.c sc.c sd.c
#SRCS+=	st.c tape.c
SRCS+=	disklabel.c
#SRCS+=	fsdump.c
SRCS+=	ufs_disksubr.c

PROG=   boot

SRCS+=          vers.c
CLEANFILES+=    vers.c

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"

### find out what to use for libz
Z_AS=		library
.include "${S}/lib/libz/Makefile.inc"

### find out what to use for libsa
SA_AS=		library
SAMISCMAKEFLAGS+=SA_USE_LOADFILE=yes SA_USE_CREAD=yes
.include "${S}/lib/libsa/Makefile.inc"

LIBS=	${SALIB} ${ZLIB} ${KERNLIB}

.PHONY: vers.c
vers.c: ${.CURDIR}/version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${${MKREPRO} == "yes" :?:-D} \
	    ${.CURDIR}/version "${MACHINE}"

${PROG}: ${LDSCRIPT} ${OBJS} ${LIBS}
	${LD} ${LINKFORMAT} -x -o ${PROG}.elf ${OBJS} ${LIBS}
	${ELF2AOUT} ${PROG}.elf ${PROG}.aout
	mv ${PROG}.aout ${PROG}

CLEANFILES+=	${PROG}.map ${PROG}.elf ${PROG}.gz

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
