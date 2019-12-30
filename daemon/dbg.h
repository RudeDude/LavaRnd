/*
 * dbg - lavapool daemon debugging, warnings and errors
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: dbg.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
 *
 * Copyright (c) 2000-2003 by Landon Curt Noll and Simon Cooper.
 * All Rights Reserved.
 *
 * This is open software; you can redistribute it and/or modify it under
 * the terms of the version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 *
 * This software is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General
 * Public License for more details.
 *
 * The file COPYING contains important info about Licenses and Copyrights.
 * Please read the COPYING file for details about this open software.
 *
 * A copy of version 2.1 of the GNU Lesser General Public License is
 * distributed with calc under the filename COPYING-LGPL.  You should have
 * received a copy with calc; if not, write to Free Software Foundation, Inc.
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * For more information on LavaRnd: http://www.LavaRnd.org
 *
 * Share and enjoy! :-)
 */


#if !defined(__DBG_H__)
#define __DBG_H__


/*
 * truth - as we know it
 */
#if !defined(TRUE)
# define TRUE 1
#endif
#if !defined(FALSE)
# define FALSE 0
#endif


/*
 * global declarations
 */
extern char *program;
extern char *prog;
extern int dbg_lvl;
/**/
extern void unsetlog(void);
extern void open_syslog(void);
extern int setlog(char *setlog);
extern void dont_use_stderr(void);
extern void dbg(int level, char *func, char *fmt, ...);
extern void warn(char *func, char *fmt, ...);
extern void fatal(int code, char *func, char *fmt, ...);


#endif /* __DBG_H__ */
