#!/bin/make
#
# lib - LavaRnd library
#
# @(#) $Revision: 10.1 $
# @(#) $Id: Makefile,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

# How to compile C programs
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
DESTLIB=/usr/lib
DESTINC=/usr/include/LavaRnd
DOCDIR= /usr/share/doc
TOOLDIR= ${DESTBIN}/lavatool
RC_DIR= /etc/rc.d/init.d
CFGDIR=/etc/LavaRnd

# user and host information
#
# LAVARND_VIDEODEV	special device file for the LavaRnd webcam
# LAVARND_CAMTYPE	camera type (use a name given by: tool/camget list all)
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
BASESUBDIST= lib
SUBDIST= LavaRnd/${BASESUBDIST}
DISTDIR= /var/tmp/LavaRnd-src
TARBALL= lib.tar.gz

#########################################
#=-=-= end of common Makefile vars =-=-=#
#########################################

# src and .o files
#
CSRC= fetchlava.c fnv1.c \
	lavarnd.c lavasocket.c \
	liblava_exit.c liblava_retry.c liblava_return.c liblava_s100_any.c \
	liblava_s100_high.c liblava_s100_med.c liblava_try_any.c \
	liblava_try_high.c liblava_try_med.c liblava_tryonce_any.c \
	liblava_tryonce_high.c liblava_tryonce_med.c random.c random_libc.c \
	rawio.c s100.c sha1.c sysstuff.c lava_debug.c camop.c \
	pwc_drvr.c ov511_drvr.c \
	liblava_invalid.c cleanup.c palette.c

HAVE_HFILE= LavaRnd/have/cam_videodev.h LavaRnd/have/endian.h \
	LavaRnd/have/have_getcontext.h LavaRnd/have/have_getdtablesize.h \
	LavaRnd/have/have_gethostid.h LavaRnd/have/have_getpgrp.h \
	LavaRnd/have/have_getppid.h LavaRnd/have/have_getprid.h \
	LavaRnd/have/have_getpriority.h LavaRnd/have/have_getrlimit.h \
	LavaRnd/have/have_gettime.h LavaRnd/have/have_pselect.h \
	LavaRnd/have/have_rusage.h LavaRnd/have/have_sbrk.h \
	LavaRnd/have/have_statfs.h LavaRnd/have/have_sys_resource.h \
	LavaRnd/have/have_sys_time.h LavaRnd/have/have_sys_times.h \
	LavaRnd/have/have_time.h LavaRnd/have/have_uid_t.h \
	LavaRnd/have/have_ustat.h LavaRnd/have/have_ustat_h.h \
	LavaRnd/have/pwc_cam.h LavaRnd/have/ov511_cam.h

HAVE_SRC= LavaRnd/have/endian.c LavaRnd/have/have_getcontext.c \
	LavaRnd/have/have_getdtablesize.c LavaRnd/have/have_gethostid.c \
	LavaRnd/have/have_getpgrp.c LavaRnd/have/have_getppid.c \
	LavaRnd/have/have_getprid.c LavaRnd/have/have_getpriority.c \
	LavaRnd/have/have_getrlimit.c LavaRnd/have/have_gettime.c \
	LavaRnd/have/have_pselect.c LavaRnd/have/have_rusage.c \
	LavaRnd/have/have_sbrk.c LavaRnd/have/have_statfs.c \
	LavaRnd/have/have_uid_t.c LavaRnd/have/have_ustat.c \
	LavaRnd/have/pwc-ioctl-8.6.h LavaRnd/have/videodev_2.4.h \
	LavaRnd/have/Makefile

HSRC= LavaRnd/cfg.h LavaRnd/fetchlava.h LavaRnd/fnv1.h \
	LavaRnd/lava_callback.h LavaRnd/lavaerr.h LavaRnd/lavaquality.h \
	LavaRnd/lavarnd.h LavaRnd/random.h LavaRnd/random_libc.h \
	LavaRnd/rawio.h LavaRnd/s100.h LavaRnd/s100_internal.h LavaRnd/sha1.h \
	LavaRnd/sha1_internal.h LavaRnd/sysstuff.h LavaRnd/lava_debug.h \
	LavaRnd/lavacam.h \
	LavaRnd/pwc_drvr.h LavaRnd/pwc_state.h \
	LavaRnd/ov511_drvr.h LavaRnd/ov511_state.h \
	LavaRnd/cleanup.h

