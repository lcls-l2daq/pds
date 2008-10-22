/**camstream.h
 * Describe the network and file structure for files
 * compatible with camreceiver.
 */

#ifndef __CAMSTREAM_H
#define __CAMSTREAM_H

#undef USE_TCP

#define CAMSTREAM_DEFAULT_PORT		10000
#define CAMSTREAM_PKT_MAXSIZE		(32*1024)

struct camstream_basic_t {
	uint32_t hdr;
	uint32_t pktsize;
};
#define CAMSTREAM_BASIC_HDR		htonl(0)
#define CAMSTREAM_BASIC_PKTSIZE		htonl(sizeof(struct camstream_basic_t))

struct camstream_image_t {
	struct camstream_basic_t base;
	uint8_t format;
#define IMAGE_FORMAT_UNKNOWN	0
#define IMAGE_FORMAT_GRAY8	1
#define IMAGE_FORMAT_GRAY16	2
	uint32_t size;
	uint16_t width;
	uint16_t height;
	void *data;	
};
#define CAMSTREAM_IMAGE_HDR		htonl(('I'<<24) | ('M'<<16) | ('G'<<24) | 0)
#define CAMSTREAM_IMAGE_PKTSIZE		htonl(sizeof(struct camstream_image_t))

enum camstream_proto {
	CAMSTREAM_USE_UDP,
	CAMSTREAM_USE_TCP
};

#ifdef USE_TCP
#define CAMSTREAM_DEFAULT_PROTO	CAMSTREAM_USE_TCP
#else	/* #ifdef USE_TCP */
#define CAMSTREAM_DEFAULT_PROTO	CAMSTREAM_USE_UDP
#endif	/* #ifdef USE_TCP */

#endif	/* #ifndef __CAMSTREAM_H */
