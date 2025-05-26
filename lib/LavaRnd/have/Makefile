#!/bin/make
#
# lib/have - determine what special system calls and include files we have
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
CC= cc
CHMOD= chmod
CHOWN= chown
CI= ci
CMP = cmp
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
BASESUBDIST= lib/LavaRnd/have
SUBDIST= LavaRnd/${BASESUBDIST}
DISTDIR= /var/tmp/LavaRnd-src
TARBALL= lib.tar.gz

#########################################
#=-=-= end of common Makefile vars =-=-=#
#########################################

# src and .o files
#
CSRC= endian.c \
	have_getcontext.c have_getdtablesize.c have_gethostid.c \
	have_getppid.c have_getprid.c have_getrlimit.c \
	have_gettime.c have_rusage.c have_sbrk.c \
	have_statfs.c have_uid_t.c have_ustat.c \
	have_getpriority.c have_getpgrp.c have_pselect.c

# intermediate files that are made/built
#
OBJS= endian.o \
	have_getcontext.o have_getdtablesize.o have_gethostid.o \
	have_getppid.o have_getprid.o have_getrlimit.o \
	have_gettime.o have_rusage.o have_sbrk.o \
	have_statfs.o have_uid_t.o have_ustat.o \
	have_getpriority.o have_getpgrp.o have_pselect.o

HAVE_PROG= endian \
	have_getcontext have_getdtablesize have_gethostid \
	have_getppid have_getprid have_getrlimit \
	have_gettime have_rusage have_sbrk \
	have_statfs have_uid_t have_ustat \
	have_getpriority have_getpgrp have_pselect

BUILT_HSRC= endian.h pwc_cam.h cam_videodev.h ov511_cam.h \
	have_getppid.h have_getprid.h have_gettime.h \
	have_rusage.h have_sys_resource.h have_sys_time.h \
	have_sys_times.h have_time.h have_uid_t.h \
	have_ustat.h have_ustat_h.h have_sbrk.h have_getrlimit.h \
	have_statfs.h have_getcontext.h have_getdtablesize.h \
	have_gethostid.h have_getpriority.h have_getpgrp.h have_pselect.h

SRC= ${CSRC} ${BUILT_HSRC}

# C compiler control
#
CC_PIC=
#CC_PIC= -fPIC

#SHARED= -shared
#SHARED= -static
SHARED=

CC_INC= -I.

#LSUF= .a
LSUF= .so

#LDIR= ../lib
LDIR= ../lib/shared

#LD_LIB= -L${LDIR}
LD_LIB= -Wl,-rpath=${DESTLIB} -L${LDIR}

CFLAGS= ${LAVA_DEBUG} ${DMALLOC_CC} ${CC_WARN} ${CC_DBG} ${CC_PIC} ${SHARED} ${CC_INC}
CLINK= ${CC_DBG} ${LD_LIB} ${DMALLOC_LIB} ${SHARED}
LDFLAGS= ${CC_OPT} ${DMALLOC_LIB} ${SHARED}

# what to build or have
#
TARGETS= ${BUILT_HSRC}

# where to install
#
DIR= lib/have

#####################################
#=-=-= start of Makefile rules =-=-=#
#####################################

all: ${BUILT_HSRC}

# constructed sources
#

endian.h: Makefile
	@echo "forming $@"
	@rm -f endian.h endian
	@${CC} endian.c -o endian
	@./endian > endian.h
	@rm -f endian
	@echo "$@ formed"

pwc_cam.h: pwc-ioctl-8.6.h Makefile
	@echo "forming $@"
	@rm -f $@
	@echo "/*" > $@
	@echo " * pwc-cam - where to find pwc-ioctl.h" >> $@
	@echo " *" >> $@
	@echo " * Generated by Makefile, do not edit!" >> $@
	@echo " */" >> $@
	@echo "" >> $@
	@echo "#if !defined(__PWC_CAM_H__)" >> $@
	@echo "#define __PWC_CAM_H__" >> $@
	@echo "" >> $@
	-@if [ -f /usr/src/linux-2.4/drivers/usb/pwc-ioctl.h ]; then \
	    echo "#include \"/usr/src/linux-2.4/drivers/usb/pwc-ioctl.h\"" ; \
	elif [ -f /usr/src/linux/drivers/usb/pwc-ioctl.h ]; then \
	    echo "#include \"/usr/src/linux/drivers/usb/pwc-ioctl.h\"" ; \
	else \
	    echo "#include \"pwc-ioctl-8.6.h\"" ; \
	fi >> $@
	@echo "" >> $@
	@echo "#endif /* __PWC_CAM_H__ */" >> $@
	@echo "$@ formed"

