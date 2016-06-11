#ifndef _TRACKER_H
#define _TRACKER_H

typedef enum { UNKNOWN, INIT, CLOSED, OPEN } port_status_t;

/* Port tracker. */
struct tracker {
	/* Start and end of the scanned port. */
	unsigned short start;
	unsigned short end;

	/* Port tracker. */
	unsigned short next;

	/* Open port status. */
	port_status_t status[UINT16_MAX + 1];
};

/* Prototypes. */
void tracker_init(struct tracker *t, const unsigned short start_port,
		const unsigned short end_port);
void tracker_term(struct tracker *t);

#endif /* _TRACKER_H */