# intermediate files that are made/built
#
OBJS= fetchlava.o fnv1.o lavarnd.o lavasocket.o liblava_exit.o \
	liblava_retry.o liblava_return.o liblava_s100_any.o \
	liblava_s100_high.o liblava_s100_med.o liblava_try_any.o \
	liblava_try_high.o liblava_try_med.o liblava_tryonce_any.o \
	liblava_tryonce_high.o liblava_tryonce_med.o random.o random_libc.o \
	rawio.o s100.o sha1.o sysstuff.o lava_debug.o camop.o \
	pwc_drvr.o ov511_drvr.o \
	liblava_invalid.o cleanup.o palette.o

COMMON_LAVA_OBS= fetchlava.o fnv1.o lavasocket.o random.o rawio.o s100.o \
		sha1.o sysstuff.o lava_debug.o cleanup.o

# what to install
#
LIBFILES= libLavaRnd_util${LSUF} libLavaRnd_raw${LSUF} liblava_libc${LSUF} \
	liblava_exit${LSUF} liblava_retry${LSUF} liblava_return${LSUF} \
	liblava_try_high${LSUF} liblava_try_med${LSUF} liblava_try_any${LSUF} \
	liblava_tryonce_high${LSUF} liblava_tryonce_med${LSUF} \
	liblava_tryonce_any${LSUF} liblava_s100_high${LSUF} \
	liblava_s100_med${LSUF} liblava_s100_any${LSUF} libLavaRnd_cam${LSUF}

# what to build or have
#
CFG_TARGETS= cfg.random-def
TARGETS= ${LIBFILES} ${CFG_TARGETS} Makefile shared/Makefile

# C compiler control
#
CC_PIC=
#CC_PIC= -fPIC

#SHARED= -static
SHARED=

CC_INC= -I.

LSUF= .a

LDIR= .

LD_LIB= -L${LDIR}

CFLAGS= ${LAVA_DEBUG} ${DMALLOC_CC} ${CC_WARN} ${CC_DBG} ${CC_PIC} ${SHARED} ${CC_INC}
CLINK= ${CC_DBG} ${LD_LIB} ${DMALLOC_LIB} ${SHARED}
LDFLAGS= ${CC_OPT} ${DMALLOC_LIB} ${SHARED}

# passdown - complete list of Makefile vars passed down to lib/shared/Makefile
# and the lib/LavaRnd/have/Makefile.
#
# NOTE: We do not want to override ${BASESUBDIST}, ${SUBDIST}, or ${TARBALL}
#	because each sub-dir needs to use its own value to create
#	its own tarball.
#
PASSDOWN= \
    SHELL='${SHELL}' \
    AWK='${AWK}' \
    BASENAME='${BASENAME}' \
    CAT='${CAT}' \
    CC='${CC}' \
    CHMOD='${CHMOD}' \
    CHOWN='${CHOWN}' \
    CI='${CI}' \
    CMP='${CMP}' \
    CO='${CO}' \
    CP='${CP}' \
    CTAGS='${CTAGS}' \
    DATE='${DATE}' \
    DIRNAME='${DIRNAME}' \
    EGREP='${EGREP}' \
    FGREP='${FGREP}' \
    FIND='${FIND}' \
    GROUPADD='${GROUPADD}' \
    HEAD='${HEAD}' \
    INSTALL='${INSTALL}' \
    LDD='${LDD}' \
    LN='${LN}' \
    MAKE='${MAKE}' \
    MAKEDEPEND='${MAKEDEPEND}' \
    MKDIR='${MKDIR}' \
    MV='${MV}' \
    PERL='${PERL}' \
    POD2MAN='${POD2MAN}' \
    RM='${RM}' \
    RSYNC='${RSYNC}' \
    SED='${SED}' \
    SORT='${SORT}' \
    TAIL='${TAIL}' \
    TAR='${TAR}' \
    USERADD='${USERADD}' \
    XARGS='${XARGS}' \
    LAVA_DEBUG='${LAVA_DEBUG}' \
    DMALLOC_CC='${DMALLOC_CC}' \
    DMALLOC_LIB='${DMALLOC_LIB}' \
    CC_WARN='${CC_WARN}' \
    CC_OPT='${CC_OPT}' \
    CC_DBG='${CC_DBG}' \
    DESTBIN='${DESTBIN}' \
    DESTSBIN='${DESTSBIN}' \
    DESTLIB='${DESTLIB}' \
    DESTINC='${DESTINC}' \
    DOCDIR='${DOCDIR}' \
    TOOLDIR='${TOOLDIR}' \
    RC_DIR='${RC_DIR}' \
    CFGDIR='${CFGDIR}' \
    LAVARND_VIDEODEV='${LAVARND_VIDEODEV}' \
    LAVARND_USER='${LAVARND_USER}' \
    LAVARND_GROUP='${LAVARND_GROUP}' \
    LAVAPOOL_CHROOT='${LAVAPOOL_CHROOT}' \
    DIST='${DIST}' \
    DISTDIR='${DISTDIR}'

