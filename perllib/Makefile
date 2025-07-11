#!/usr/bin/make
#
# perllib - Perl interface to the LavaRnd library
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

# utilities
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
DESTLIB= /usr/lib
DESTINC= /usr/include
DOCDIR= /usr/share/doc
TOOLDIR= ${DESTBIN}/lavatool
RC_DIR= /etc/rc.d/init.d
CFGDIR= /etc/LavaRnd

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
BASESUBDIST= perllib
SUBDIST= LavaRnd/${BASESUBDIST}
DISTDIR= /var/tmp/LavaRnd-src
TARBALL= perllib.tar.gz

#########################################
#=-=-= end of common Makefile vars =-=-=#
#########################################

# Dir under which module sub-directores reside
#
MTOP= LavaRnd

# Names of directories under the ${MTOP} sub-directory
#
MODNAME_SET= Exit Retry Return S100_Any TryOnce_Any Try_Any Util
#MODNAME_SET= TryOnce_Any

# LavaRnd Libs needed
#
LAVA_LIBS= ${LDIR}/liblava_exit${LSUF} ${LDIR}/liblava_retry${LSUF} \
	${LDIR}/liblava_return${LSUF} ${LDIR}/liblava_s100_any${LSUF} \
	${LDIR}/liblava_tryonce_any${LSUF} ${LDIR}/liblava_try_any${LSUF} \
	${LDIR}/libLavaRnd_util${LSUF}

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

all: ${LAVA_LIBS} ${MODNAME_SET} install.sed dist.sed COPYING COPYING-LGPL

${MODNAME_SET}:
	@if [ ! -s ${MTOP}/$@/Makefile ]; then \
	    echo "making ${MTOP}/$@/Makefile =-="; \
	    INCDIR="`cd ../lib; /bin/pwd`"; \
	    LIBDIR="`cd ${LDIR}; /bin/pwd`"; \
	    name="`echo $@ | tr '[A-Z]' '[a-z]'`"; \
	    if [ ! -f "${DESTLIB}/liblava_$$name${LSUF}" -a \
	    	 ! -f "${DESTLIB}/lib${MTOP}_$$name${LSUF}" ]; then \
	        echo "Warning: -llava_$$name not in ${DESTLIB}" 1>&2; \
	    fi; \
	    cd ${MTOP}/$@; \
	    if [ -f "$$LIBDIR/liblava_$$name${LSUF}" ]; then \
		echo ${PERL} Makefile.PL \
		  LIBS="-L${DESTLIB} -llava_"$$name INC="-I$$INCDIR"; \
		${PERL} Makefile.PL \
		  LIBS="-L${DESTLIB} -llava_"$$name INC="-I$$INCDIR"; \
	    else \
		echo ${PERL} Makefile.PL \
		  LIBS="-L${DESTLIB} -l${MTOP}_"$$name INC="-I$$INCDIR"; \
		${PERL} Makefile.PL \
		  LIBS="-L${DESTLIB} -l${MTOP}_"$$name INC="-I$$INCDIR"; \
	    fi; \
	fi
	cd ${MTOP}/$@; ${MAKE} all

