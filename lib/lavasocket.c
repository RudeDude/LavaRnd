/*
 * lavasocket - routines for opening connections to LavaRnd related daemons
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lavasocket.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/un.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/rawio.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * cached IP address - used if host:port string matches
 */
static char *cached_hostport = NULL;	/* cached hostname:port */
static struct sockaddr cached_addr;	/* cached hostname address */

/*
 * forward declare functions
 */
static int lava_unix_connect(char *);
static int lava_tcp_connect(char *);
static int lava_unix_listen(char *);
static int lava_tcp_listen(char *);


/*
 * lava_connect - connect to a socket (TCP/IP or Un*x socket)
 *
 * given:
 *      port            host:port or /socket/path of request port
 *
 * returns:
 *      fd      open socket, or <0 on error
 *
 * NOTE: We ignore leading whitespace on the port.
 */
int
lava_connect(char *port)
{
    int fd;	/* socket descriptor */

    /*
     * firewall
     */
    if (port == NULL) {
	/* invalid argument */
	return LAVAERR_BADARG;
    }
    if (lava_ring) {
	/* alarm timeout */
	return LAVAERR_TIMEOUT;
    }
    port += strspn(port, " \t\n");
    if (port[0] == '\0') {
	/* invalid argument */
	return LAVAERR_BADARG;
    }

    /*
     * open a path to a Un*x domain socket, if it is one
     *
     * A hostname cannot contain a '/', nor can it begin with a '.' character.
     * Thus if the port starts with a '/' or '.' then it MUST be a path
     * to a Un*x domain socket.
     *
     * A TCP/IP socket must be of then form hostname:port.  So if port
     * does NOT contain a ':', then it must be a Un*x domain socket.
     */
    if (port[0] == '/' || port[0] == '.' || strchr(port, ':') == NULL) {
	fd = lava_unix_connect(port);

	/*
	 * If it is not a Un*x domain socket path, then it must be host:port.
	 */
    } else {
	fd = lava_tcp_connect(port);
    }

    /*
     * return the open socket or error code
     */
    return fd;
}


/*
 * lava_listen - listen on a socket (TCP/IP or Un*x socket)
 *
 * given:
 *      port            host:port or /socket/path of request port
 *
 * returns:
 *      fd      open socket, or <0 on error
 *
 * NOTE: We ignore leading whitespace on the port.
 */
int
lava_listen(char *port)
{
    int fd;	/* socket descriptor */

    /*
     * firewall
     */
    if (port == NULL) {
	/* invalid argument */
	return LAVAERR_BADARG;
    }
    if (lava_ring) {
	/* alarm timeout */
	return LAVAERR_TIMEOUT;
    }
    port += strspn(port, " \t\n");
    if (port[0] == '\0') {
	/* invalid argument */
	return LAVAERR_BADARG;
    }

    /*
     * open a path to a Un*x domain socket, if it is one
     *
     * A hostname cannot contain a '/', nor can it begin with a '.' character.
     * Thus if the port starts with a '/' or '.' then it MUST be a path
     * to a Un*x domain socket.
     *
     * A TCP/IP socket must be of then form hostname:port.  So if port
     * does NOT contain a ':', then it must be a Un*x domain socket.
     */
    if (port[0] == '/' || port[0] == '.' || strchr(port, ':') == NULL) {
	fd = lava_unix_listen(port);

	/*
	 * If it is not a Un*x domain socket path, then it must be host:port.
	 */
    } else {
	fd = lava_tcp_listen(port);
    }

    /*
     * return the open socket or error code
     */
    return fd;
}


/*
 * lava_unix_connect - connect to a Un*x domain socket
 *
 * given:
 *      port            host:port or /socket/path of request port
 *
 * returns:
 *      fd      open socket, or <0 on error
 *
 */
