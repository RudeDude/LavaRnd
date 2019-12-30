#!/usr/bin/make
#
# tool - misc LavaRnd development and testing tools
#
# @(#) $Revision: 10.2 $
# @(#) $Id: Makefile,v 10.2 2003/08/25 11:14:07 lavarnd Exp $
#
# Copyright (c) 2000-2003 by Landon Curt Noll and Simon Cooper.
# All Rights Reserved.
#
# This is open software; you can redistribute it and/or modify it under
# the terms of the version 2.1 of the GNU Lesser General Public License
# as published by the Free Software Foundation.
#
# This software is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General
# Public License for more details.
#
# The file COPYING contains important info about Licenses and Copyrights.
# Please read the COPYING file for details about this open software.
#
# A copy of version 2.1 of the GNU Lesser General Public License is
# distributed with calc under the filename COPYING-LGPL.  You should have
# received a copy with calc; if not, write to Free Software Foundation, Inc.
# 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
#
# For more information on LavaRnd: http://www.LavaRnd.org
#
# Share and enjoy! :-)

# setup
#
SHELL= /bin/sh
AWK= awk
BASENAME= basename
CAT= cat
CC= cc
CHMOD= chmod
CHOWN= chown
CI= ci
CMP= cmp
CO= co
CP= cp
CTAGS= ctags
DATE= date
DIRNAME= dirname
EGREP= egrep
FGREP= fgrep
FIND= find
GROUPADD= groupadd
HEAD= head
INSTALL= install
LDD= ldd
LN= ln
MAKE= make
MAKEDEPEND= makedepend
MKDIR= mkdir
MV= mv
PERL= perl
POD2MAN= pod2man
RM= rm
RSYNC= rsync
SED= sed
SORT= sort
TAIL= tail
TAR= tar
USERADD= useradd
XARGS= xargs

# compile flags
#
#LAVA_DEBUG=
LAVA_DEBUG= -DLAVA_DEBUG

DMALLOC_CC=
DMALLOC_LIB=
#DMALLOC_CC= -DDMALLOC
#DMALLOC_LIB= -ldmalloc

#CC_WARN=
CC_WARN= -Wall -Werror

#CC_OPT=
CC_OPT= -O3

#CC_DBG= ${CC_OPT}
#CC_DBG= -g ${CC_OPT}
CC_DBG= -g3 ${CC_OPT}

# where to install
#
# DESTBIN	Locally installed program bin directory
# DESTSBIN	Locally installed program etc directory and daemons
# DESTLIB	Locally installed local libraries
# DESTINC	Locally installed *.h header files
# DOCDIR	System wide documentation directory
# TOOLDIR	Where LavaRnd tools are installed (usually DESTBIN/lavatool)
# RC_DIR	Where the rc startup scripts are installed
# CFGDIR	LavaRnd configuration directory
#
DESTBIN= /usr/bin
DESTSBIN= /usr/sbin
DESTLIB= /usr/lib
DESTINC= /usr/include
DOCDIR= /usr/share/doc
TOOLDIR= ${DESTBIN}/lavatool
RC_DIR= /etc/rc.d/init.d
CFGDIR= /etc/LavaRnd

# user and host information
#
# LAVARND_VIDEODEV	special device file for the LavaRnd webcam
# LAVARND_CAMTYPE	camera type (use a name given by: tool/camset list all)
# LAVARND_USER		username under which lavapool other lavarnd daemons run
# LAVARND_GROUP		group under which lavapool other lavarnd daemons run
# LAVAPOOL_CHROOT	lavapool chroots under this directory at startup
#
LAVARND_VIDEODEV=/dev/video0
LAVARND_CAMTYPE= pwc730
LAVARND_USER=lava
LAVARND_GROUP=lava
LAVAPOOL_CHROOT=/

# distribution
#
# DIST		sub-directory under . where external distributions are formed
# BASESUBDIST	basename sub-directory under ${DIST} to form a tarball
# SUBDIST	sub-directory under ${DIST} used to form a tarball
# DISTDIR	source release path
# TARBALL	make of source tarball installed under DISTDIR
#
DIST= ./dist
BASESUBDIST= tool
SUBDIST= LavaRnd/${BASESUBDIST}
DISTDIR= /var/tmp/LavaRnd-src
TARBALL= tool.tar.gz

#########################################
#=-=-= end of common Makefile vars =-=-=#
#########################################

# source
#
CSRC= imgtally.c camset.c camget.c camdump.c camdumpdir.c camsanity.c \
	ppmhead.c lavadump.c lavaop.c baseconv.c \
	lavaop_i.c chk_lavarnd.c tryrnd.c poolout.c \
	yuv2ppm.c y2grey.c yuv2rgb.c y2yuv.c y2pseudoyuv.c
