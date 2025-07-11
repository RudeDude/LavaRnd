#!/usr/bin/make
#
# doc - LavaRnd documentation
#
# @(#) $Revision: 10.4 $
# @(#) $Id: Makefile,v 10.4 2003/08/25 11:14:07 lavarnd Exp $
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
LAVARND_VIDEODEV= /dev/video0
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
BASESUBDIST= doc
SUBDIST= LavaRnd/${BASESUBDIST}
DISTDIR= /var/tmp/LavaRnd-src
TARBALL= doc.tar.gz

#########################################
#=-=-= end of common Makefile vars =-=-=#
#########################################

# Where to install documentation files
#
DOCDEST= ${DOCDIR}/LavaRnd

# what to build
#
DOC= CHANGES TODO BUGS README-compile README-install README-test README-tool \
	README-camera README-config README-src README-API
TARGETS= ${DOC}

# Force the modes on directories by setting FORCE to a non-empty string
#
FORCE=

#####################################
#=-=-= start of Makefile rules =-=-=#
#####################################

all: ${TARGETS}

# utility rules
#
tags: ${BUILT_SRC} Makefile
	@#echo nothing to tag

clean:
	${RM} -f dist.sed install.sed

clobber:
	${RM} -f dist.sed install.sed

install: all ../README-first install.sed
	@if [ ! -d "${DOCDIR}" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DOCDIR}"; \
	    ${MKDIR} -p "${DOCDIR}"; \
	    echo "${CHMOD} 0755 ${DOCDIR}"; \
	    ${CHMOD} 0755 "${DOCDIR}"; \
	    echo "${CHOWN} root.root ${DOCDIR}"; \
	    ${CHOWN} root.root "${DOCDIR}"; \
	fi
	@if [ ! -d "${DOCDEST}" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DOCDEST}"; \
	    ${MKDIR} -p "${DOCDEST}"; \
	    echo "${CHMOD} 0755 ${DOCDEST}"; \
	    ${CHMOD} 0755 "${DOCDEST}"; \
	    echo "${CHOWN} root.root ${DOCDEST}"; \
	    ${CHOWN} root.root "${DOCDEST}"; \
	fi
	-@for i in ${TARGETS}; do \
	    ${RM} -f "${DOCDEST}/$$i.new"; \
	    ${SED} -f install.sed "$$i" > "${DOCDEST}/$$i.new"; \
	    if ! ${CMP} -s "${DOCDEST}/$$i" "${DOCDEST}/$$i.new"; then \
		${CHMOD} 0444 "${DOCDEST}/$$i.new"; \
		${MV} -f "${DOCDEST}/$$i.new" "${DOCDEST}/$$i"; \
		echo "installed ${DOCDEST}/$$i"; \
	    else \
		${RM} -f "${DOCDEST}/$$i.new"; \
	    fi; \
	done
	@${RM} -f ${DOCDEST}/README-first.new
	@${SED} -f install.sed ../README-first > ${DOCDEST}/README-first.new
	@if ! ${CMP} -s ${DOCDEST}/README-first \
		       ${DOCDEST}/README-first.new; then \
	    ${CHMOD} 0444 ${DOCDEST}/README-first.new; \
	    ${MV} -f ${DOCDEST}/README-first.new ${DOCDEST}/README-first; \
	    echo "installed ${DOCDEST}/README-first"; \
	else \
	    ${RM} -f ${DOCDEST}/README-first.new; \
	fi

# ccflags - output C compiler flag values
#
ccflags:
	@# no flags

# How to modify things as they are installed on the local system
#
# Modify scripts and source, prior to compiling or installing,
# according to this Makefile's varablies.
#
install.sed: Makefile ../version
	@echo building $@
	@VERSION="`../version`"; \
	 (echo 's:^LAVARND_USER=.*:LAVARND_USER=${LAVARND_USER}:'; \
	  echo 's:^LAVARND_GROUP=.*:LAVARND_GROUP=${LAVARND_GROUP}:'; \
	  echo 's:^LAVAPOOL_CHROOT=.*:LAVAPOOL_CHROOT=${LAVAPOOL_CHROOT}:'; \
	  echo 's:0.1.2:'$$VERSION':g'; \
	) > $@
	@echo $@ built

# How to modify things as they are distributed for source code release
#
# Modify scripts and source, prior to forming the source to be distributed,
# to use a canonical set of paths and values.
#
dist.sed: Makefile ../version
	@echo building $@
	@${RM} -f $@
	@VERSION="`../version`"; \
	 (echo 's:^LAVARND_USER=.*:LAVARND_USER=lava:'; \
	  echo 's:^LAVARND_GROUP=.*:LAVARND_GROUP=lava:'; \
	  echo 's:^LAVAPOOL_CHROOT=.*:LAVAPOOL_CHROOT=/:'; \
	  echo 's:0.1.2:'$$VERSION':g'; \
	) > $@
	@echo $@ built

# what and how we make source available via the web
#
# NOTE: The ../README-first file is disted by the top level Makefile.
#
dist: clobber dist.sed COPYING COPYING-LGPL
	@echo "=+= starting $@ for ${BASESUBDIST} =+=+="
	@echo form ${DIST} tree with dist.sed modified files for ${BASESUBDIST}
	${RM} -rf ${DIST}
	${MKDIR} -p ${DIST}/${SUBDIST}
	${CHMOD} 0755 ${DIST}/${SUBDIST}
	${CP} -p COPYING COPYING-LGPL ${DIST}/${SUBDIST}
	-@for i in ${TARGETS} Makefile; do \
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
depend:
	@ # no depend
