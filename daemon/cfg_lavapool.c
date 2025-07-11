/*
 * cfg_lavapool - lavapool (private) configuration information
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: cfg_lavapool.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "LavaRnd/sha1.h"
#include "LavaRnd/cfg.h"

#include "cfg_lavapool.h"
#include "dbg.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * the default private configuration
 */
static struct cfg_lavapool def_cfg = {
    NULL,			/* chaos command line or driver line for lavapool */
    LAVA_DEF_FASTPOOL,		/* def fast pool filling level */
    LAVA_DEF_SLOWPOOL,		/* def slow filling level */
    LAVA_DEF_POOLSIZE,		/* size of lava pool */
    LAVA_DEF_FAST_CYCLE,	/* def fastest chan fill cycle */
    LAVA_DEF_SLOW_CYCLE,	/* def slowest chan fill cycle */
    LAVA_DEF_MAXCLINETS,	/* max number if clients if > 0 */
    LAVA_DEF_TIMEOUT,		/* client timeout in secs if > 0.0 */
    LAVA_DEF_USE_PREFIX		/* 0==>dont use system stuff as a URL content prefix */
};
struct cfg_lavapool cfg_lavapool;	/* current cfg.lavapool cfg */

#define MAXLINE 1024		/* longest config line allowed */


/*
 * config_priv - load the standard configuration file for url
 *
 * given:
 *      cfg_file        path to cfg.random (or NULL for default path)
 *      *config         configuration to set
 *
 * returns:
 *      0 - all of OK, *config is the new configuration
 *      <0 - error, *config is not changed
 *
 * All lines that the empty or that start with # are treated as
 * configuration file comments and are ignored.
 */
