/*
 * Copyright (c) 2015 Frekk van Blagh <frekk@frekkworks.com>
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

#ifndef __WINDOW_H__
#define __WINDOW_H__

/* Hard-coded sequence ID and fragment size limits
 * These should match the limitations of the protocol. */
#define MAX_SEQ_ID 256
#define MAX_FRAGSIZE_DOWN 2048
#define MAX_FRAGSIZE_UP	255
#define MAX_SEQ_AHEAD (MAX_SEQ_ID / 2)

/* Window function definitions. */
#define WINDOW_SENDING 1
#define WINDOW_RECVING 0

typedef struct fragment {
	uint8_t *data;				/* pointer to fragment data */
	struct timeval lastsent;	/* timestamp of most recent send attempt */
	size_t len;					/* Length of fragment data (0 if fragment unused) */
	unsigned seqID;				/* fragment sequence ID */
	unsigned retries;			/* number of times has been sent or dupes recv'd */
	int acks;					/* number of times packet has been ack'd */
	int ack_other;				/* other way ACK seqID (>=0) or unset (<0) */
	uint8_t compressed;			/* compression flag */
	uint8_t start;				/* start of chunk flag */
	uint8_t end;				/* end of chunk flag */
} fragment;

struct frag_buffer {
	fragment *frags;		/* pointer to array of fragment metadata */
	uint8_t *data;			/* pointer to actual fragment data */
	size_t length;			/* Length of buffer */
	size_t numitems;		/* number of non-empty fragments stored in buffer */
	size_t window_start;	/* Start of window (index) */
	size_t last_write;		/* Last fragment appended (index) */
	size_t chunk_start;		/* index of oldest fragment slot (lowest seqID) in buffer */
	struct timeval timeout;	/* Fragment ACK timeout before resend or drop */
	unsigned windowsize;	/* Max number of fragments in flight */
	unsigned maxfraglen;	/* Max outgoing fragment data size */
	unsigned cur_seq_id;	/* Next unused sequence ID */
	unsigned start_seq_id;	/* lowest seqID that exists in buffer (at index chunk_start) */
	unsigned max_retries;	/* max number of resends before dropping */
	unsigned resends;		/* number of fragments resent or number of dupes received */
	unsigned oos;			/* Number of out-of-sequence fragments received */
	int direction;			/* WINDOW_SENDING or WINDOW_RECVING */
};

extern int window_debug;

/* Window debugging macro */
#ifdef DEBUG_BUILD
#define WDEBUG_LEVEL 2
#define WDEBUG(...) \
	if (debug >= WDEBUG_LEVEL) {\
		TIMEPRINT("[WDEBUG:%s] (%s:%d) ", w->direction == WINDOW_SENDING ? "S" : "R", __FILE__, __LINE__);\
		fprintf(stderr, __VA_ARGS__);\
		fprintf(stderr, "\n");\
	}
#else
#define WDEBUG(...)
#endif

/* Gets pointer to fragment data given fragment index */
#define FRAG_DATA(w, fragIndex) ((w->data + (w->maxfraglen * fragIndex)))

/* Gets index of fragment o fragments after window start */
#define AFTER(w, o) ((w->window_start + o) % w->length)

/* Gets seqID of fragment o fragments after window start seqID */
#define AFTERSEQ(w, o) ((w->start_seq_id + o) % MAX_SEQ_ID)

/* Distance (going forwards) between a and b in window of length l */
#define DISTF(l, a, b) ((a <= b) ? b-a : l-a+b)

/* Distance backwards between a and b in window of length l */
#define DISTB(l, a, b) (l-DISTF(l, a, b))

/* Check if fragment index a is within window_buffer *w */
#define INWINDOW_INDEX(w, a) ((w->window_start < w->window_end) ? \
		(a >= w->window_start && a < w->window_end) : \
		((a >= w->window_start && a < w->length) || \
		(a >= 0 && a < w->window_end)))

/* Check if sequence ID a is within sequence range start to end */
#define INWINDOW_SEQ(start, end, a) ((start < end) ? \
		(a >= start && a < end) : \
		((a >= start && a < MAX_SEQ_ID) || (a < end)))

/* Find the wrapped offset between sequence IDs start and a
 * Note: the maximum possible offset is MAX_SEQ_ID - 1 */
#define SEQ_OFFSET(start, a) ((a >= start) ? a - start : MAX_SEQ_ID - start + a)

/* Wrap index x to a value within the window buffer length */
#define WRAP(x) ((x) % w->length)

/* Wrap index x to a value within the seqID range */
#define WRAPSEQ(x) ((x) % MAX_SEQ_ID)


/* Perform wrapped iteration of statement with pos = (begin to end) wrapped at
 * max, executing statement f for every value of pos. */
#define ITER_FORWARD(begin, end, max, pos, f) { \
		if (end >= begin) \
			for (pos = begin; pos < end && pos < max; pos++) {f}\
		else {\
			for (pos = begin; pos < max; pos++) {f}\
			for (pos = 0; pos < end && pos < max; pos++) {f}\
		}\
	}

/* Window buffer creation */
struct frag_buffer *window_buffer_init(size_t length, unsigned windowsize, unsigned maxfraglen, int dir);

/* Resize buffer, clear and reset stats and data */
void window_buffer_resize(struct frag_buffer *w, size_t length, unsigned maxfraglen);

/* Destroys window buffer instance */
void window_buffer_destroy(struct frag_buffer *w);

/* Clears fragments and resets window stats */
void window_buffer_clear(struct frag_buffer *w);

/* Returns number of available fragment slots (NOT BYTES) */
size_t window_buffer_available(struct frag_buffer *w);

/* Handles fragment received from the sending side (RECV) */
ssize_t window_process_incoming_fragment(struct frag_buffer *w, fragment *f);

/* Reassembles first complete sequence of fragments into data. (RECV)
 * Returns length of data reassembled, or 0 if no data reassembled */
int window_reassemble_data(struct frag_buffer *w, uint8_t *data, size_t *maxlen, uint8_t *compression);

/* Returns number of fragments to be sent */
size_t window_sending(struct frag_buffer *w, struct timeval *);

/* Returns next fragment to be sent or NULL if nothing (SEND) */
fragment *window_get_next_sending_fragment(struct frag_buffer *w, int *other_ack);

/* Sets the fragment with seqid to be ACK'd (SEND) */
void window_ack(struct frag_buffer *w, int seqid);

void window_slide(struct frag_buffer *w, unsigned slide, int delete);

/* To be called after all other processing has been done
 * when anything happens (moves window etc) (SEND/RECV) */
void window_tick(struct frag_buffer *w);

/* Splits data into fragments and adds to the end of the window buffer for sending
 * All fragment meta-data is created here (SEND) */
int window_add_outgoing_data(struct frag_buffer *w, uint8_t *data, size_t len, uint8_t compressed);

#endif /* __WINDOW_H__ */
