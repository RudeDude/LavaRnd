/*
 * lava_debug - lavapool library debugging, warnings and errors
 *
 * Debug is controlled by the environment variable $LAVA_DEBUG.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lava_debug.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_LAVA_DEBUG_H__)
#  define __LAVARND_LAVA_DEBUG_H__


#include "LavaRnd/lavaquality.h"


/*
 * debug macros
 */
#  if defined(LAVA_DEBUG)

#    define LAVA_DEBUG_B(func, fmt, int1, int2) \
        lava_debug_b(__FILE__, __LINE__, func, fmt, int1, int2)

#    define LAVA_DEBUG_C(func, fmt, int1) \
        lava_debug_c(__FILE__, __LINE__, func, fmt, int1)

#    define LAVA_DEBUG_E(func, err) \
        lava_debug_e(__FILE__, __LINE__, func, err)

#    define LAVA_DEBUG_H(func, buf, len) \
        lava_debug_h(__FILE__, __LINE__, func, buf, len)

#    define LAVA_DEBUG_I(func, fmt, str) \
        lava_debug_i(__FILE__, __LINE__, func, fmt, str)

#    define LAVA_DEBUG_L(func) \
        lava_debug_l(__FILE__, __LINE__, func)

#    define LAVA_DEBUG_P(func, fmt, int1) \
        lava_debug_p(__FILE__, __LINE__, func, fmt, int1)

#    define LAVA_DEBUG_Q(func, str, quality) \
        lava_debug_q(__FILE__, __LINE__, func, str, quality)

#    define LAVA_DEBUG_R(func, len) \
        lava_debug_r(__FILE__, __LINE__, func, len)

#    define LAVA_DEBUG_S(func, fmt, int1) \
        lava_debug_s(__FILE__, __LINE__, func, fmt, int1)

#    define LAVA_DEBUG_V(func, callback) \
        lava_debug_v(__FILE__, __LINE__, func, ((int)callback))

#    define LAVA_DEBUG_Z(func, dtime) \
	lava_debug_z(__FILE__, __LINE__, func, dtime)

#else /* LAVA_DEBUG */

#    define LAVA_DEBUG_B(func, fmt, int1, int2)
#    define LAVA_DEBUG_C(func, fmt, int1)
#    define LAVA_DEBUG_E(func, err)
#    define LAVA_DEBUG_H(func, buf, len)
#    define LAVA_DEBUG_I(func, fmt, str)
#    define LAVA_DEBUG_L(func)
#    define LAVA_DEBUG_P(func, fmt, int1)
#    define LAVA_DEBUG_Q(func, str, quality)
#    define LAVA_DEBUG_R(func, len)
#    define LAVA_DEBUG_S(func, fmt, int1)
#    define LAVA_DEBUG_V(func, callback)
#    define LAVA_DEBUG_Z(func, dtime)

#endif /* LAVA_DEBUG */


/*
 * external functions
 */
extern int lava_init_debug(void);

extern const char *lava_err_name(int err);

extern void lava_debug_b(char *file, int line, char *func,
			 char *fmt, int int1, int int2);

extern void lava_debug_c(char *file, int line, char *func,
			 char *fmt, int int1);

extern void lava_debug_e(char *file, int line, char *func, int err);

extern void lava_debug_h(char *file, int line, char *func,
			 u_int8_t * buf, int len);

extern void lava_debug_i(char *file, int line, char *func,
			 char *fmt, char *str);

extern void lava_debug_l(char *file, int line, char *func);

extern void lava_debug_p(char *file, int line, char *func,
			 char *fmt, int int1);

extern void lava_debug_q(char *file, int line, char *func,
			 char *str, lavaqual quality);

extern void lava_debug_r(char *file, int line, char *func, int len);

extern void lava_debug_s(char *file, int line, char *func,
			 char *fmt, int int1);

extern void lava_debug_v(char *file, int line, char *func, int callback);

extern void lava_debug_z(char *file, int line, char *func, double dtime);


#endif /* __LAVARND_LAVA_DEBUG_H__ */