static int
lava_unix_connect(char *port)
{
    int sock;	/* connected socket */
    struct sockaddr_un conaddr;	/* address to connect to */
    int addrlen;	/* length of socket address */
    struct stat statbuf;	/* socket file status */
    int ret;	/* connect call return */

    /*
     * firewall
     */
    if (port == NULL) {
	/* invalid argument */
	return LAVAERR_BADARG;
    }
    if (lava_ring) {
	/* alarm timeout */
	return LAVAERR_TIMEOUT;
    }

    /*
     * socket firewall
     */
    if (port[0] == '\0') {
	/* invalid socket path, bad address */
	return LAVAERR_BADADDR;
    }
    if (stat(port, &statbuf) < 0) {
	/* no such path, bad address */
	return LAVAERR_BADADDR;
    }
    if (!S_ISSOCK(statbuf.st_mode)) {
	/* not a socket, bad address */
	return LAVAERR_BADADDR;
    }

    /*
     * make a generic Un*x domain stream socket
     */
    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (lava_ring) {
	/* alarm timeout */
	if (sock >= 0) {
	    close(sock);
	}
	return LAVAERR_TIMEOUT;
    }
    if (sock < 0) {
	/* unable to create socket */
	return LAVAERR_SOCKERR;
    }

    /*
     * form the connection address
     */
    memset(&conaddr, 0, sizeof(conaddr));
    conaddr.sun_family = AF_UNIX;
    strncpy(conaddr.sun_path, port, sizeof(conaddr.sun_path));
    addrlen = sizeof(conaddr);

    /*
     * connect to the Un*x domain socket
     */
    ret = connect(sock, (const struct sockaddr *)&conaddr, addrlen);
    if (lava_ring) {
	/* alarm timeout */
	close(sock);
	return LAVAERR_TIMEOUT;
    }
    if (ret < 0) {
	/* unable to open socket */
	close(sock);
	return LAVAERR_CONNERR;
    }

    /*
     * return the open socket
     */
    return sock;
}


/*
 * lava_tcp_connect - connect to a TCP/IP domain socket
 *
 * given:
 *      port            host:port or /socket/path of request port
 *
 * returns:
 *      fd      open socket, or <0 on error
 *
 *
 * NOTE: We only try the first IP address returned by DNS.
 */
static int
lava_tcp_connect(char *port)
{
    struct servent *service;	/* service port entry */
    int portnum;	/* remote port number */
    char *hostname;	/* host part of host:port */
    char *portname;	/* port part of host:port */
    struct hostent *haddr;	/* hostname address */
    struct sockaddr_in *in;	/* IPV4 address */
    int sock;	/* connected socket */
    int ret;	/* connect call return */
    char *p;

    /*
     * firewall
     */
    if (port == NULL) {
	/* invalid argument */
	return LAVAERR_BADARG;
    }
    if (lava_ring) {
	/* alarm timeout */
	return LAVAERR_TIMEOUT;
    }

    /*
     * if we do not matched a cached address, parse port and lookup IP addr
     */
    if (cached_hostport == NULL || strcmp(port, cached_hostport) != 0) {

	/*
	 * split hostname and port
	 */
	hostname = strdup(port);
	if (hostname == NULL) {
	    /* out of memory */
	    return LAVAERR_MALLOC;
	}
	p = strchr(hostname, ':');
	if (p == NULL) {
	    /* no :, invalid argument */
	    free(hostname);
	    return LAVAERR_BADARG;
	}
	*p = '\0';
	portname = p + 1;
	if (hostname[0] == '\0' || portname[0] == '\0') {
	    /* empty host or port, invalid argument */
	    free(hostname);
	    return LAVAERR_BADARG;
	}

	/*
	 * determine port
	 */
	portnum = 0;
	for (p = portname; *p; ++p) {
	    if (!isascii(*p) || !isdigit(*p)) {
		portnum = -1;
		break;
	    }
	}
	if (portnum == 0) {
	    portnum = strtol(portname, NULL, 0);
	} else {
	    service = getservbyname(portname, "tcp");
	    if (lava_ring) {
		/* alarm timeout */
		return LAVAERR_TIMEOUT;
	    }
	    if (service != NULL) {
		portnum = service->s_port;
	    }
	}
	if (portnum <= 0) {
	    /* unknown port, bad address */
	    free(hostname);
	    return LAVAERR_BADADDR;
	}

	/*
	 * determine host IP address
	 */
	haddr = gethostbyname(hostname);
	if (lava_ring) {
	    /* alarm timeout */
	    return LAVAERR_TIMEOUT;
	}
	if (haddr == NULL) {
	    /* unknown host, bad address */
	    free(hostname);
	    return LAVAERR_BADADDR;
	}
	free(hostname);
	if (haddr->h_addrtype != AF_INET) {
	    /* not an IVP4 or IPV6 address, bad address */
	    return LAVAERR_BADADDR;
	}

	/*
	 * we will attempt to cache the port for later
	 */
	if (cached_hostport != NULL) {
	    free(cached_hostport);
	    cached_hostport = NULL;
	}
	cached_hostport = strdup(port);
	if (cached_hostport == NULL) {
	    /* out of memory */
	    return LAVAERR_MALLOC;
	}

	/*
	 * form the socket address and cache it
	 */
	memset(&cached_addr, 0, sizeof(cached_addr));
	in = (struct sockaddr_in *)&cached_addr;
	in->sin_family = haddr->h_addrtype;
	in->sin_port = htons(portnum);
	memcpy(&(in->sin_addr), haddr->h_addr_list[0], haddr->h_length);
    }

    /*
     * make a generic TCP/IP domain stream socket
     */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (lava_ring) {
	/* alarm timeout */
	if (sock >= 0) {
	    close(sock);
	}
	return LAVAERR_TIMEOUT;
    }
    if (sock < 0) {
	/* socket error */
	return LAVAERR_SOCKERR;
    }

    /*
     * connect to the TCP/IP address
     */
    ret = connect(sock, &cached_addr, sizeof(cached_addr));
    if (lava_ring) {
	/* alarm timeout */
	close(sock);
	return LAVAERR_TIMEOUT;
    }
    if (ret < 0) {
	/* unable to open socket */
	close(sock);
	return LAVAERR_CONNERR;
    }

    /*
     * return the open socket
     */
    return sock;
}