int
config_priv(char *cfg_file, struct cfg_lavapool *config)
{
    FILE *f;	/* configuration file stream */
    char buf[MAXLINE + 1];	/* config file buffer */
    int linenum;	/* config line number */
    struct cfg_lavapool new;	/* new configuration to set */

    /*
     * firewall
     */
    if (config == NULL) {
	return -1;		/* bad arg */
    }
    if (cfg_file == NULL) {
	cfg_file = LAVA_LAVAPOOL_CFG;
    }

    /*
     * firewall
     */
    if (cfg_file == NULL || config == NULL) {
	return -1;		/* bad arg */
    }

    /*
     * open the cfg.random file
     */
    f = fopen(cfg_file, "r");
    if (f == NULL) {
	warn("config_priv", "cannot open cfg_file");
	return -1;		/* bad open */
    }

    /*
     * initialize the new configuration to default values
     */
    new = def_cfg;

    /*
     * parse each line
     */
    buf[MAXLINE - 1] = '\0';
    buf[MAXLINE] = '\0';
    linenum = 0;
    while (fgets(buf, MAXLINE, f) != NULL) {
	char *fld1;	/* start if 1st field */
	char *fld2;	/* start if 2nd field */
	char *p;

	/* count line */
	++linenum;

	/* lines that are too long */
	if (buf[MAXLINE - 1] != '\0' && buf[MAXLINE - 1] != '\n') {
	    fclose(f);
	    warn("config_priv", "line %d is too long", linenum);
	    return -1;		/* line too long */
	}

	/* ignore lines that begin with # */
	if (buf[0] == '#') {
	    continue;
	}

	/* ignore leading whitespace */
	fld1 = buf + strspn(buf, " \t");

	/* ignore empty lines */
	if (fld1[0] == '\0' || fld1[0] == '\n') {
	    continue;
	}

	/* must have an =, the 1st which splits the two fields */
	p = strchr(fld1, '=');
	if (p == NULL) {
	    warn("config_priv", "line %d is missing an =", linenum);
	    fclose(f);
	    return -1;
	}
	*p = '\0';
	fld2 = p + 1;

	/* trim trailing whitespace from the end of the 1st field */
	for (--p; p > fld1; --p) {
	    if (!isascii(*p) || !isspace(*p)) {
		break;
	    }
	}
	if (p <= fld1) {
	    fclose(f);
	    warn("config_priv", "line %d as empty st field", linenum);
	    return -1;
	}
	*(p + 1) = '\0';

	/* trim leading whitespace from the start of the 2nd field */
	fld2 += strspn(fld2, " \t");

	/* trim trailing whitespace from end of the 2nd field */
	for (p = fld2 + strlen(fld2) - 1; p > fld2; --p) {
	    if (!isascii(*p) || !isspace(*p)) {
		break;
	    }
	}
	*(p + 1) = '\0';

	/*
	 * we now have two fields, match on the 1st field
	 *
	 * This is not the most efficient way to parse, but it does
	 * the job for as little as we need to do this.
	 */
	if (strcmp(fld1, "chaos") == 0) {
	    new.chaos = strdup(fld2);

	    if (new.chaos == NULL) {
		warn("config_priv", "line %d: strdup malloc failed", linenum);
		fclose(f);
		return -1;
	    }
	} else if (strcmp(fld1, "fastpool") == 0) {
	    errno = 0;
	    new.fastpool = strtol(fld2, NULL, 0);
	    if (errno == ERANGE || new.fastpool < 1) {
		warn("config_priv", "line %d: fastpool must be > 0", linenum);
		fclose(f);
		return -1;
	    }
	} else if (strcmp(fld1, "slowpool") == 0) {
	    errno = 0;
	    new.slowpool = strtol(fld2, NULL, 0);
	    if (errno == ERANGE || new.slowpool < 1) {
		warn("config_priv", "line %d: slowpool must be > 0", linenum);
		fclose(f);
		return -1;
	    }
	} else if (strcmp(fld1, "poolsize") == 0) {
	    errno = 0;
	    new.poolsize = strtol(fld2, NULL, 0);
	    if (errno == ERANGE || new.poolsize < 1) {
		warn("config_priv", "line %d: poolsize must be > 0", linenum);
		fclose(f);
		return -1;
	    }
	} else if (strcmp(fld1, "fast_cycle") == 0) {
	    errno = 0;
	    new.fast_cycle = strtod(fld2, NULL);
	    if (errno == ERANGE || new.fast_cycle <= 0.0) {
		warn("config_priv",
		     "line %d: fast_cycle must be > 0.0", linenum);
		fclose(f);
		return -1;
	    }
	} else if (strcmp(fld1, "slow_cycle") == 0) {
	    errno = 0;
	    new.slow_cycle = strtod(fld2, NULL);
	    if (errno == ERANGE || new.slow_cycle <= 0.0) {
		warn("config_priv",
		     "line %d: slow_cycle must be > 0.0", linenum);
		fclose(f);
		return -1;
	    }
	} else if (strcmp(fld1, "maxclients") == 0) {
	    errno = 0;
	    new.maxclients = strtol(fld2, NULL, 0);
	    if (errno == ERANGE || new.maxclients < 0) {
		warn("config_priv", "line %d: maxclients must be >= 0",
		     linenum);
		fclose(f);
		return -1;
	    }
	} else if (strcmp(fld1, "timeout") == 0) {
	    errno = 0;
	    new.timeout = strtod(fld2, NULL);
	    if (errno == ERANGE || new.timeout < 0.0) {
		warn("config_priv", "line %d: timeout must be >= 0.0",
		     linenum);
		fclose(f);
		return -1;
	    }
	} else if (strcmp(fld1, "prefix") == 0) {
	    errno = 0;
	    new.prefix = strtol(fld2, NULL, 0);
	    if (errno == ERANGE || new.prefix < 0 || new.prefix > 1) {
		warn("config_priv", "line %d: prefix must be 0 or 1", linenum);
		fclose(f);
		return -1;
	    }
	} else {
	    warn("config_priv", "line %d unknown name", linenum);
	    fclose(f);
	    return -1;
	}
    }
    if (!feof(f)) {
	warn("config_priv", "I/O stopped early on %s", cfg_file);
	fclose(f);
	return -1;		/* I/O error */
    }
    fclose(f);

    /*
     * must have a URL
     */
    if (new.chaos == NULL) {
	warn("config_priv", "%s: missing chaos", cfg_file);
	return -1;		/* no URL given */
    }

    /*
     * pool levels must be in range
     */
    if (new.fastpool >= new.slowpool) {
	warn("config_priv", "%s: fastpool: %d must be < slowpool: %d",
	     cfg_file, new.fastpool, new.slowpool);
	return -1;		/* bogus levels */
    }
    if (new.slowpool >= new.poolsize) {
	warn("config_priv", "%s: slowpool: %d must be < poolsize: %d",
	     cfg_file, new.slowpool, new.poolsize);
	return -1;		/* bogus levels */
    }

    /*
     * fast_cycle must be faster than slow_cycle
     */
    if (new.fast_cycle >= new.slow_cycle) {
	warn("config_priv", "%s: fast_cycle: %.3f must be < slow_cycle: %.3f",
	     cfg_file, new.fast_cycle, new.slow_cycle);
	return -1;		/* bogus levels */
    }

    /*
     * we have a valid configuration, load its duplicate into 2nd arg and return
     */
    if (dup_cfg_lavapool(&new, config) == NULL) {
	warn("config_priv", "dup_cfg_lavapool failed!!");
	return -1;
    }
    dbg(1, "config_priv", "chaos: %s", config->chaos);
    dbg(1, "config_priv", "fastpool: %d  slowpool: %d",
	config->fastpool, config->slowpool);
    dbg(1, "config_priv", "poolsize: %d", config->poolsize);
    dbg(1, "config_priv", "fast_cycle: %.3f  slow_cycle: %.3f",
	config->fast_cycle, config->slow_cycle);
    dbg(1, "config_priv", "maxclients: %d  timeout: %.3f  prefix: %d",
	config->maxclients, config->timeout, config->prefix);
    free(new.chaos);
    return 0;			/* success */
}