HSRC= chi_tbl.h yuv2rgb.h
SHSRC= test_tryrnd test_perllib unload_modules

# what to build
#
BUILT_SRC=
OBJS= imgtally.o camset.o camget.o camdump.o camdumpdir.o camsanity.o \
	ppmhead.o lavadump.o lavaop.o baseconv.o \
	lavaop_i.o chk_lavarnd.o tryrnd.o poolout.o \
	yuv2ppm.o y2grey.o yuv2rgb.o y2yuv.o y2pseudoyuv.o
TRYRND= tryrnd_exit tryrnd_retry tryrnd_return tryrnd_s100_high \
	tryrnd_s100_med tryrnd_s100_any tryrnd_try_high tryrnd_try_med \
	tryrnd_try_any tryrnd_tryonce_high tryrnd_tryonce_med \
	tryrnd_tryonce_any
PROGS= imgtally camset camget camdump camdumpdir camsanity \
	ppmhead lavadump lavaop baseconv \
	lavaop_i chk_lavarnd poolout \
	yuv2ppm y2grey y2yuv y2pseudoyuv
SRC= ${HSRC} ${CSRC} ${BUILT_SRC}
TARGETS= ${TRYRND} ${PROGS} ${SHSRC}

# C compiler control
#
CC_PIC=
#CC_PIC= -fPIC

#SHARED= -shared
#SHARED= -static
SHARED=

CC_INC= -I../lib

#LSUF= .a
LSUF= .so

#LDIR= ../lib
LDIR= ../lib/shared

#LD_LIB= -L${LDIR}
LD_LIB= -Wl,-rpath=${DESTLIB} -L${LDIR}

CFLAGS= ${LAVA_DEBUG} ${DMALLOC_CC} ${CC_WARN} ${CC_DBG} ${CC_PIC} ${SHARED} ${CC_INC}
CLINK= ${CC_DBG} ${LD_LIB} ${DMALLOC_LIB} ${SHARED}
LDFLAGS= ${CC_OPT} ${DMALLOC_LIB} ${SHARED}

# Force the modes on directories by setting FORCE to a non-empty string
#
FORCE=

#####################################
#=-=-= start of Makefile rules =-=-=#
#####################################

all: ${TARGETS} install.sed dist.sed COPYING COPYING-LGPL

imgtally.o: imgtally.c chi_tbl.h
	${CC} ${CFLAGS} -Wno-unused-variable imgtally.c -c

imgtally: imgtally.o ${LDIR}/libLavaRnd_cam${LSUF} \
		     ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} imgtally.o -lLavaRnd_cam -lLavaRnd_util -lm -o imgtally

camset.o: camset.c
	${CC} ${CFLAGS} camset.c -c

camset: camset.o ${LDIR}/libLavaRnd_cam${LSUF} ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} camset.o -lLavaRnd_cam \
		-lLavaRnd_util -lm -o camset

camget.o: camget.c
	${CC} ${CFLAGS} camget.c -c

camget: camget.o ${LDIR}/libLavaRnd_cam${LSUF} ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} camget.o -lLavaRnd_cam \
		-lLavaRnd_util -lm -o camget

camdump.o: camdump.c
	${CC} ${CFLAGS} camdump.c -c

camdump: camdump.o ${LDIR}/libLavaRnd_cam${LSUF} ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} camdump.o -lLavaRnd_cam \
	    -lLavaRnd_util -lm -o camdump

camdumpdir.o: camdumpdir.c
	${CC} ${CFLAGS} camdumpdir.c -c

camdumpdir: camdumpdir.o ${LDIR}/libLavaRnd_cam${LSUF} \
			 ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} camdumpdir.o -lLavaRnd_cam \
	    -lLavaRnd_util -lm -o camdumpdir

camsanity.o: camsanity.c
	${CC} ${CFLAGS} camsanity.c -c

camsanity: camsanity.o ${LDIR}/libLavaRnd_cam${LSUF} \
		       ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} camsanity.o -lLavaRnd_cam \
	    -lLavaRnd_util -lm -o camsanity

ppmhead: ppmhead.c
	${CC} ${CLINK} ppmhead.c -o ppmhead

lavadump.o: lavadump.c
	${CC} ${CFLAGS} lavadump.c -c

lavadump: lavadump.o ${LDIR}/libLavaRnd_cam${LSUF} \
		     ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} lavadump.o -lLavaRnd_cam \
	    -lLavaRnd_util -lm -o lavadump

yuv2rgb.o: yuv2rgb.c yuv2rgb.h
	${CC} ${CFLAGS} yuv2rgb.c -c

yuv2ppm.o: yuv2ppm.c yuv2rgb.h
	${CC} ${CFLAGS} yuv2ppm.c -c