ov511_cam.h: Makefile cam_videodev.h
	@echo "forming $@"
	@rm -f $@
	@echo "/*" > $@
	@echo " * ov511-cam - where to find ov511-ioctl.h" >> $@
	@echo " *" >> $@
	@echo " * Generated by Makefile, do not edit!" >> $@
	@echo " */" >> $@
	@echo "" >> $@
	@echo "#if !defined(__OV511_CAM_H__)" >> $@
	@echo "#define __OV511_CAM_H__" >> $@
	@echo "" >> $@
	@echo "#include \"cam_videodev.h\"" >> $@
	@echo "" >> $@
	@echo "#endif /* __OV511_CAM_H__ */" >> $@
	@echo "$@ formed"

cam_videodev.h: videodev_2.4.h Makefile
	@echo "forming $@"
	@rm -f $@
	@echo "/*" > $@
	@echo " * cam_videodev.h - where to find videodev.h" >> $@
	@echo " *" >> $@
	@echo " * Generated by Makefile, do not edit!" >> $@
	@echo " */" >> $@
	@echo "" >> $@
	@echo "#if !defined(__CAM_VIDEODEV_H__)" >> $@
	@echo "#define __CAM_VIDEODEV_H__" >> $@
	@echo "" >> $@
	-@if [ -f /usr/include/linux/videodev2.h ]; then \
	    echo "#include \"/usr/include/linux/videodev2.h\"" ; \
	else \
	    echo "#include \"videodev_2.4.h\"" ; \
	fi >> $@
	@echo "" >> $@
	@echo "#endif /* __CAM_VIDEODEV_H__ */" >> $@
	@echo "$@ formed"

have_time.h: Makefile
	@rm -f $@.tmp
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if [ -f /usr/include/time.h ]; then \
	    echo '#define HAVE_TIME_H	/* we have <time.h> */'; \
	else \
	    echo '#undef HAVE_TIME_H	/* do not have <time.h> */'; \
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi

have_sys_time.h: Makefile
	@rm -f $@.tmp
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if [ -f /usr/include/sys/time.h ]; then \
	    echo '#define HAVE_SYS_TIME_H /* we have <sys/time.h> */'; \
	else \
	    echo '#undef HAVE_SYS_TIME_H /* dont have <sys/time.h> */'; \
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi

have_sys_times.h: Makefile
	@rm -f $@.tmp
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if [ -f /usr/include/sys/times.h ]; then \
	    echo '#define HAVE_SYS_TIMES_H /* we have <sys/times.h> */'; \
	else \
	    echo '#undef HAVE_SYS_TIMES_H /* dont have <sys/times.h> */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi

have_sys_resource.h: Makefile
	@rm -f $@.tmp
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if [ -f /usr/include/sys/resource.h ]; then \
	    echo '#define HAVE_SYS_RESOURCE_H /* we have <sys/resource.h> */'; \
	else \
	    echo '#undef HAVE_SYS_RESOURCE_H /* dont have <sys/resource.h> */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi

have_ustat_h.h: Makefile
	@rm -f $@.tmp
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if [ -f /usr/include/ustat.h ]; then \
	    echo '#define HAVE_USTAT_H /* we have <ustat.h> */'; \
	else \
	    echo '#undef HAVE_USTAT_H /* do not have <ustat.h> */'; \
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi

have_gettime.h: Makefile have_gettime.c
	@rm -f $@.tmp have_gettime.o have_gettime
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_gettime.c \
			     -o have_gettime >/dev/null 2>&1; then \
	    echo '#define HAVE_GETTIME /* we have gettime() POSIX clock */'; \
	else \
	    echo '#undef HAVE_GETTIME /* dont have gettime() POSIX clock */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_gettime.o have_gettime

have_getprid.h: Makefile have_getprid.c
	@rm -f $@.tmp have_getprid.o have_getprid
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_getprid.c \
			     -o have_getprid >/dev/null 2>&1; then \
	    echo '#define HAVE_GETPRID /* we have getprid() call */'; \
	else \
	    echo '#undef HAVE_GETPRID /* dont have getprid() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_getprid.o have_getprid

have_getppid.h: Makefile have_getppid.c
	@rm -f $@.tmp have_getppid.o have_getppid
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_getppid.c \
			     -o have_getppid >/dev/null 2>&1; then \
	    echo '#define HAVE_GETPPID /* we have getppid() call */'; \
	else \
	    echo '#undef HAVE_GETPPID /* dont have getppid() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_getppid.o have_getppid

have_sbrk.h: Makefile have_sbrk.c
	@rm -f $@.tmp have_sbrk.o have_sbrk
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_sbrk.c \
			     -o have_sbrk >/dev/null 2>&1; then \
	    echo '#define HAVE_SBRK /* we have sbrk() call */'; \
	else \
	    echo '#undef HAVE_SBRK /* dont have sbrk() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_sbrk.o have_sbrk

