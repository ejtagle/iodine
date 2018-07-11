/*
 * Copyright (c) 2006-2015 Erik Ekman <yarrick@kryo.se>,
 * 2006-2009 Bjorn Andersson <flex@kryo.se>,
 * 2015 Frekk van Blagh <frekk@frekkworks.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __SERVER_H__
#define __SERVER_H__

#ifdef WINDOWS32
#include "windows.h"
#include <winsock2.h>
#else
#include <arpa/nameser.h>
#ifdef DARWIN
#define BIND_8_COMPAT
#include <arpa/nameser_compat.h>
#endif
#define _XPG4_2
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <grp.h>
#include <sys/uio.h>
#include <pwd.h>
#include <netdb.h>
#include <syslog.h>
#endif

/* Max number of incoming queries to hold at one time (recommended to be same as windowsize)
 * Memory = USERS * (sizeof(struct query_buffer) + sizeof(query) * QMEM_LEN) */
#define QMEM_LEN 32

/* QMEM entries contain additional space for DNS responses.
 * Undefine to disable. */
#define USE_DNSCACHE

/* Number of fragments in outgoing buffer.
 * Mem usage: USERS * (MAX_FRAGLEN * OUTFRAGBUF_LEN + sizeof(struct window_buffer)) */
#define OUTFRAGBUF_LEN 64

/* Number of fragments in incoming buffer; must be at least windowsize * 2
 * Minimum recommended = ((max packet size or MTU) / (max up fragsize)) * 2
 * ie. (1200 / 100) * 2 = 24 */
#define INFRAGBUF_LEN 64

#define PASSWORD_ENV_VAR "IODINED_PASS"

#define INSTANCE server

#if !defined(BSD) && !defined(__GLIBC__)
static char *__progname;
#endif

/* Struct with IPv4 and IPv6 file descriptors.
 * Need to be passed on down to tunneling code since we can get a
 * packet on one fd meant for a user on the other.
 */
struct dnsfd {
	int v4fd;
	int v6fd;
};

struct server_instance {
	/* Global server variables */
	int running;
	uint8_t topdomain[128];
	uint8_t passwordmd5[16];
	int check_ip;
	int my_mtu;
	in_addr_t my_ip;
	int netmask;
	in_addr_t ns_ip;
	int bind_port;

	int addrfamily;
	struct dnsfd dns_fds;
	int tun_fd;
	int port;
	int mtu;
	int max_idle_time;
	struct sockaddr_storage dns4addr;
	int dns4addr_len;
	struct sockaddr_storage dns6addr;
	int dns6addr_len;

	int allow_forward_local_port;
	int allow_forward_remote;

	/* settings for forwarding normal DNS to
	 * local real DNS server */
	int bind_fd;
	int bind_enable;
};

extern struct server_instance server;

typedef enum {
	VERSION_ACK,
	VERSION_NACK,
	VERSION_FULL
} version_ack_t;

void server_init();
void server_stop();
int server_tunnel();

#endif /* __SERVER_H__ */
