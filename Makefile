#!/bin/make
#
# LavaRnd - master LavaRnd makefile
#
# @(#) $Revision: 10.3 $
# @(#) $Id: Makefile,v 10.3 2003/08/25 11:13:15 lavarnd Exp $
# @(#) $Source: /home/lavarnd/int/../src/RCS/Makefile,v $
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

# How to compile C programs - partial list
#
# We let the lower level Makefils set ${CC_PIC}, ${SHARED}, ${LSUF},
# ${LDIR}, ${LD_LIB} individually.  Some dirs, such as lib, need to
# do special shared and static lib stuff that we do not want to override.
# The ${CFLAGS}, ${CLINK} and ${LDFLAGS} are composite vars that
# depend on those vars so for the same reason, we do not set them here.
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
BASESUBDIST= .
SUBDIST= LavaRnd/${BASESUBDIST}
DISTDIR= /var/tmp/LavaRnd-src
TARBALL= toplevel.tar.gz

#########################################
#=-=-= end of common Makefile vars =-=-=#
#########################################

# LavaWWW::Reuse locations
#
REUSE_LOCK= ${RWDATA}/reuse_lock
REUSE_CACHE= ${WWWRW}/reuse_cache
REUSE_CFG= ${RODATA}/reuse_cfg
REUSE_DISABLE= ${RODATA}/reuse_disable
REUSE_DOWN= ${WWWRO}/reuse_down

# LAVALIB_DIR	dir of libs to be linked with Perl modules
# LAVAINC_DIR	dir of *.h files to be linked with Perl modules
#

# subdirs
#
# These dirs should be built in this order
#
# NOTE: The perllib really needs to be built after lib has been installed,
#	so we first install ${PRESUBDIRS} and then recompile perllib
#	before installing it.  *sigh*
#
PRESUBDIRS= lib daemon tool doc
SUBDIRS= ${PRESUBDIRS} perllib

# passdown - complete list of Makefile vars passed down to SUBDIRS Makefiles
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
    LAVARND_CAMTYPE='${LAVARND_CAMTYPE}' \
    LAVARND_USER='${LAVARND_USER}' \
    LAVARND_GROUP='${LAVARND_GROUP}' \
    LAVAPOOL_CHROOT='${LAVAPOOL_CHROOT}' \
    DIST='${DIST}' \
    DISTDIR='${DISTDIR}'

# LavaRnd release
#
# NOTE on VERSION:
#	We call the release_dir rule with VERSION overridden by
#	the output of the version command.
#
VERSION= invalid
RELDIR= /var/tmp/LavaRnd-release

# Force the modes on directories by setting FORCE to a non-empty string
#
FORCE=

#####################################
#=-=-= start of Makefile rules =-=-=#
#####################################

# default rules
#
all: install.sed dist.sed README-first COPYING COPYING-LGPL
	@echo "=+=+=+= starting $@ rule =+=+=+="
	@for i in ${SUBDIRS}; do \
	    if [ ! -d "$$i" ]; then \
	    	echo "$$i is not a sub-directory" 1>&2; \
		exit 1; \
	    fi; \
	done
	@for i in ${SUBDIRS}; do \
	    echo "=+=+= starting $$i subdir =+=+="; \
	    echo "	(cd $$i; $(MAKE) $@)"; \
	    (cd $$i; $(MAKE) $@ ${PASSDOWN}); \
	    if [ $$? -ne 0 ]; then \
	        echo "  $(MAKE) $@ for $$i failed" 1>&2; \
		exit 2; \
	    fi; \
	    echo "=-_-= ending $$i subdir =-_-="; \
	done
	@echo "=+=+=+= ending $@ rule =+=+=+="

# utility rules
#
clean:
	@echo "=+=+=+= starting $@ rule =+=+=+="
	@for i in ${SUBDIRS}; do \
	    if [ ! -d "$$i" ]; then \
	    	echo "$$i is not a sub-directory" 1>&2; \
		exit 1; \
	    fi; \
	done
	-@for i in ${SUBDIRS}; do \
	    echo "=+=+= starting $$i subdir =+=+="; \
	    echo "	(cd $$i; $(MAKE) $@)"; \
	    (cd $$i; $(MAKE) $@ ${PASSDOWN}); \
	    echo "=-_-= ending $$i subdir =-_-="; \
	done
	@${RM} -f dist.sed install.sed
	@echo "=+=+=+= ending $@ rule =+=+=+="