# Force the modes on directories by setting FORCE to a non-empty string
#
FORCE=

#####################################
#=-=-= start of Makefile rules =-=-=#
#####################################

all: ${TARGETS} install.sed dist.sed COPYING COPYING-LGPL
	@echo "=+= starting $@ rule for lib/shared =+="
	@cd shared; $(MAKE) $@ ${PASSDOWN}
	@echo "=+= ending $@ rule for lib/shared =+="

have: ${HAVE_HFILE}

${HAVE_HFILE}:
	@echo "=+= starting all rule for lib/LavaRnd/have =+="
	@cd LavaRnd/have; $(MAKE) all ${PASSDOWN}
	@echo "=+= ending all rule for lib/LavaRnd/have =+="

# building dynamic libraries
#
libLavaRnd_util${LSUF}: ${COMMON_LAVA_OBS} lavarnd.o liblava_invalid.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

libLavaRnd_raw${LSUF}: s100.o lava_debug.o fetchlava.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_libc${LSUF}: random_libc.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_exit${LSUF}: ${COMMON_LAVA_OBS} liblava_exit.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_retry${LSUF}: ${COMMON_LAVA_OBS} liblava_retry.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_return${LSUF}: ${COMMON_LAVA_OBS} liblava_return.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_try_high${LSUF}: ${COMMON_LAVA_OBS} liblava_try_high.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_try_med${LSUF}: ${COMMON_LAVA_OBS} liblava_try_med.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_try_any${LSUF}: ${COMMON_LAVA_OBS} liblava_try_any.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_tryonce_high${LSUF}: ${COMMON_LAVA_OBS} liblava_tryonce_high.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_tryonce_med${LSUF}: ${COMMON_LAVA_OBS} liblava_tryonce_med.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_tryonce_any${LSUF}: ${COMMON_LAVA_OBS} liblava_tryonce_any.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_s100_high${LSUF}: ${COMMON_LAVA_OBS} liblava_s100_high.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_s100_med${LSUF}: ${COMMON_LAVA_OBS} liblava_s100_med.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

liblava_s100_any${LSUF}: ${COMMON_LAVA_OBS} liblava_s100_any.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

libLavaRnd_cam${LSUF}: camop.o palette.o pwc_drvr.o ov511_drvr.o
	@echo "creating $@"
	@${RM} -f $@
	@${AR} -cvsr $@ $^

# utility rules
#
tags: ${CSRC} ${HAVE_HFILE} ${HSRC} Makefile
	${CTAGS} ${CSRC} ${HAVE_HFILE} ${HSRC}

clean:
	@echo "=+= starting $@ rule =+="
	${RM} -f ${OBJS}
	${RM} -rf dist skel
	${RM} -f install.sed dist.sed
	@echo "= starting $@ rule for lib/shared ="
	@cd shared; $(MAKE) $@ ${PASSDOWN}
	@echo "= ending $@ rule for lib/shared ="
	@echo "= starting $@ rule for lib/LavaRnd/have ="
	@cd LavaRnd/have; $(MAKE) $@ ${PASSDOWN}
	@echo "= ending $@ rule for lib/LavaRnd/have ="
	@echo "=+= ending $@ rule =+="