have_getrlimit.h: Makefile have_getrlimit.c have_sys_time.h \
		       have_sys_resource.h
	@rm -f $@.tmp have_getrlimit.o have_getrlimit
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_getrlimit.c \
			     -o have_getrlimit >/dev/null 2>&1; then \
	    echo '#define HAVE_GETRLIMIT /* we have getrlimit() */'; \
	else \
	    echo '#undef HAVE_GETRLIMIT /* dont have getrlimit() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_getrlimit.o have_getrlimit

have_statfs.h: Makefile have_statfs.c
	@rm -f $@.tmp have_statfs.o have_statfs
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_statfs.c \
			     -o have_statfs >/dev/null 2>&1; then \
	    echo '#define HAVE_STATFS /* we have statfs() call */'; \
	else \
	    echo '#undef HAVE_STATFS /* dont have statfs() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_statfs.o have_statfs

have_getcontext.h: Makefile have_getcontext.c
	@rm -f $@.tmp have_getcontext.o have_getcontext
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_getcontext.c \
			     -o have_getcontext >/dev/null 2>&1; then \
	    echo '#define HAVE_GETCONTEXT /* we have getcontext() */'; \
	else \
	    echo '#undef HAVE_GETCONTEXT /* dont have getcontext() */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_getcontext.o have_getcontext

have_getdtablesize.h: Makefile have_getdtablesize.c
	@rm -f $@.tmp have_getdtablesize.o have_getdtablesize
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_getdtablesize.c \
			     -o have_getdtablesize >/dev/null 2>&1; then \
	    echo '#define HAVE_GETDTABLESIZE /* we have getdtablesize() call */'; \
	else \
	    echo '#undef HAVE_GETDTABLESIZE /* dont have getdtablesize() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_getdtablesize.o have_getdtablesize

have_gethostid.h: Makefile have_gethostid.c
	@rm -f $@.tmp have_gethostid.o have_gethostid
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_gethostid.c \
			     -o have_gethostid >/dev/null 2>&1; then \
	    echo '#define HAVE_GETHOSTID /* we have gethostid() call */'; \
	else \
	    echo '#undef HAVE_GETHOSTID /* dont have gethostid() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_gethostid.o have_gethostid

have_getpriority.h: Makefile have_getpriority.c
	@rm -f $@.tmp have_getpriority.o have_getpriority
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_getpriority.c \
			     -o have_getpriority >/dev/null 2>&1; then \
	    echo '#define HAVE_GETPRIORITY /* we have getpriority() call */'; \
	else \
	    echo '#undef HAVE_GETPRIORITY /* dont have getpriority() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_getpriority.o have_getpriority

have_uid_t.h: Makefile have_uid_t.c
	@rm -f $@.tmp have_uid_t.o have_uid_t
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_uid_t.c \
			     -o have_uid_t >/dev/null 2>&1; then \
	    echo '#define HAVE_UID_T /* we have uid_t type */'; \
	else \
	    echo '#undef HAVE_UID_T /* dont have uid_t type */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_uid_t.o have_uid_t

have_ustat.h: Makefile have_ustat.c have_ustat_h.h
	@rm -f $@.tmp have_ustat.o have_ustat
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_ustat.c \
			     -o have_ustat >/dev/null 2>&1; then \
	    echo '#define HAVE_USTAT /* we have ustat() call */'; \
	else \
	    echo '#undef HAVE_USTAT /* dont have ustat() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_ustat.o have_ustat

have_rusage.h: Makefile have_rusage.c
	@rm -f $@.tmp have_rusage.o have_rusage
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_rusage.c \
			     -o have_rusage >/dev/null 2>&1; then \
	    echo '#define HAVE_GETRUSAGE /* we have getrusage() call */'; \
	else \
	    echo '#undef HAVE_GETRUSAGE /* dont have getrusage() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_rusage.o have_rusage

have_getpgrp.h: Makefile have_getpgrp.c
	@rm -f $@.tmp have_getpgrp.o have_getpgrp
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_getpgrp.c \
			     -o have_getpgrp >/dev/null 2>&1; then \
	    echo '#define HAVE_GETPGRP /* we have getpgrp() call */'; \
	else \
	    echo '#undef HAVE_GETPGRP /* dont have getpgrp() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_getpgrp.o have_getpgrp

have_pselect.h: Makefile have_pselect.c
	@rm -f $@.tmp have_pselect.o have_pselect
	@echo '/* Do not edit - auto generated by Makefile */' > $@.tmp
	-@if ${CC} ${CFLAGS} have_pselect.c \
			     -o have_pselect >/dev/null 2>&1; then \
	    echo '#define HAVE_PSELECT /* we have pselect() call */'; \
	else \
	    echo '#undef HAVE_PSELECT /* dont have pselect() call */';\
	fi >> $@.tmp
	-@if ! cmp -s $@ $@.tmp; then \
	    mv -f $@.tmp $@; \
	    echo 'formed $@'; \
	else \
	    rm -f $@.tmp; \
	fi
	@rm -f have_pselect.o have_pselect