clobber:
	@echo "=+=+=+= starting $@ rule =+=+=+="
	@for i in ${SUBDIRS}; do \
	    if [ ! -d "$$i" ]; then \
	    	echo "$$i is not a sub-directory" 1>&2; \
		exit 1; \
	    fi; \
	done
	-@for i in ${SUBDIRS}; do \
	    echo "=+=+= starting $$i subdir =+=+="; \
	    echo "	(cd $$i; $(MAKE) $@)"; \
	    (cd $$i; $(MAKE) $@ ${PASSDOWN}); \
	    echo "=-_-= ending $$i subdir =-_-="; \
	done
	@${RM} -f dist.sed install.sed
	@echo "=+=+=+= ending $@ rule =+=+=+="

# NOTE: The perllib really needs to be built after lib has been installed,
#	so we first install ${PRESUBDIRS} and then recompile perllib
#
install: all
	@echo "=+=+=+= starting $@ rule =+=+=+="
	@for i in ${SUBDIRS}; do \
	    if [ ! -d "$$i" ]; then \
	    	echo "$$i is not a sub-directory" 1>&2; \
		exit 1; \
	    fi; \
	done
	@for i in ${PRESUBDIRS}; do \
	    echo "=+=+= starting $@ $$i subdir =+=+="; \
	    echo "	(cd $$i; $(MAKE) $@)"; \
	    (cd $$i; $(MAKE) $@ ${PASSDOWN}); \
	    if [ $$? -ne 0 ]; then \
	        echo "  $(MAKE) $@ for $$i failed" 1>&2; \
		exit 2; \
	    fi; \
	    echo "=-_-= ending $@ $$i subdir =-_-="; \
	done
	@echo "=+=+= must rebuiild perllib after lib has been installed =+=+="
	@echo "(cd perllib; $(MAKE) clobber)"
	@(cd perllib; $(MAKE) clobber ${PASSDOWN})
	@echo "(cd perllib; $(MAKE) all)"
	@(cd perllib; $(MAKE) all ${PASSDOWN})
	@echo "=+=+= perllib rebuilt =+=+="
	@echo "=+=+= starting $@ perllib subdir =+=+="
	@echo "      (cd perllib; $(MAKE) $@)"
	@(cd perllib; $(MAKE) $@ ${PASSDOWN}); \
	 if [ $$? -ne 0 ]; then \
	     echo "  $(MAKE) $@ for perllib failed" 1>&2; \
	     exit 3; \
	 fi; \
	 echo "=-_-= ending $@ perllib =-_-="
	@echo "=+=+=+= ending $@ rule =+=+=+="

# ccflags - output C compiler flag values
#
# NOTE: We do not use the ${PASSDOWN} variable because we
#	want to know what the lower level Makefiles have
#	for CC related makefile vars.
#
ccflags:
	@echo "=+=+=+= starting $@ rule =+=+=+="
	@for i in ${SUBDIRS}; do \
	    if [ ! -d "$$i" ]; then \
	    	echo "$$i is not a sub-directory" 1>&2; \
		exit 1; \
	    fi; \
	done
	-@for i in ${SUBDIRS}; do \
	    echo "=+=+= starting $$i subdir =+=+="; \
	    echo "	(cd $$i; $(MAKE) $@)"; \
	    (cd $$i; $(MAKE) $@); \
	    echo "=-_-= ending $$i subdir =-_-="; \
	done
	@echo "=+=+=+= ending $@ rule =+=+=+="

# How to modify things as they are installed on the local system
#
# Modify scripts and source, prior to compiling or installing,
# according to this Makefile's varablies.
#
install.sed: Makefile
	@echo "=+=+=+= starting $@ rule =+=+=+="
	@for i in ${SUBDIRS}; do \
	    if [ ! -d "$$i" ]; then \
	    	echo "$$i is not a sub-directory" 1>&2; \
		exit 1; \
	    fi; \
	done
	-@for i in ${SUBDIRS}; do \
	    echo "=+=+= starting $$i subdir =+=+="; \
	    echo "	(cd $$i; $(MAKE) $@)"; \
	    (cd $$i; $(MAKE) $@ ${PASSDOWN}); \
	    echo "=-_-= ending $$i subdir =-_-="; \
	done
	@echo "=+=+=+= ending $@ rule =+=+=+="