/*
 * lava_unix_listen - listen on a Un*x domain socket
 *
 * given:
 *      port            host:port or /socket/path of request port
 *
 * returns:
 *      fd      open socket, or <0 on error
 *
 */
static int
lava_unix_listen(char *port)
{
    int sock;	/* listened socket */
    struct sockaddr_un lisaddr;	/* address to listen to */
    int addrlen;	/* length of socket address */
    struct stat statbuf;	/* socket file status */
    int ret;	/* connect call return */

    /*
     * firewall
     */
    if (port == NULL) {
	/* invalid argument */
	return LAVAERR_BADARG;
    }
    if (lava_ring) {
	/* alarm timeout */
	return LAVAERR_TIMEOUT;
    }

    /*
     * socket firewall
     */
    if (port[0] == '\0') {
	/* invalid socket path, bad address */
	return LAVAERR_BADADDR;
    }
    if (stat(port, &statbuf) >= 0 && !S_ISSOCK(statbuf.st_mode)) {
	/* file exists but is not a socket */
	return LAVAERR_BADADDR;
    }

    /*
     * form the address to listen with
     */
    memset(&lisaddr, 0, sizeof(lisaddr));
    lisaddr.sun_family = AF_UNIX;
    strncpy(lisaddr.sun_path, port, sizeof(lisaddr.sun_path));
    addrlen = sizeof(lisaddr);

    /*
     * make a generic Un*x domain stream socket
     */
    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (lava_ring) {
	/* alarm timeout */
	if (sock >= 0) {
	    close(sock);
	}
	return LAVAERR_TIMEOUT;
    }
    if (sock < 0) {
	/* unable to create socket */
	return LAVAERR_SOCKERR;
    }

    /*
     * bind the socket to the address
     */
    (void)unlink(port);
    if (bind(sock, (const struct sockaddr *)&lisaddr, addrlen) < 0) {
	/* unable to open socket */
	close(sock);
	return LAVAERR_BINDERR;
    }

    /*
     * listen to the socket
     */
    ret = listen(sock, SOMAXCONN);
    if (lava_ring) {
	/* alarm timeout */
	close(sock);
	return LAVAERR_TIMEOUT;
    }
    if (ret < 0) {
	/* unable to open socket */
	close(sock);
	return LAVAERR_LISTENERR;
    }

    /*
     * return the open socket
     */
    return sock;
}


/*
 * lava_tcp_listen - listen on a TCP/IP domain socket
 *
 * given:
 *      port            host:port or /socket/path of request port
 *
 * returns:
 *      fd      open socket, or <0 on error
 *
 *
 * NOTE: We only try the first IP address returned by DNS.
 */