yuv2ppm: yuv2rgb.o yuv2ppm.o
	${CC} ${CLINK} yuv2rgb.o yuv2ppm.o -o yuv2ppm

y2grey.o: y2grey.c
	${CC} ${CFLAGS} y2grey.c -c

y2grey: y2grey.o y2grey.o
	${CC} ${CLINK} y2grey.o -o y2grey

y2yuv.o: y2yuv.c
	${CC} ${CFLAGS} y2yuv.c -c

y2yuv: y2yuv.o y2yuv.o
	${CC} ${CLINK} y2yuv.o -o y2yuv

y2pseudoyuv.o: y2pseudoyuv.c
	${CC} ${CFLAGS} y2pseudoyuv.c -c

y2pseudoyuv: y2pseudoyuv.o
	${CC} ${CLINK} y2pseudoyuv.o -o y2pseudoyuv

lavaop.o: lavaop.c
	${CC} ${CFLAGS} lavaop.c -c

lavaop: lavaop.o ${LDIR}/libLavaRnd_raw${LSUF} ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} lavaop.o -lLavaRnd_raw \
		-lLavaRnd_util -lm -o lavaop

baseconv.o: baseconv.c
	${CC} ${CFLAGS} baseconv.c -c

baseconv: baseconv.o ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} baseconv.o -lLavaRnd_util -lm -o baseconv

lavaop_i.o: lavaop_i.c
	${CC} ${CFLAGS} lavaop_i.c -c

lavaop_i: lavaop_i.o ${LDIR}/libLavaRnd_raw${LSUF} \
		     ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} lavaop_i.o -lLavaRnd_raw \
		-lLavaRnd_util -lm -o lavaop_i

chk_lavarnd.o: chk_lavarnd.c
	${CC} ${CFLAGS} chk_lavarnd.c -c

chk_lavarnd: chk_lavarnd.o ${LDIR}/libLavaRnd_util${LSUF}
	${CC} ${CLINK} chk_lavarnd.o -lLavaRnd_util -lm -o chk_lavarnd

tryrnd_set: ${TRYRND}

tryrnd.o: tryrnd.c
	${CC} ${CFLAGS} tryrnd.c -c

tryrnd_exit: tryrnd.o ${LDIR}/liblava_exit${LSUF} \
		      ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_exit \
				 -llava_libc -o $@

tryrnd_retry: tryrnd.o ${LDIR}/liblava_retry${LSUF} \
		       ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_retry \
				 -llava_libc -o $@