# How to modify things as they are distributed for source code release
#
# Modify scripts and source, prior to forming the source to be distributed,
# to use a canonical set of paths and values.
#
dist.sed: Makefile
	@echo "=+=+=+= starting $@ rule for ${SUBDIRS} =+=+=+="
	@for i in ${SUBDIRS}; do \
	    if [ ! -d "$$i" ]; then \
	    	echo "$$i is not a sub-directory" 1>&2; \
		exit 1; \
	    fi; \
	done
	-@for i in ${SUBDIRS}; do \
	    echo "=+=+= starting $$i subdir =+=+="; \
	    echo "	(cd $$i; $(MAKE) $@)"; \
	    (cd $$i; $(MAKE) $@ ${PASSDOWN}); \
	    echo "=-_-= ending $$i subdir =-_-="; \
	done
	@echo "=+=+=+= ending $@ rule =+=+=+="
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
dist: dist.sed COPYING COPYING-LGPL
	@echo "=+=+=+= starting $@ rule for ${SUBDIRS} =+=+=+="
	@for i in ${SUBDIRS}; do \
	    if [ ! -d "$$i" ]; then \
	    	echo "$$i is not a sub-directory" 1>&2; \
		exit 1; \
	    fi; \
	done
	${RM} -rf ${DISTDIR}
	${MKDIR} -p ${DISTDIR}
	@for i in ${SUBDIRS}; do \
	    echo "=+=+= starting $$i subdir =+=+="; \
	    echo "	(cd $$i; $(MAKE) $@)"; \
	    (cd $$i; $(MAKE) $@ ${PASSDOWN}); \
	    if [ $$? -ne 0 ]; then \
	        echo "  $(MAKE) $@ for $$i failed" 1>&2; \
		exit 2; \
	    fi; \
	    echo "=-_-= ending $$i subdir =-_-="; \
	done
	@echo "=+=+= starting . dir =+=+="
	$(MAKE) topdist
	@echo "=-_-= ending . dir =-_-="
	@echo "=+=+=+= ending $@ rule =+=+=+="

topdist: dist.sed README-first version Makefile COPYING COPYING-LGPL
	@echo "=+=+= starting top-level $@ =+=+="
	@echo form ${DIST} tree with dist.sed modified files for ${BASESUBDIST}
	${RM} -rf ${DIST}
	${MKDIR} -p ${DIST}/${SUBDIST}
	${CHMOD} 0755 ${DIST}/${SUBDIST}
	${CP} -p COPYING COPYING-LGPL ${DIST}/${SUBDIST}
	@for i in README-first Makefile; do \
	    echo "${SED} -f dist.sed < $$i > ${DIST}/${SUBDIST}/$$i"; \
	    ${SED} -f dist.sed < "$$i" > "${DIST}/${SUBDIST}/$$i"; \
	    echo "${CHMOD} 0644 ${DIST}/${SUBDIST}/$$i"; \
	    ${CHMOD} 0644 ${DIST}/${SUBDIST}/$$i; \
	done
	@for i in version; do \
	    echo "${SED} -f dist.sed < $$i > ${DIST}/${SUBDIST}/$$i"; \
	    ${SED} -f dist.sed < "$$i" > "${DIST}/${SUBDIST}/$$i"; \
	    echo "${CHMOD} 0755 ${DIST}/${SUBDIST}/$$i"; \
	    ${CHMOD} 0755 ${DIST}/${SUBDIST}/$$i; \
	done
	@echo set distribution permissions under ${DIST}
	${FIND} ${DIST} -type d | ${XARGS} ${CHMOD} 0775
	${FIND} ${DIST} ! -type d | ${XARGS} ${CHMOD} ugo-w,ugo+r
	${FIND} ${DIST} -name Makefile -o -name '*.cfg' -o -name 'cfg.*' | \
	    ${XARGS} ${CHMOD} 0666
	@echo list files being distributed under ${DISTDIR}/top-level.list
	${RM} -f ${DISTDIR}/top-level.list
	cd ${DIST}; ${FIND} . ! -type d | ${SED} -e 's:^\./LavaRnd/::' | \
	   ${SORT} > ${DISTDIR}/top-level.list
	${CHMOD} 0444 ${DISTDIR}/top-level.list
	@echo create the tarball ${TARBALL}
	cd ${DIST}; ${RM} -f ${TARBALL}
	cd ${DIST}; ${TAR} -zcvf ${TARBALL} ${SUBDIST}
	-@if [ ! -d "${DISTDIR}" -o -n "${FORCE}" ]; then \
	    echo "${MKDIR} -p ${DISTDIR}"; \
	    ${MKDIR} -p "${DISTDIR}"; \
	    echo "${CHMOD} 0775 ${DISTDIR}"; \
	    ${CHMOD} 0775 "${DISTDIR}"; \
	fi
	@for i in ${TARBALL}; do \
	    ${RM} -f ${DISTDIR}/$$i.new; \
	    ${CP} -p ${DIST}/$$i ${DISTDIR}/$$i.new; \
	    ${CHMOD} 0444 "${DISTDIR}/$$i.new"; \
	    ${MV} -f ${DISTDIR}/$$i.new ${DISTDIR}/$$i; \
	    echo "distributed ${DISTDIR}/$$i"; \
	done
	${RM} -rf ${DIST}
	@echo "=-_-= ending top-level $@ =-_-="

