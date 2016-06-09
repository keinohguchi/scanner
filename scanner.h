#ifndef _SCANNER_H
#define _SCANNER_H

#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netdb.h>

/* Command line options. */
extern bool debug_flag;
extern bool packet_dump_flag;

/* Scanner manager. */
struct scanner {
	/* Event manager. */
	struct epoll_event ev;
	int eventfd;

	/* Raw socket for the data packets. */
	int rawfd;

	/* Start time. */
	time_t start_time;

	/* Read/write buffers. */
	unsigned char ibuf[BUFSIZ];
	unsigned char obuf[BUFSIZ];
	size_t olen;

	/* Source and destination addresses. */
	struct addrinfo hints;
	struct sockaddr_storage src;
	struct addrinfo *dst;

	/* Scanning port related info. */
	int next_port;
	int start_port;
	int end_port;

	/* Packet counters. */
	size_t icounter;
	size_t ocounter;

	/* TCP header checksum buffer. */
	unsigned char cbuf[BUFSIZ];

	/* Reader and writer of the data packages. */
	int (*reader)(struct scanner *sc);
	int (*writer)(struct scanner *sc);
};

/* Inlines. */
static inline unsigned short checksum(unsigned short *buf, int nwords)
{
	unsigned long sum;

	for (sum = 0; nwords > 0; nwords--)
		sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return (unsigned short)(~sum);
}

/* Prototypes. */
int scanner_wait(struct scanner *sc);
void scanner_exec(struct scanner *sc);
void scanner_init(struct scanner *sc, const char *name, int family,
		int proto, const unsigned short start_port,
		const unsigned short end_port, const char *ifname);
void scanner_term(struct scanner *sc);

#endif /* _SCANNER_H */