# rules to make required things from other directories
#
${LDIR}/liblava_exit${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_exit${LSUF}

${LDIR}/liblava_retry${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_retry${LSUF}

${LDIR}/liblava_return${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_return${LSUF}

${LDIR}/liblava_s100_any${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_s100_any${LSUF}

${LDIR}/liblava_tryonce_any${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_tryonce_any${LSUF}

${LDIR}/liblava_try_any${LSUF}:
	cd ${LDIR}; $(MAKE) liblava_try_any${LSUF}

${LDIR}/libLavaRnd_util${LSUF}:
	cd ${LDIR}; $(MAKE) libLavaRnd_util${LSUF}

# utility rules
#
test:
	@for i in ${MODNAME_SET}; do \
	    echo "testing ${MTOP}/$$i =-=-=-="; \
	    name="`echo $@ | tr '[A-Z]' '[a-z]'`"; \
	    if [ ! -f "${DESTLIB}/liblava_$$name${LSUF}" -a \
	    	 ! -f "${DESTLIB}/lib${MTOP}_$$name${LSUF}" ]; then \
	        echo "Error: -llava_$$name not installed in ${DESTLIB}"; 1>&2 \
		echo "Cannot test without this library installed first"; 1>&2 \
		exit 1; \
	    fi; \
	    (cd ${MTOP}/$$i; $(MAKE) test); \
	    if [ $$? -ne 0 ]; then \
	        echo "  $(MAKE) $@ for $$i failed" 1>&2; \
		exit 1; \
	    fi; \
	done

clean:
	@for i in ${MODNAME_SET}; do \
	    if [ ! -s ${MTOP}/$$i/Makefile ]; then \
		echo "making ${MTOP}/$$i/Makefile =-="; \
		INCDIR="`cd ../lib; /bin/pwd`"; \
		LIBDIR="`cd ${LDIR}; /bin/pwd`"; \
		name="`echo $$i | tr '[A-Z]' '[a-z]'`"; \
		cd ${MTOP}/$$i; \
		if [ -f "$$LIBDIR/liblava_$$name${LSUF}" ]; then \
		    echo ${PERL} Makefile.PL \
		      LIBS="-L${DESTLIB} -llava_"$$name INC="-I$$INCDIR"; \
		    ${PERL} Makefile.PL \
		      LIBS="-L${DESTLIB} -llava_"$$name INC="-I$$INCDIR"; \
		else \
		    echo ${PERL} Makefile.PL \
		      LIBS="-L${DESTLIB} -l${MTOP}_"$$name INC="-I$$INCDIR"; \
		    ${PERL} Makefile.PL \
		      LIBS="-L${DESTLIB} -l${MTOP}_"$$name INC="-I$$INCDIR"; \
		fi; \
	    fi; \
	    echo "cleaning ${MTOP}/$$i =-=-=-="; \
	    (cd ${MTOP}/$$i; $(MAKE) clean); \
	done
	${RM} -f install.sed dist.sed

clobber:
	@for i in ${MODNAME_SET}; do \
	    if [ ! -s ${MTOP}/$$i/Makefile ]; then \
		echo "making ${MTOP}/$$i/Makefile =-="; \
		INCDIR="`cd ../lib; /bin/pwd`"; \
		LIBDIR="`cd ${LDIR}; /bin/pwd`"; \
		name="`echo $$i | tr '[A-Z]' '[a-z]'`"; \
		cd ${MTOP}/$$i; \
		if [ -f "$$LIBDIR/liblava_$$name${LSUF}" ]; then \
		    echo ${PERL} Makefile.PL \
		      LIBS="-L${DESTLIB} -llava_"$$name INC="-I$$INCDIR"; \
		    ${PERL} Makefile.PL \
		      LIBS="-L${DESTLIB} -llava_"$$name INC="-I$$INCDIR"; \
		else \
		    echo ${PERL} Makefile.PL \
		      LIBS="-L${DESTLIB} -l${MTOP}_"$$name INC="-I$$INCDIR"; \
		    ${PERL} Makefile.PL \
		      LIBS="-L${DESTLIB} -l${MTOP}_"$$name INC="-I$$INCDIR"; \
		fi; \
	    fi; \
	    echo "clobbering ${MTOP}/$$i =-=-=-="; \
	    (cd ${MTOP}/$$i; $(MAKE) realclean); \
	done
	${RM} -f install.sed dist.sed

install: all
	@for i in ${MODNAME_SET}; do \
	    echo "installing ${MTOP}/$$i =-=-=-="; \
	    (cd ${MTOP}/$$i; $(MAKE) install); \
	    if [ $$? -ne 0 ]; then \
	        echo "  $(MAKE) $@ for $$i failed" 1>&2; \
		exit 1; \
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
	@(echo 's:^LAVARND_VIDEODEV=.*:LAVARND_VIDEODEV=/dev/video0:'; \
	  echo 's:^LAVARND_USER=.*:LAVARND_USER=lava:'; \
	  echo 's:^LAVARND_GROUP=.*:LAVARND_GROUP=lava:'; \
	  echo 's:^LAVAPOOL_CHROOT=.*:LAVAPOOL_CHROOT=/:'; \
	) > $@
	@echo $@ built

dist: clobber dist.sed COPYING COPYING-LGPL
	@echo "=+= starting $@ for ${BASESUBDIST} =+=+="
	@echo form ${DIST} tree with dist.sed modified files for ${BASESUBDIST}
	${RM} -rf ${DIST}
	${MKDIR} -p ${DIST}/${SUBDIST}
	${CHMOD} 0755 ${DIST}/${SUBDIST}
	${MKDIR} -p ${DIST}/${SUBDIST}/${MTOP}
	${CHMOD} 0755 ${DIST}/${SUBDIST}/${MTOP}
	${CP} -p COPYING COPYING-LGPL ${DIST}/${SUBDIST}
	@for i in ${MODNAME_SET}; do \
	    echo "preping ${DIST}/${SUBDIST}/${MTOP}/$$i =-=-=-="; \
	    find ${MTOP}/$$i -depth -print | \
	        ${EGREP} -v 'RCS|NOTES' | \
	    	cpio -dumpv ${DIST}/${SUBDIST}; \
	done
	-@for i in Makefile; do \
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