# form a release
#
release: version
	@echo "=+=+=+= starting $@ rule =+=+=+="
	@if ! ${FGREP} 'LavaRnd version '`./version` doc/CHANGES>/dev/null;then\
	    echo "Version `./version` is not mentioned in doc/CHANGES" 1>&2; \
	    exit 1; \
	fi
	@if [ ! -d ${RELDIR} ]; then \
	    echo "${RELDIR} directory does not exist" 1>&2; \
	    exit 2; \
	fi
	@if [ -f ${RELDIR}/LavaRnd-${VERSION}.tar.gz ]; then \
	    echo "${RELDIR}/LavaRnd-${VERSION}.tar.gz exists" 1>&2; \
	    exit 3; \
	fi
	@echo "=+=+= starting dist for ../src =+=+="
	$(MAKE) dist
	@echo "=-_-= ending dist for ../src =-_-="
	@echo "=+=+= forming src tarball for LavaRnd version ${VERSION} =+=+="
	$(MAKE) release_dir VERSION=`./version` RELDIR=${RELDIR}
	@echo "=-_-= formed src tarball for LavaRnd version ${VERSION} =-_-="
	@echo "=+=+=+= ending $@ rule =+=+=+="

release_dir:
	@echo "=+=+= starting $@ rule for version ${VERSION} =+=+="
	@if [ X"${VERSION}" = X"invalid" ]; then \
	    echo "must call $(MAKE) release_dir VERSION=`./version`" 1>&2; \
	    echo "invalid version" 1>&2; \
	    exit 1; \
	fi
	@if [ ! -d ${RELDIR} ]; then \
	    echo "${RELDIR} directory does not exist" 1>&2; \
	    exit 2; \
	fi
	@if [ -f ${RELDIR}/LavaRnd-${VERSION}.tar.gz ]; then \
	    echo "${RELDIR}/LavaRnd-${VERSION}.tar.gz exists" 1>&2; \
	    exit 3; \
	fi
	@if [ -f ${RELDIR}/manifest-LavaRnd-${VERSION} ]; then \
	    echo "${RELDIR}/manifest-LavaRnd-${VERSION} exists" 1>&2; \
	    exit 4; \
	fi
	${RM} -rf ${RELDIR}/LavaRnd
	${MKDIR} ${RELDIR}/LavaRnd
	${FIND} ${DISTDIR} -name '*.tar.gz' -print | while read i; do \
	    echo "cd ${RELDIR}; ${TAR} -zxf $$i"; \
	    cd ${RELDIR}; ${TAR} -zxf $$i; \
	done
	${RM} -rf ${RELDIR}/LavaRnd-${VERSION}
	${MV} ${RELDIR}/LavaRnd ${RELDIR}/LavaRnd-${VERSION}
	${RM} -f ${RELDIR}/LavaRnd-${VERSION}/manifest-LavaRnd
	${RM} -f ${DISTDIR}/manifest.list
	echo manifest-LavaRnd > ${DISTDIR}/manifest.list
	${CHMOD} 0444 ${DISTDIR}/manifest.list
	${FIND} ${DISTDIR} -name '*.list' | \
	    ${XARGS} ${CAT} | \
	    ${SORT} > ${RELDIR}/LavaRnd-${VERSION}/manifest-LavaRnd
	${CHMOD} 0444 ${RELDIR}/LavaRnd-${VERSION}/manifest-LavaRnd
	${RM} -f ${RELDIR}/LavaRnd-${VERSION}.tar.gz
	cd ${RELDIR}; \
	    ${TAR} -zcf ${RELDIR}/LavaRnd-${VERSION}.tar.gz LavaRnd-${VERSION}
	${CHMOD} 0444 ${RELDIR}/LavaRnd-${VERSION}.tar.gz
	${RM} -f ${RELDIR}/manifest-LavaRnd-${VERSION}
	${CP} ${RELDIR}/LavaRnd-${VERSION}/manifest-LavaRnd \
	      ${RELDIR}/manifest-LavaRnd-${VERSION}
	${CHMOD} 0444 ${RELDIR}/manifest-LavaRnd-${VERSION}
	${RM} -rf ${RELDIR}/LavaRnd-${VERSION}
	@echo "=-_-= ending $@ rule =-_-="