# utility rules
#
tags: hsrc Makefile
	ctags *.c *.h

clean:
	rm -f ${OBJS}
	-@for i in ${BUILT_HSRC}; do \
	    if [ -f "$$i.tmp" ]; then \
		echo "rm -f $$i.tmp"; \
		rm -f "$$i.tmp"; \
	    fi; \
	    if [ -f "$$i" ]; then \
		echo "rm -f $$i"; \
		rm -f "$$i"; \
	    fi; \
	done

clobber: clean
	rm -f ${HAVE_PROG} tags dist skel

install: all
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
	-@if [ ! -d "${DESTINC}/LavaRnd/have" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DESTINC}/LavaRnd/have"; \
	    ${MKDIR} -p "${DESTINC}/LavaRnd/have"; \
	    echo "${CHMOD} 0755 ${DESTINC}/LavaRnd/have"; \
	    ${CHMOD} 0755 "${DESTINC}/LavaRnd/have"; \
	    echo "${CHOWN} root.root ${DESTINC}/LavaRnd/have"; \
	    ${CHOWN} root.root "${DESTINC}/LavaRnd/have"; \
	fi
	-@for i in ${BUILT_HSRC}; do \
	    if ! ${CMP} -s "$$i" "${DESTINC}/LavaRnd/have/$$i"; then \
		${RM} -f "${DESTINC}/LavaRnd/have/$$i.new"; \
		${CP} -f "$$i.tmp" "${DESTINC}/LavaRnd/have/$$i.new"; \
		${CHMOD} 0444 "${DESTINC}/LavaRnd/have/$$i.new"; \
		${MV} -f "${DESTINC}/LavaRnd/have/$$i.new" \
			 "${DESTINC}/LavaRnd/have/$$i"; \
		echo "$$i installed into ${DESTINC}/LavaRnd/have/$$i"; \
	    fi; \
	done

# dependency setup
#
depend: ${SRC}
	if [ -f Makefile.bak ]; then \
		echo "Makefile.bak exists, remove or move it out of the way"; \
		exit 1; \
	else \
	    true; \
	fi
	echo forming skel
	-rm -rf skel
	mkdir -p skel skel/${DIR}
	cp ${SRC} skel/${DIR}
	-rm -f skel/${DIR}/makedep.out
	echo skel formed
	echo forming dependency list
	echo "# DO NOT DELETE THIS LINE -- make depend depends on it." > \
	    skel/${DIR}/makedep.out
	cd skel/${DIR}; \
	    ${MAKEDEPEND} -w 1 -f makedep.out ${CSRC}
	-for i in ${CSRC}; do \
		echo "$$i" | ${SED} 's/^\(.*\)\.c/\1.o: \1.c/'; \
	done >> skel/${DIR}/makedep.out
	echo dependency list formed
	echo forming new Makefile
	-rm -f Makefile.bak
	mv Makefile Makefile.bak
	${SED} -n '1,/^# DO NOT DELETE THIS LINE/p' Makefile.bak | \
		${SED} '$d' > Makefile
	${TAIL} +2 skel/${DIR}/makedep.out | \
	    ${EGREP} -v '/usr/include|/usr/lib/gcc-lib|/usr/src/|/boot/' | \
	    LANG=C ${SORT} -u >> Makefile
	-rm -rf skel
	-if cmp -s Makefile.bak Makefile; then \
		echo 'Makefile was already up to date'; \
		mv -f Makefile.bak Makefile; \
	else \
		echo 'new Makefile formed'; \
	fi

# DO NOT DELETE THIS LINE - make depend needs it

endian.o: endian.c
have_getcontext.o: have_getcontext.c
have_getdtablesize.o: have_getdtablesize.c
have_gethostid.o: have_gethostid.c
have_getpgrp.o: have_getpgrp.c
have_getppid.o: have_getppid.c
have_getprid.o: have_getprid.c
have_getpriority.o: have_getpriority.c
have_getpriority.o: have_sys_resource.h
have_getpriority.o: have_sys_time.h
have_getrlimit.o: have_getrlimit.c
have_getrlimit.o: have_sys_resource.h
have_getrlimit.o: have_sys_time.h
have_gettime.o: have_gettime.c
have_gettime.o: have_time.h
have_pselect.o: have_pselect.c
have_pselect.o: have_sys_time.h
have_rusage.o: have_rusage.c
have_sbrk.o: have_sbrk.c
have_statfs.o: have_statfs.c
have_uid_t.o: have_uid_t.c
have_ustat.o: have_ustat.c
have_ustat.o: have_ustat_h.h
