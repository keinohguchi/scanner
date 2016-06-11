#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "utils.h"
#include "scanner.h"

static const size_t iphdrlen = 20;
static const size_t tcphdrlen = 20;
static char addr[INET_ADDRSTRLEN];

static int reader(struct scanner *sc)
{
	char src[INET_ADDRSTRLEN];
	struct sockaddr_in *sin;
	unsigned short port;
	struct tcphdr *tcp;
	struct iphdr *ip;
	int ret;

	ret = recv(sc->rawfd, sc->ibuf, sizeof(sc->ibuf), 0);
	if (ret < 0) {
		if (errno == EAGAIN)
			return 0;
		fatal("recv(3)");
	}

	/* Ignore packet less than 40(IP + TCP header size) bytes. */
	if (ret < iphdrlen + tcphdrlen)
		return -1;

	/* Drop the packet which is not from the destination. */
	sin = (struct sockaddr_in *) sc->dst->ai_addr;
	ip = (struct iphdr *) sc->ibuf;
	if (ip->saddr != sin->sin_addr.s_addr) {
		debug("Drop packet from non-target host(%s)\n",
			inet_ntop(AF_INET, &ip->saddr, src, sizeof(src)));
		return -1;
	}

	inet_ntop(AF_INET, &ip->saddr, src, sizeof(src));
	tcp = (struct tcphdr *) (ip + 1);
	port = ntohs(tcp->source);
	debug("Recv from %s:%d\n", src, port);
	dump(sc->ibuf, ret);
	sc->icounter++;

	/* We only care about packet with SA flag on. */
	if (tcp->syn == 0 || tcp->ack == 0) {
		debug("Drop packet w/o SYN/ACK from host(%s:%d)\n", src, port);
		return -1;
	}

	info("Port %d is open on %s\n", port, src);

	return port;
}

static unsigned short tcp_checksum(struct scanner *sc, struct tcphdr *tcp)
{
	struct cdata {
		u_int32_t saddr;
		u_int32_t daddr;
		u_int8_t buf;
		u_int8_t protocol;
		u_int16_t length;
		struct tcphdr tcp;
	} *cdata = (struct cdata *) sc->cbuf;
	cdata->tcp = *tcp;

	return checksum((uint16_t *) cdata, sizeof(struct cdata));
}

static int writer(struct scanner *sc)
{
	char dst[INET_ADDRSTRLEN];
	struct sockaddr_in *sin;
	struct tcphdr *tcp;
	struct iphdr *ip;
	int ret;

	/* IP header. */
	ip = (struct iphdr *) sc->obuf;
	ip->id = htonl(54321); /* randomize. */

	/* TCP header. */
	tcp = (struct tcphdr *)(sc->obuf + iphdrlen);
	tcp->source = htons(1024);
	tcp->dest = htons(sc->next_port);
	tcp->seq = 0;
	tcp->ack_seq = 0;
	tcp->res1 = 0;
	tcp->doff = 5;
	tcp->syn = 1;
	tcp->window = 0;
	tcp->check = 0;
	tcp->urg_ptr = 0;
	tcp->check = tcp_checksum(sc, tcp);

	ret = sendto(sc->rawfd, sc->obuf, sc->olen, 0, sc->dst->ai_addr,
			sc->dst->ai_addrlen);
	if (ret != sc->olen) {
		if (ret < 0)
			warn("sendto() error\n");
		else
			info("sendto() can't send full data.  Will retry\n");
		return -1;
	}

	inet_ntop(AF_INET, &ip->daddr, dst, sizeof(dst));
	debug("Sent to %s:%d\n", dst, ntohs(tcp->dest));
	dump(sc->obuf, sc->olen);

	return ret;
}

int scanner4_tcp_init(struct scanner *sc)
{
	struct iphdr *ip = (struct iphdr *) sc->obuf;

	/* TCPv4 specific reader/writer. */
	sc->reader = reader;
	sc->writer = writer;

	/* We send both IP and TCP header portion. */
	sc->olen = sizeof(struct iphdr) + sizeof(struct tcphdr);

	/* Prepare the checksum buffer. */
	struct cdata {
		u_int32_t saddr;
		u_int32_t daddr;
		u_int8_t buf;
		u_int8_t protocol;
		u_int16_t length;
		struct tcphdr tcp;
	} *cdata = (struct cdata *) sc->cbuf;
	cdata->saddr = ip->saddr;
	cdata->daddr = ip->daddr;
	cdata->buf = 0;
	cdata->protocol = sc->dst->ai_protocol;
	cdata->length = htons(tcphdrlen);

	return 0;
}