/*
 * dup_cfg_lavapool - duplicate a configuration
 *
 * given:
 *      cfg1    pointer to configuration to duplicate
 *      cfg2    pointer to where to place duplicated information
 *
 * returns:
 *      cfg2 if OK or NULL if error
 *
 * This function takes care of internal malloc-ing.
 *
 * NOTE: Call free_cfg() when the configuration is no longer needed.
 *
 * NOTE: Even if this function returns NULL, it will copy all but the
 *       chaos element string, provided that this function was
 *       given non-NULL args.
 */
struct cfg_lavapool *
dup_cfg_lavapool(struct cfg_lavapool *cfg1, struct cfg_lavapool *cfg2)
{
    struct cfg_lavapool new;	/* newly configuration */

    /* firewall */
    if (cfg1 == NULL || cfg2 == NULL) {
	return NULL;
    }

    /*
     * duplicate non-strings
     */
    new = *cfg1;

    /*
     * duplicate strings
     */
    new.chaos = (cfg1->chaos ? strdup(cfg1->chaos) : NULL);

    *cfg2 = new;
    if (cfg1->chaos !=NULL && new.chaos == NULL) {
	return NULL;
    }

    /*
     * return duplication
     */
    return cfg2;
}


/*
 * free_cfg_lavapool - free strings from a configuration
 *
 * given:
 *      cfg     pointer to from which to free
 *
 * This function will free storage from a duplicated configuration (see
 * dup_cfg()).  Internal points to freed data are set to NULL.
 *
 * NOTE: This function does NOT free the actual structure.
 */
void
free_cfg_lavapool(struct cfg_lavapool *cfg)
{
    /* firewall */
    if (cfg == NULL) {
	return;
    }

    /*
     * free malloc-ed strings
     */
    if (cfg->chaos !=NULL) {
	free(cfg->chaos);
    }
}