clobber: clean
	@echo "=+= starting $@ rule =+="
	${RM} -f ${LIBFILES} tags
	@echo "= starting $@ rule for lib/shared ="
	@cd shared; $(MAKE) $@ ${PASSDOWN}
	@echo "= ending $@ rule for lib/shared ="
	@echo "= starting $@ rule for lib/LavaRnd/have ="
	@cd LavaRnd/have; $(MAKE) $@ ${PASSDOWN}
	@echo "= ending $@ rule for lib/LavaRnd/have ="
	@echo "=+= ending $@ rule =+="

install: all
	@echo "=+= starting $@ rule =+="
	-@if [ ! -d "${DESTLIB}" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DESTLIB}"; \
	    ${MKDIR} -p "${DESTLIB}"; \
	    echo "${CHMOD} 0755 ${DESTLIB}"; \
	    ${CHMOD} 0755 "${DESTLIB}"; \
	    echo "${CHOWN} root.root ${DESTLIB}"; \
	    ${CHOWN} root.root "${DESTLIB}"; \
	fi
	-@for i in ${LIBFILES} ""; do \
	    if [ "$$i" = "" ]; then break; fi; \
	    if ! ${CMP} -s "$$i" "${DESTLIB}/$$i"; then \
		${RM} -f ${DESTLIB}/$$i.new; \
		${CP} $$i ${DESTLIB}/$$i.new; \
		${CHMOD} 0444 ${DESTLIB}/$$i.new; \
		${MV} -f ${DESTLIB}/$$i.new ${DESTLIB}/$$i; \
		echo "$$i installed into ${DESTLIB}/$$i"; \
	    fi; \
	done
	-@if [ ! -d "${DESTINC}" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DESTINC}"; \
	    ${MKDIR} -p "${DESTINC}"; \
	    echo "${CHMOD} 0755 ${DESTINC}"; \
	    ${CHMOD} 0755 "${DESTINC}"; \
	    echo "${CHOWN} root.root ${DESTINC}"; \
	    ${CHOWN} root.root "${DESTINC}"; \
	fi
	-@if [ ! -d "${DESTINC}/LavaRnd" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DESTINC}/LavaRnd"; \
	    ${MKDIR} -p "${DESTINC}/LavaRnd"; \
	    echo "${CHMOD} 0755 ${DESTINC}/LavaRnd"; \
	    ${CHMOD} 0755 "${DESTINC}/LavaRnd"; \
	    echo "${CHOWN} root.root ${DESTINC}/LavaRnd"; \
	    ${CHOWN} root.root "${DESTINC}/LavaRnd"; \
	fi
	-@for i in ${HSRC} ""; do \
	    if [ "$$i" = "" ]; then break; fi; \
	    if ! ${CMP} -s "$$i" "${DESTINC}/$$i"; then \
		${RM} -f ${DESTINC}/$$i.new; \
		${CP} $$i ${DESTINC}/$$i.new; \
		${CHMOD} 0444 ${DESTINC}/$$i.new; \
		${MV} -f ${DESTINC}/$$i.new ${DESTINC}/$$i; \
		echo "$$i installed into ${DESTINC}/$$i"; \
	    fi; \
	done
	-@if [ ! -d "${DESTINC}/LavaRnd/have" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DESTINC}/LavaRnd/have"; \
	    ${MKDIR} -p "${DESTINC}/LavaRnd/have"; \
	    echo "${CHMOD} 0755 ${DESTINC}/LavaRnd/have"; \
	    ${CHMOD} 0755 "${DESTINC}/LavaRnd/have"; \
	    echo "${CHOWN} root.root ${DESTINC}/LavaRnd/have"; \
	    ${CHOWN} root.root "${DESTINC}/LavaRnd/have"; \
	fi
	-@for i in ${HAVE_HFILE} ""; do \
	    if [ "$$i" = "" ]; then break; fi; \
	    if ! ${CMP} -s "$$i" "${DESTINC}/$$i"; then \
		${RM} -f ${DESTINC}/$$i.new; \
		${CP} $$i ${DESTINC}/$$i.new; \
		${CHMOD} 0444 ${DESTINC}/$$i.new; \
		${MV} -f ${DESTINC}/$$i.new ${DESTINC}/$$i; \
		echo "$$i installed into ${DESTINC}/$$i"; \
	    fi; \
	done
	-@if [ ! -d "${CFGDIR}" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${CFGDIR}"; \
	    ${MKDIR} -p "${CFGDIR}"; \
	    echo "${CHMOD} 0755 ${CFGDIR}"; \
	    ${CHMOD} 0755 "${CFGDIR}"; \
	    echo "${CHOWN} root.root ${CFGDIR}"; \
	    ${CHOWN} root.root "${CFGDIR}"; \
	fi
	-@for i in ${CFG_TARGETS}; do \
	    if ! ${CMP} -s "$$i" "${CFGDIR}/$$i"; then \
		if [ -f "${CFGDIR}/$$i" ]; then \
		    ${RM} -f ${CFGDIR}/$$i.new; \
		    ${CP} $$i ${CFGDIR}/$$i.new; \
		    ${CHMOD} 0444 ${CFGDIR}/$$i.new; \
		    echo "config file ${CFGDIR}/$$i exists"; \
		    echo "placed new version into ${CFGDIR}/$$i.new"; \
		else \
		    ${RM} -f ${CFGDIR}/$$i.new; \
		    if [ -f "${CFGDIR}/RCS/$$i,v" ]; then \
			echo "${CO} -l ${CFGDIR}/$$i"; \
			${CO} -l ${CFGDIR}/$$i; \
		    fi; \
		    ${CP} $$i ${CFGDIR}/$$i.new; \
		    ${MV} -f ${CFGDIR}/$$i.new ${CFGDIR}/$$i; \
		    if [ -f "${CFGDIR}/RCS/$$i,v" ]; then \
			echo "${CI} -u -m'make install' ${CFGDIR}/$$i"; \
			${CI} -u -m'make install' ${CFGDIR}/$$i; \
		    fi; \
		    echo "$$i installed into ${CFGDIR}/$$i"; \
		fi; \
	    fi; \
	done
	@echo "= starting $@ rule for lib/shared ="
	@cd shared; $(MAKE) $@ ${PASSDOWN}
	@echo "= starting $@ rule for lib/shared ="
	@echo "=+= ending $@ rule =+="

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
	  echo 's:^DESTINC=.*:DESTINC=${DESTINC}:'; \
	  echo 's:^DESTLIB=.*:DESTLIB=${DESTLIB}:'; \
	  echo 's:^CFGDIR=.*:CFGDIR=${CFGDIR}:'; \
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
	  echo 's:^DESTINC=.*:DESTINC=/usr/include/LavaRnd:'; \
	  echo 's:^DESTLIB=.*:DESTLIB=/usr/lib:'; \
	  echo 's:^CFGDIR=.*:CFGDIR=/etc/LavaRnd:'; \
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
	${MKDIR} -p ${DIST}/${SUBDIST}/LavaRnd
	${CHMOD} 0755 ${DIST}/${SUBDIST}/LavaRnd
	${MKDIR} -p ${DIST}/${SUBDIST}/LavaRnd/have
	${CHMOD} 0755 ${DIST}/${SUBDIST}/LavaRnd/have
	${MKDIR} -p ${DIST}/${SUBDIST}/shared
	${CHMOD} 0755 ${DIST}/${SUBDIST}/shared
	${CP} -p COPYING COPYING-LGPL ${DIST}/${SUBDIST}
	-@for i in ${CSRC} ${HAVE_SRC} ${HSRC} ${CFG_TARGETS} \
		   Makefile shared/Makefile; do \
	    echo "${SED} -f dist.sed < $$i > ${DIST}/${SUBDIST}/$$i"; \
	    ${SED} -f dist.sed < "$$i" > "${DIST}/${SUBDIST}/$$i"; \
	    echo "${CHMOD} 0644 ${DIST}/${SUBDIST}/$$i"; \
	    ${CHMOD} 0644 ${DIST}/${SUBDIST}/$$i; \
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
depend: ${HAVE_HFILE} ${HSRC} ${CSRC}
	if [ -f Makefile.bak ]; then \
	    echo "Makefile.bak exists, remove or move it out of the way"; \
	    exit 1; \
	else \
	    true; \
	fi
	echo forming skel
	-${RM} -rf skel
	${MKDIR} skel skel/${BASESUBDIST} skel/${BASESUBDIST}/LavaRnd
	${CP} ${CSRC} skel/${BASESUBDIST}
	${CP} ${HSRC} skel/${BASESUBDIST}/LavaRnd
	${MKDIR} skel/${BASESUBDIST}/LavaRnd/have
	${CP} ${HAVE_HFILE} skel/${BASESUBDIST}/LavaRnd/have
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

camop.o: LavaRnd/have/cam_videodev.h
camop.o: LavaRnd/have/ov511_cam.h
camop.o: LavaRnd/have/pwc_cam.h
camop.o: LavaRnd/lavacam.h
camop.o: LavaRnd/lavaerr.h
camop.o: LavaRnd/ov511_drvr.h
camop.o: LavaRnd/ov511_state.h
camop.o: LavaRnd/pwc_drvr.h
camop.o: LavaRnd/pwc_state.h
camop.o: camop.c
cleanup.o: LavaRnd/fetchlava.h
cleanup.o: LavaRnd/lava_callback.h
cleanup.o: LavaRnd/lavaquality.h
cleanup.o: LavaRnd/rawio.h
cleanup.o: cleanup.c
fetchlava.o: LavaRnd/cfg.h
fetchlava.o: LavaRnd/fetchlava.h
fetchlava.o: LavaRnd/lava_callback.h
fetchlava.o: LavaRnd/lava_debug.h
fetchlava.o: LavaRnd/lavaerr.h
fetchlava.o: LavaRnd/lavaquality.h
fetchlava.o: LavaRnd/rawio.h
fetchlava.o: LavaRnd/s100.h
fetchlava.o: LavaRnd/sha1.h
fetchlava.o: fetchlava.c
fnv1.o: LavaRnd/fnv1.h
fnv1.o: fnv1.c
lava_debug.o: LavaRnd/have/have_sys_time.h
lava_debug.o: LavaRnd/lava_callback.h
lava_debug.o: LavaRnd/lava_debug.h
lava_debug.o: LavaRnd/lavaerr.h
lava_debug.o: LavaRnd/lavaquality.h
lava_debug.o: LavaRnd/rawio.h
lava_debug.o: lava_debug.c
lavarnd.o: LavaRnd/have/have_getcontext.h
lavarnd.o: LavaRnd/have/have_getpgrp.h
lavarnd.o: LavaRnd/have/have_getppid.h
lavarnd.o: LavaRnd/have/have_getprid.h
lavarnd.o: LavaRnd/have/have_getpriority.h
lavarnd.o: LavaRnd/have/have_getrlimit.h
lavarnd.o: LavaRnd/have/have_gettime.h
lavarnd.o: LavaRnd/have/have_rusage.h
lavarnd.o: LavaRnd/have/have_sbrk.h
lavarnd.o: LavaRnd/have/have_statfs.h
lavarnd.o: LavaRnd/have/have_sys_resource.h
lavarnd.o: LavaRnd/have/have_sys_time.h
lavarnd.o: LavaRnd/have/have_sys_times.h
lavarnd.o: LavaRnd/have/have_time.h
lavarnd.o: LavaRnd/have/have_uid_t.h
lavarnd.o: LavaRnd/have/have_ustat.h
lavarnd.o: LavaRnd/have/have_ustat_h.h
lavarnd.o: LavaRnd/lavaerr.h
lavarnd.o: LavaRnd/lavarnd.h
lavarnd.o: LavaRnd/sha1.h
lavarnd.o: LavaRnd/sysstuff.h
lavarnd.o: lavarnd.c
lavasocket.o: LavaRnd/lavaerr.h
lavasocket.o: LavaRnd/rawio.h
lavasocket.o: lavasocket.c
liblava_exit.o: LavaRnd/lava_callback.h
liblava_exit.o: LavaRnd/lavaquality.h
liblava_exit.o: liblava_exit.c
liblava_invalid.o: LavaRnd/lava_callback.h
liblava_invalid.o: LavaRnd/lavaquality.h
liblava_invalid.o: liblava_invalid.c
liblava_retry.o: LavaRnd/lava_callback.h
liblava_retry.o: LavaRnd/lavaquality.h
liblava_retry.o: liblava_retry.c
liblava_return.o: LavaRnd/lava_callback.h
liblava_return.o: LavaRnd/lavaquality.h
liblava_return.o: liblava_return.c
liblava_s100_any.o: LavaRnd/lava_callback.h
liblava_s100_any.o: LavaRnd/lavaquality.h
liblava_s100_any.o: liblava_s100_any.c
liblava_s100_high.o: LavaRnd/lava_callback.h
liblava_s100_high.o: LavaRnd/lavaquality.h
liblava_s100_high.o: liblava_s100_high.c
liblava_s100_med.o: LavaRnd/lava_callback.h
liblava_s100_med.o: LavaRnd/lavaquality.h
liblava_s100_med.o: liblava_s100_med.c
liblava_try_any.o: LavaRnd/lava_callback.h
liblava_try_any.o: LavaRnd/lavaquality.h
liblava_try_any.o: liblava_try_any.c
liblava_try_high.o: LavaRnd/lava_callback.h
liblava_try_high.o: LavaRnd/lavaquality.h
liblava_try_high.o: liblava_try_high.c
liblava_try_med.o: LavaRnd/lava_callback.h
liblava_try_med.o: LavaRnd/lavaquality.h
liblava_try_med.o: liblava_try_med.c
liblava_tryonce_any.o: LavaRnd/lava_callback.h
liblava_tryonce_any.o: LavaRnd/lavaquality.h
liblava_tryonce_any.o: liblava_tryonce_any.c
liblava_tryonce_high.o: LavaRnd/lava_callback.h
liblava_tryonce_high.o: LavaRnd/lavaquality.h
liblava_tryonce_high.o: liblava_tryonce_high.c
liblava_tryonce_med.o: LavaRnd/lava_callback.h
liblava_tryonce_med.o: LavaRnd/lavaquality.h
liblava_tryonce_med.o: liblava_tryonce_med.c
ov511_drvr.o: LavaRnd/have/cam_videodev.h
ov511_drvr.o: LavaRnd/have/have_pselect.h
ov511_drvr.o: LavaRnd/have/ov511_cam.h
ov511_drvr.o: LavaRnd/have/pwc_cam.h
ov511_drvr.o: LavaRnd/lavacam.h
ov511_drvr.o: LavaRnd/lavaerr.h
ov511_drvr.o: LavaRnd/ov511_drvr.h
ov511_drvr.o: LavaRnd/ov511_state.h
ov511_drvr.o: LavaRnd/pwc_drvr.h
ov511_drvr.o: LavaRnd/pwc_state.h
ov511_drvr.o: LavaRnd/rawio.h
ov511_drvr.o: ov511_drvr.c
palette.o: LavaRnd/have/cam_videodev.h
palette.o: LavaRnd/have/ov511_cam.h
palette.o: LavaRnd/have/pwc_cam.h
palette.o: LavaRnd/lavacam.h
palette.o: LavaRnd/lavaerr.h
palette.o: LavaRnd/ov511_drvr.h
palette.o: LavaRnd/ov511_state.h
palette.o: LavaRnd/pwc_drvr.h
palette.o: LavaRnd/pwc_state.h
palette.o: palette.c
pwc_drvr.o: LavaRnd/have/cam_videodev.h
pwc_drvr.o: LavaRnd/have/have_pselect.h
pwc_drvr.o: LavaRnd/have/ov511_cam.h
pwc_drvr.o: LavaRnd/have/pwc_cam.h
pwc_drvr.o: LavaRnd/lavacam.h
pwc_drvr.o: LavaRnd/lavaerr.h
pwc_drvr.o: LavaRnd/ov511_drvr.h
pwc_drvr.o: LavaRnd/ov511_state.h
pwc_drvr.o: LavaRnd/pwc_drvr.h
pwc_drvr.o: LavaRnd/pwc_state.h
pwc_drvr.o: LavaRnd/rawio.h
pwc_drvr.o: pwc_drvr.c
random.o: LavaRnd/fetchlava.h
random.o: LavaRnd/lava_callback.h
random.o: LavaRnd/lava_debug.h
random.o: LavaRnd/lavaerr.h
random.o: LavaRnd/lavaquality.h
random.o: LavaRnd/random.h
random.o: random.c
random_libc.o: LavaRnd/fetchlava.h
random_libc.o: LavaRnd/lava_callback.h
random_libc.o: LavaRnd/lavaerr.h
random_libc.o: LavaRnd/lavaquality.h
random_libc.o: LavaRnd/random.h
random_libc.o: LavaRnd/random_libc.h
random_libc.o: random_libc.c
rawio.o: LavaRnd/have/have_pselect.h
rawio.o: LavaRnd/have/have_sys_time.h
rawio.o: LavaRnd/lavaerr.h
rawio.o: LavaRnd/rawio.h
rawio.o: rawio.c
s100.o: LavaRnd/fnv1.h
s100.o: LavaRnd/have/have_getcontext.h
s100.o: LavaRnd/have/have_getpgrp.h
s100.o: LavaRnd/have/have_getppid.h
s100.o: LavaRnd/have/have_getprid.h
s100.o: LavaRnd/have/have_getpriority.h
s100.o: LavaRnd/have/have_getrlimit.h
s100.o: LavaRnd/have/have_gettime.h
s100.o: LavaRnd/have/have_rusage.h
s100.o: LavaRnd/have/have_sbrk.h
s100.o: LavaRnd/have/have_statfs.h
s100.o: LavaRnd/have/have_sys_resource.h
s100.o: LavaRnd/have/have_sys_time.h
s100.o: LavaRnd/have/have_sys_times.h
s100.o: LavaRnd/have/have_time.h
s100.o: LavaRnd/have/have_uid_t.h
s100.o: LavaRnd/have/have_ustat.h
s100.o: LavaRnd/have/have_ustat_h.h
s100.o: LavaRnd/lavaerr.h
s100.o: LavaRnd/lavaquality.h
s100.o: LavaRnd/s100.h
s100.o: LavaRnd/s100_internal.h
s100.o: LavaRnd/sha1.h
s100.o: LavaRnd/sysstuff.h
s100.o: s100.c
sha1.o: LavaRnd/have/endian.h
sha1.o: LavaRnd/sha1.h
sha1.o: LavaRnd/sha1_internal.h
sha1.o: sha1.c
sysstuff.o: LavaRnd/fnv1.h
sysstuff.o: LavaRnd/have/have_getcontext.h
sysstuff.o: LavaRnd/have/have_getpgrp.h
sysstuff.o: LavaRnd/have/have_getppid.h
sysstuff.o: LavaRnd/have/have_getprid.h
sysstuff.o: LavaRnd/have/have_getpriority.h
sysstuff.o: LavaRnd/have/have_getrlimit.h
sysstuff.o: LavaRnd/have/have_gettime.h
sysstuff.o: LavaRnd/have/have_rusage.h
sysstuff.o: LavaRnd/have/have_sbrk.h
sysstuff.o: LavaRnd/have/have_statfs.h
sysstuff.o: LavaRnd/have/have_sys_resource.h
sysstuff.o: LavaRnd/have/have_sys_time.h
sysstuff.o: LavaRnd/have/have_sys_times.h
sysstuff.o: LavaRnd/have/have_time.h
sysstuff.o: LavaRnd/have/have_uid_t.h
sysstuff.o: LavaRnd/have/have_ustat.h
sysstuff.o: LavaRnd/have/have_ustat_h.h
sysstuff.o: LavaRnd/sha1.h
sysstuff.o: LavaRnd/sysstuff.h
sysstuff.o: sysstuff.c