static int
lava_tcp_listen(char *port)
{
    struct servent *service;	/* service port entry */
    int portnum;	/* remote port number */
    char *hostname;	/* host part of host:port */
    char *portname;	/* port part of host:port */
    struct hostent *haddr;	/* hostname address */
    struct sockaddr_in *in;	/* IPV4 address */
    int sock;	/* listened socket */
    int ret;	/* listen call return */
    int on;	/* 1 => turn an option on */
    char *p;

    /*
     * firewall
     */
    if (port == NULL) {
	/* invalid argument */
	return LAVAERR_BADARG;
    }
    if (lava_ring) {
	/* alarm timeout */
	return LAVAERR_TIMEOUT;
    }

    /*
     * if we do not matched a cached address, parse port and lookup IP addr
     */
    if (cached_hostport == NULL || strcmp(port, cached_hostport) != 0) {

	/*
	 * split hostname and port
	 */
	hostname = strdup(port);
	if (hostname == NULL) {
	    /* out of memory */
	    return LAVAERR_MALLOC;
	}
	p = strchr(hostname, ':');
	if (p == NULL) {
	    /* no :, invalid argument */
	    free(hostname);
	    return LAVAERR_BADARG;
	}
	*p = '\0';
	portname = p + 1;
	if (hostname[0] == '\0' || portname[0] == '\0') {
	    /* empty host or port, invalid argument */
	    free(hostname);
	    return LAVAERR_BADARG;
	}

	/*
	 * determine port
	 */
	portnum = 0;
	for (p = portname; *p; ++p) {
	    if (!isascii(*p) || !isdigit(*p)) {
		portnum = -1;
		break;
	    }
	}
	if (portnum == 0) {
	    portnum = strtol(portname, NULL, 0);
	} else {
	    service = getservbyname(portname, "tcp");
	    if (lava_ring) {
		/* alarm timeout */
		return LAVAERR_TIMEOUT;
	    }
	    if (service != NULL) {
		portnum = service->s_port;
	    }
	}
	if (portnum <= 0) {
	    /* unknown port, bad address */
	    free(hostname);
	    return LAVAERR_BADADDR;
	}

	/*
	 * determine host IP address
	 */
	haddr = gethostbyname(hostname);
	if (lava_ring) {
	    /* alarm timeout */
	    return LAVAERR_TIMEOUT;
	}
	if (haddr == NULL) {
	    /* unknown host, bad address */
	    free(hostname);
	    return LAVAERR_BADADDR;
	}
	free(hostname);
	if (haddr->h_addrtype != AF_INET) {
	    /* not an IVP4 or IPV6 address, bad address */
	    return LAVAERR_BADADDR;
	}

	/*
	 * we will attempt to cache the port for later
	 */
	if (cached_hostport != NULL) {
	    free(cached_hostport);
	}
	cached_hostport = strdup(port);
	if (cached_hostport == NULL) {
	    /* out of memory */
	    return LAVAERR_MALLOC;
	}

	/*
	 * form the socket address and cache it
	 */
	memset(&cached_addr, 0, sizeof(cached_addr));
	in = (struct sockaddr_in *)&cached_addr;
	in->sin_family = haddr->h_addrtype;
	in->sin_port = htons(portnum);
	memcpy(&(in->sin_addr), haddr->h_addr_list[0], haddr->h_length);
    }

    /*
     * make a generic TCP/IP domain stream socket
     */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (lava_ring) {
	/* alarm timeout */
	if (sock >= 0) {
	    close(sock);
	}
	return LAVAERR_TIMEOUT;
    }
    if (sock < 0) {
	/* socket error */
	return LAVAERR_SOCKERR;
    }

    /*
     * declare it is OK to rebind to the listening socket address
     *
     * * If the setsockopt is not done, then if you bind using non-zero
     * port and rerun this prog soon after it successfully completes,
     * then bind will fail with an "Address already in use" error.
     */
    on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
	/* unable to setsockopt socket */
	close(sock);
	return LAVAERR_SOCKERR;
    }

    /*
     * bind the socket to the address
     */
    if (bind(sock, (const struct sockaddr *)&cached_addr,
	     sizeof(cached_addr)) < 0) {
	/* unable to open socket */
	close(sock);
	return LAVAERR_BINDERR;
    }

    /*
     * listen to the socket
     */
    ret = listen(sock, SOMAXCONN);
    if (lava_ring) {
	/* alarm timeout */
	close(sock);
	return LAVAERR_TIMEOUT;
    }
    if (ret < 0) {
	/* unable to open socket */
	close(sock);
	return LAVAERR_LISTENERR;
    }

    /*
     * return the open socket
     */
    return sock;
}


/*
 * lavasocket_cleanup - cleanup to make memory leak checkers happy
 *
 * NOTE: This function is called from the dormant() function.
 */
void
lavasocket_cleanup(void)
{
    /* free cached host:port */
    if (cached_hostport != NULL) {
	free(cached_hostport);
	cached_hostport = NULL;
    }
}