tryrnd_return: tryrnd.o ${LDIR}/liblava_return${LSUF} \
			${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_return \
				 -llava_libc -o $@

tryrnd_s100_high: tryrnd.o ${LDIR}/liblava_s100_high${LSUF} \
			   ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_s100_high \
				 -llava_libc -o $@

tryrnd_s100_med: tryrnd.o ${LDIR}/liblava_s100_med${LSUF} \
			  ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_s100_med \
				 -llava_libc -o $@

tryrnd_s100_any: tryrnd.o ${LDIR}/liblava_s100_any${LSUF} \
			  ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_s100_any \
				 -llava_libc -o $@

tryrnd_try_high: tryrnd.o ${LDIR}/liblava_try_high${LSUF} \
			  ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_try_high \
				 -llava_libc -o $@

tryrnd_try_med: tryrnd.o ${LDIR}/liblava_try_med${LSUF} \
			 ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_try_med \
				 -llava_libc -o $@

tryrnd_try_any: tryrnd.o ${LDIR}/liblava_try_any${LSUF} \
			 ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_try_any \
				 -llava_libc -o $@

tryrnd_tryonce_high: tryrnd.o ${LDIR}/liblava_tryonce_high${LSUF} \
				 ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_tryonce_high \
				 -llava_libc -o $@

tryrnd_tryonce_med: tryrnd.o ${LDIR}/liblava_tryonce_med${LSUF} \
			     ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_tryonce_med \
				 -llava_libc -o $@

tryrnd_tryonce_any: tryrnd.o ${LDIR}/liblava_tryonce_any${LSUF} \
			     ${LDIR}/liblava_libc${LSUF}
	${CC} ${CLINK} tryrnd.o -llava_tryonce_any \
				 -llava_libc -o $@

poolout.o: poolout.c
	${CC} ${CFLAGS} poolout.c -c

poolout: poolout.o ${LDIR}/liblava_exit${LSUF}
	${CC} ${CLINK} poolout.o -llava_return -o $@

# rules to make required things from other directories
#

${LDIR}/libLavaRnd_util${LSUF}:
	cd ${LDIR}; $(MAKE) libLavaRnd_util${LSUF}

${LDIR}/libLavaRnd_cam${LSUF}:
	cd ${LDIR}; $(MAKE) libLavaRnd_cam${LSUF}

${LDIR}/libLavaRnd_raw${LSUF}:
	cd ${LDIR}; $(MAKE) libLavaRnd_raw${LSUF}

${LDIR}/liblava_exit${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_exit${LSUF}

${LDIR}/liblava_retry${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_retry${LSUF}

${LDIR}/liblava_return${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_return${LSUF}

${LDIR}/liblava_s100_high${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_s100_high${LSUF}

${LDIR}/liblava_s100_med${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_s100_med${LSUF}

${LDIR}/liblava_s100_any${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_s100_any${LSUF}

${LDIR}/liblava_try_high${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_try_high${LSUF}

${LDIR}/liblava_try_med${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_try_med${LSUF}

${LDIR}/liblava_try_any${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_try_any${LSUF}

${LDIR}/liblava_tryonce_high${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_tryonce_high${LSUF}

${LDIR}/liblava_tryonce_med${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_tryonce_med${LSUF}

${LDIR}/liblava_tryonce_any${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_tryonce_any${LSUF}

${LDIR}/liblava_libc${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_libc${LSUF}

../lib/LavaRnd/have/pwc_cam.h:
	cd ../lib/LavaRnd/have; $(MAKE) pwc_cam.h

../lib/LavaRnd/have/have_getcontext.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_getcontext.h

../lib/LavaRnd/have/have_getpgrp.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_getpgrp.h

../lib/LavaRnd/have/have_getppid.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_getppid.h

../lib/LavaRnd/have/have_getprid.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_getprid.h

../lib/LavaRnd/have/have_getpriority.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_getpriority.h

../lib/LavaRnd/have/have_getrlimit.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_getrlimit.h

../lib/LavaRnd/have/have_gettime.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_gettime.h

../lib/LavaRnd/have/have_rusage.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_rusage.h

../lib/LavaRnd/have/have_sbrk.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_sbrk.h

../lib/LavaRnd/have/have_statfs.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_statfs.h

../lib/LavaRnd/have/have_sys_resource.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_sys_resource.h

../lib/LavaRnd/have/have_sys_time.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_sys_time.h

../lib/LavaRnd/have/have_sys_times.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_sys_times.h

../lib/LavaRnd/have/have_time.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_time.h

../lib/LavaRnd/have/have_uid_t.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_uid_t.h

../lib/LavaRnd/have/have_ustat.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_ustat.h

../lib/LavaRnd/have/have_ustat_h.h:
	cd ../lib/LavaRnd/have; $(MAKE) have_ustat_h.h

../lib/LavaRnd/have/cam_videodev.h:
	cd ../lib/LavaRnd/have; $(MAKE) cam_videodev.h

../lib/LavaRnd/have/ov511_cam.h:
	cd ../lib/LavaRnd/have; $(MAKE) ov511_cam.h

# perform various tests of the installed LavaRnd system
#
# NOTE: Most of these tests assume that LavaRnd has been installed,
#	the perl modules have been installed and that the lavapool
#	daemon is up and running.
#
test: chk_lavarnd camget poolout baseconv test_tryrnd ${TRYRND} \
	    test_perllib ../perllib/LavaRnd
	@echo =-=-= testing the core LavaRnd algorithm =-=-=
	./chk_lavarnd
	@echo =-=-= listing supported webcams =-=-=
	./camget list all
	@echo =-=-= obtaining 32 octets from lavapool =-=-=
	@rm -f tmpfile
	./poolout -v 32 1 0 > tmpfile
	./baseconv b256 b16 < tmpfile
	@rm -f tmpfile
	@echo
	@echo =-=-= testing LavaRnd C API to lavapool =-=-=
	./test_tryrnd
	@echo =-=-= testing LavaRnd Perl API to lavapool =-=-=
	./test_perllib
	@echo =-=-= all tests passed - test complete =-=-=
	@echo LavaRnd is OK

# utility rules
#
tags: ${BUILT_SRC} Makefile
	${CTAGS} *.c *.h

clean:
	${RM} -f ${OBJS}
	${RM} -f install.sed dist.sed tmpfile
	${RM} -rf skel ${DIST}

clobber: clean
	${RM} -f ${BUILT_SRC}
	${RM} -f ${TRYRND} ${PROGS}

install: all
	@if [ ! -d "${DESTBIN}" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DESTBIN}"; \
	    ${MKDIR} -p "${DESTBIN}"; \
	    echo "${CHMOD} 0755 ${DESTBIN}"; \
	    ${CHMOD} 0755 "${DESTBIN}"; \
	    echo "${CHOWN} root.root ${DESTBIN}"; \
	    ${CHOWN} root.root "${DESTBIN}"; \
	fi
	@if [ ! -d "${TOOLDIR}" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${TOOLDIR}"; \
	    ${MKDIR} -p "${TOOLDIR}"; \
	    echo "${CHMOD} 0755 ${TOOLDIR}"; \
	    ${CHMOD} 0755 "${TOOLDIR}"; \
	    echo "${CHOWN} root.root ${TOOLDIR}"; \
	    ${CHOWN} root.root "${TOOLDIR}"; \
	fi
	@for file in ${TARGETS}; do \
	    i="`${BASENAME} $$file`"; \
	    if ! ${CMP} -s "$$file" "${TOOLDIR}/$$i"; then \
		${RM} -f "${TOOLDIR}/$$i.new"; \
		${CP} "$$file" "${TOOLDIR}/$$i.new"; \
		${CHMOD} 0555 "${TOOLDIR}/$$i.new"; \
		${MV} -f "${TOOLDIR}/$$i.new" "${TOOLDIR}/$$i"; \
		echo "$$i installed into ${TOOLDIR}/$$i"; \
	    fi; \
	done

# ccflags - output C compiler flag values
#
ccflags:
	@echo "${BASESUBDIST} CC= ${CC}"
	@echo "${BASESUBDIST} LD= ${LD}"
	@echo "${BASESUBDIST} LAVA_DEBUG= ${LAVA_DEBUG}"
	@echo "${BASESUBDIST} DMALLOC_CC= ${DMALLOC_CC}"
	@echo "${BASESUBDIST} DMALLOC_LIB= ${DMALLOC_LIB}"
	@echo "${BASESUBDIST} CC_WARN= ${CC_WARN}"
	@echo "${BASESUBDIST} CC_OPT= ${CC_OPT}"
	@echo "${BASESUBDIST} CC_DBG= ${CC_DBG}"
	@echo "${BASESUBDIST} CC_PIC= ${CC_PIC}"
	@echo "${BASESUBDIST} SHARED= ${SHARED}"
	@echo "${BASESUBDIST} CC_INC= ${CC_INC}"
	@echo "${BASESUBDIST} LD_LIB= ${LD_LIB}"
	@echo "${BASESUBDIST} CFLAGS= ${CFLAGS}"
	@echo "${BASESUBDIST} CLINK= ${CLINK}"
	@echo "${BASESUBDIST} LDFLAGS= ${LDFLAGS}"
	@echo "${BASESUBDIST} LSUF= ${LSUF}"
	@echo "${BASESUBDIST} LDIR= ${LDIR}"

# How to modify things as they are installed on the local system
#
# Modify scripts and source, prior to compiling or installing,
# according to this Makefile's varablies.
#
install.sed: Makefile
	@echo building $@
	@(echo 's:^LAVARND_VIDEODEV=.*:LAVARND_VIDEODEV=${LAVARND_VIDEODEV}:'; \
	  echo 's:^LAVARND_USER=.*:LAVARND_USER=${LAVARND_USER}:'; \
	  echo 's:^LAVARND_GROUP=.*:LAVARND_GROUP=${LAVARND_GROUP}:'; \
	  echo 's:^LAVAPOOL_CHROOT=.*:LAVAPOOL_CHROOT=${LAVAPOOL_CHROOT}:'; \
	) > $@
	@echo $@ built

# How to modify things as they are distributed for source code release
#
# Modify scripts and source, prior to forming the source to be distributed,
# to use a canonical set of paths and values.
#
dist.sed: Makefile
	@echo building $@
	@${RM} -f $@
	@(echo 's:^LAVARND_VIDEODEV=.*:LAVARND_VIDEODEV=/dev/video0:'; \
	  echo 's:^LAVARND_USER=.*:LAVARND_USER=lava:'; \
	  echo 's:^LAVARND_GROUP=.*:LAVARND_GROUP=lava:'; \
	  echo 's:^LAVAPOOL_CHROOT=.*:LAVAPOOL_CHROOT=/:'; \
	) > $@
	@echo $@ built

# what and how we make source available via the web
#
dist: clobber dist.sed COPYING COPYING-LGPL
	@echo "=+= starting $@ for ${BASESUBDIST} =+=+="
	@echo form ${DIST} tree with dist.sed modified files for ${BASESUBDIST}
	${RM} -rf ${DIST}
	${MKDIR} -p ${DIST}/${SUBDIST}
	${CHMOD} 0755 ${DIST}/${SUBDIST}
	${CP} -p COPYING COPYING-LGPL ${DIST}/${SUBDIST}
	-@for i in ${CSRC} ${HSRC} Makefile; do \
	    echo "${SED} -f dist.sed < $$i > ${DIST}/${SUBDIST}/$$i"; \
	    ${SED} -f dist.sed < "$$i" > "${DIST}/${SUBDIST}/$$i"; \
	    echo "${CHMOD} 0644 ${DIST}/${SUBDIST}/$$i"; \
	    ${CHMOD} 0644 ${DIST}/${SUBDIST}/$$i; \
	done
	-@for i in ${SHSRC} Makefile; do \
	    echo "${SED} -f dist.sed < $$i > ${DIST}/${SUBDIST}/$$i"; \
	    ${SED} -f dist.sed < "$$i" > "${DIST}/${SUBDIST}/$$i"; \
	    echo "${CHMOD} 0644 ${DIST}/${SUBDIST}/$$i"; \
	    ${CHMOD} 0755 ${DIST}/${SUBDIST}/$$i; \
	done
	@echo set distribution permissions under ${DIST}
	${FIND} ${DIST} -type d | ${XARGS} ${CHMOD} 0775
	${FIND} ${DIST} ! -type d | ${XARGS} ${CHMOD} ugo-w,ugo+r
	${FIND} ${DIST} -name Makefile -o -name '*.cfg' -o -name 'cfg.*' | \
	    ${XARGS} ${CHMOD} 0666
	@echo list files being distributed under ${DISTDIR}/${BASESUBDIST}.list
	${RM} -f ${DISTDIR}/${BASESUBDIST}.list
	cd ${DIST}; ${FIND} . ! -type d | ${SED} -e 's:^\./LavaRnd/::' | \
	   ${SORT} > ${DISTDIR}/${BASESUBDIST}.list
	${CHMOD} 0444 ${DISTDIR}/${BASESUBDIST}.list
	@echo create the tarball ${TARBALL}
	cd ${DIST}; ${RM} -f ${TARBALL}
	cd ${DIST}; ${TAR} -zcvf ${TARBALL} ${SUBDIST}
	-@if [ ! -d "${DISTDIR}" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DISTDIR}"; \
	    ${MKDIR} -p "${DISTDIR}"; \
	    echo "${CHMOD} 0775 ${DISTDIR}"; \
	    ${CHMOD} 0775 "${DISTDIR}"; \
	fi
	-@for i in ${TARBALL}; do \
	    ${RM} -f ${DISTDIR}/$$i.new; \
	    ${CP} -p ${DIST}/$$i ${DISTDIR}/$$i.new; \
	    ${CHMOD} 0444 "${DISTDIR}/$$i.new"; \
	    ${MV} -f ${DISTDIR}/$$i.new ${DISTDIR}/$$i; \
	    echo "distributed ${DISTDIR}/$$i"; \
	done
	${RM} -rf ${DIST}
	@echo "=+= ending $@ for ${BASESUBDIST} =+=+="

# dependency setup
#
depend: ${BUILT_SRC} ${SRC}
	if [ -f Makefile.bak ]; then \
	    echo "Makefile.bak exists, remove or move it out of the way"; \
	    exit 1; \
	else \
	    true; \
	fi
	cd ../lib; $(MAKE) all
	echo forming skel
	-${RM} -rf skel
	${MKDIR} skel skel/${BASESUBDIST}
	${CP} ${SRC} skel/${BASESUBDIST}
	${LN} -s ../${LDIR} skel/lib
	-${RM} -f skel/${BASESUBDIST}/makedep.out
	echo skel formed
	echo forming dependency list
	echo "# DO NOT DELETE THIS LINE -- make depend depends on it." > \
	    skel/${BASESUBDIST}/makedep.out
	cd skel/${BASESUBDIST}; \
	    ${MAKEDEPEND} -w 1 -f makedep.out ${CC_INC} ${CSRC}
	-for i in ${CSRC}; do \
		echo "$$i" | ${SED} 's/^\(.*\)\.c/\1.o: \1.c/'; \
	done >> skel/${BASESUBDIST}/makedep.out
	echo dependency list formed
	echo forming new Makefile
	-${RM} -f Makefile.bak
	${MV} Makefile Makefile.bak
	${SED} -n '1,/^# DO NOT DELETE THIS LINE/p' Makefile.bak | \
		${SED} '$d' > Makefile
	${TAIL} +2 skel/${BASESUBDIST}/makedep.out | \
	    ${SED} -e 's;: /usr/include/LavaRnd/;: ../lib/LavaRnd/;' | \
	    ${EGREP} -v '/usr/include|/usr/lib/gcc-lib|/usr/src/|/boot/' | \
	    LANG=C ${SORT} -u >> Makefile
	-${RM} -rf skel
	-if ${CMP} -s Makefile.bak Makefile; then \
		echo 'Makefile was already up to date'; \
		${MV} -f Makefile.bak Makefile; \
	else \
		echo 'new Makefile formed'; \
	fi

# DO NOT DELETE THIS LINE - make depend needs it

baseconv.o: ../lib/LavaRnd/lavaerr.h
baseconv.o: ../lib/LavaRnd/rawio.h
baseconv.o: baseconv.c
camdump.o: ../lib/LavaRnd/have/cam_videodev.h
camdump.o: ../lib/LavaRnd/have/ov511_cam.h
camdump.o: ../lib/LavaRnd/have/pwc_cam.h
camdump.o: ../lib/LavaRnd/lava_debug.h
camdump.o: ../lib/LavaRnd/lavacam.h
camdump.o: ../lib/LavaRnd/lavaerr.h
camdump.o: ../lib/LavaRnd/lavaquality.h
camdump.o: ../lib/LavaRnd/ov511_drvr.h
camdump.o: ../lib/LavaRnd/ov511_state.h
camdump.o: ../lib/LavaRnd/pwc_drvr.h
camdump.o: ../lib/LavaRnd/pwc_state.h
camdump.o: ../lib/LavaRnd/rawio.h
camdump.o: camdump.c
camdumpdir.o: ../lib/LavaRnd/have/cam_videodev.h
camdumpdir.o: ../lib/LavaRnd/have/ov511_cam.h
camdumpdir.o: ../lib/LavaRnd/have/pwc_cam.h
camdumpdir.o: ../lib/LavaRnd/lava_debug.h
camdumpdir.o: ../lib/LavaRnd/lavacam.h
camdumpdir.o: ../lib/LavaRnd/lavaerr.h
camdumpdir.o: ../lib/LavaRnd/lavaquality.h
camdumpdir.o: ../lib/LavaRnd/ov511_drvr.h
camdumpdir.o: ../lib/LavaRnd/ov511_state.h
camdumpdir.o: ../lib/LavaRnd/pwc_drvr.h
camdumpdir.o: ../lib/LavaRnd/pwc_state.h
camdumpdir.o: ../lib/LavaRnd/rawio.h
camdumpdir.o: camdumpdir.c
camget.o: ../lib/LavaRnd/have/cam_videodev.h
camget.o: ../lib/LavaRnd/have/ov511_cam.h
camget.o: ../lib/LavaRnd/have/pwc_cam.h
camget.o: ../lib/LavaRnd/lava_debug.h
camget.o: ../lib/LavaRnd/lavacam.h
camget.o: ../lib/LavaRnd/lavaerr.h
camget.o: ../lib/LavaRnd/lavaquality.h
camget.o: ../lib/LavaRnd/ov511_drvr.h
camget.o: ../lib/LavaRnd/ov511_state.h
camget.o: ../lib/LavaRnd/pwc_drvr.h
camget.o: ../lib/LavaRnd/pwc_state.h
camget.o: camget.c
camsanity.o: ../lib/LavaRnd/cleanup.h
camsanity.o: ../lib/LavaRnd/have/cam_videodev.h
camsanity.o: ../lib/LavaRnd/have/ov511_cam.h
camsanity.o: ../lib/LavaRnd/have/pwc_cam.h
camsanity.o: ../lib/LavaRnd/lava_debug.h
camsanity.o: ../lib/LavaRnd/lavacam.h
camsanity.o: ../lib/LavaRnd/lavaerr.h
camsanity.o: ../lib/LavaRnd/lavaquality.h
camsanity.o: ../lib/LavaRnd/ov511_drvr.h
camsanity.o: ../lib/LavaRnd/ov511_state.h
camsanity.o: ../lib/LavaRnd/pwc_drvr.h
camsanity.o: ../lib/LavaRnd/pwc_state.h
camsanity.o: ../lib/LavaRnd/rawio.h
camsanity.o: camsanity.c
camset.o: ../lib/LavaRnd/have/cam_videodev.h
camset.o: ../lib/LavaRnd/have/ov511_cam.h
camset.o: ../lib/LavaRnd/have/pwc_cam.h
camset.o: ../lib/LavaRnd/lava_debug.h
camset.o: ../lib/LavaRnd/lavacam.h
camset.o: ../lib/LavaRnd/lavaerr.h
camset.o: ../lib/LavaRnd/lavaquality.h
camset.o: ../lib/LavaRnd/ov511_drvr.h
camset.o: ../lib/LavaRnd/ov511_state.h
camset.o: ../lib/LavaRnd/pwc_drvr.h
camset.o: ../lib/LavaRnd/pwc_state.h
camset.o: camset.c
chk_lavarnd.o: ../lib/LavaRnd/have/have_getcontext.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_getpgrp.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_getppid.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_getprid.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_getpriority.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_getrlimit.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_gettime.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_rusage.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_sbrk.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_statfs.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_sys_resource.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_sys_time.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_sys_times.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_time.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_uid_t.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_ustat.h
chk_lavarnd.o: ../lib/LavaRnd/have/have_ustat_h.h
chk_lavarnd.o: ../lib/LavaRnd/lavarnd.h
chk_lavarnd.o: ../lib/LavaRnd/sha1.h
chk_lavarnd.o: ../lib/LavaRnd/sysstuff.h
chk_lavarnd.o: chk_lavarnd.c
imgtally.o: ../lib/LavaRnd/cleanup.h
imgtally.o: ../lib/LavaRnd/have/cam_videodev.h
imgtally.o: ../lib/LavaRnd/have/ov511_cam.h
imgtally.o: ../lib/LavaRnd/have/pwc_cam.h
imgtally.o: ../lib/LavaRnd/lava_debug.h
imgtally.o: ../lib/LavaRnd/lavacam.h
imgtally.o: ../lib/LavaRnd/lavaerr.h
imgtally.o: ../lib/LavaRnd/lavaquality.h
imgtally.o: ../lib/LavaRnd/ov511_drvr.h
imgtally.o: ../lib/LavaRnd/ov511_state.h
imgtally.o: ../lib/LavaRnd/pwc_drvr.h
imgtally.o: ../lib/LavaRnd/pwc_state.h
imgtally.o: ../lib/LavaRnd/rawio.h
imgtally.o: chi_tbl.h
imgtally.o: imgtally.c
lavadump.o: ../lib/LavaRnd/cleanup.h
lavadump.o: ../lib/LavaRnd/have/cam_videodev.h
lavadump.o: ../lib/LavaRnd/have/ov511_cam.h
lavadump.o: ../lib/LavaRnd/have/pwc_cam.h
lavadump.o: ../lib/LavaRnd/lava_debug.h
lavadump.o: ../lib/LavaRnd/lavacam.h
lavadump.o: ../lib/LavaRnd/lavaerr.h
lavadump.o: ../lib/LavaRnd/lavaquality.h
lavadump.o: ../lib/LavaRnd/lavarnd.h
lavadump.o: ../lib/LavaRnd/ov511_drvr.h
lavadump.o: ../lib/LavaRnd/ov511_state.h
lavadump.o: ../lib/LavaRnd/pwc_drvr.h
lavadump.o: ../lib/LavaRnd/pwc_state.h
lavadump.o: ../lib/LavaRnd/rawio.h
lavadump.o: ../lib/LavaRnd/sha1.h
lavadump.o: lavadump.c
lavaop.o: ../lib/LavaRnd/cfg.h
lavaop.o: ../lib/LavaRnd/cleanup.h
lavaop.o: ../lib/LavaRnd/fetchlava.h
lavaop.o: ../lib/LavaRnd/lava_callback.h
lavaop.o: ../lib/LavaRnd/lava_debug.h
lavaop.o: ../lib/LavaRnd/lavaerr.h
lavaop.o: ../lib/LavaRnd/lavaquality.h
lavaop.o: ../lib/LavaRnd/rawio.h
lavaop.o: lavaop.c
lavaop_i.o: ../lib/LavaRnd/cfg.h
lavaop_i.o: ../lib/LavaRnd/cleanup.h
lavaop_i.o: ../lib/LavaRnd/fetchlava.h
lavaop_i.o: ../lib/LavaRnd/lava_callback.h
lavaop_i.o: ../lib/LavaRnd/lava_debug.h
lavaop_i.o: ../lib/LavaRnd/lavaerr.h
lavaop_i.o: ../lib/LavaRnd/lavaquality.h
lavaop_i.o: ../lib/LavaRnd/rawio.h
lavaop_i.o: lavaop_i.c
poolout.o: ../lib/LavaRnd/cleanup.h
poolout.o: ../lib/LavaRnd/lava_callback.h
poolout.o: ../lib/LavaRnd/lava_debug.h
poolout.o: ../lib/LavaRnd/lavaerr.h
poolout.o: ../lib/LavaRnd/lavaquality.h
poolout.o: ../lib/LavaRnd/random.h
poolout.o: ../lib/LavaRnd/random_libc.h
poolout.o: poolout.c
ppmhead.o: ppmhead.c
tryrnd.o: ../lib/LavaRnd/cleanup.h
tryrnd.o: ../lib/LavaRnd/lava_callback.h
tryrnd.o: ../lib/LavaRnd/lava_debug.h
tryrnd.o: ../lib/LavaRnd/lavaerr.h
tryrnd.o: ../lib/LavaRnd/lavaquality.h
tryrnd.o: ../lib/LavaRnd/random.h
tryrnd.o: ../lib/LavaRnd/random_libc.h
tryrnd.o: tryrnd.c
y2grey.o: y2grey.c
y2yuv.o: y2yuv.c
y2pseudoyuv.o: y2pseudoyuv.c
yuv2ppm.o: yuv2ppm.c
yuv2ppm.o: yuv2rgb.h
yuv2rgb.o: yuv2rgb.c
yuv2rgb.o: yuv2rgb.h
