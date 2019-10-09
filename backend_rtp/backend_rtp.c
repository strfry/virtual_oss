#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <time.h>

#include <sys/queue.h>
#include <sys/filio.h>
#include <sys/soundcard.h>

#include "../virtual_int.h"
#include "../virtual_backend.h"


#include <math.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#define EXAMPLE_PORT 9875
//#define EXAMPLE_GROUP "sap.mcast.net"
#define EXAMPLE_GROUP "224.0.0.56"

static void
rtp_close(struct voss_backend *pbe)
{
}

static int
rtp_open(struct voss_backend *pbe, const char *devname,
    int samplerate, int bufsize, int *pchannels, int *pformat)
{
   struct sockaddr_in addr;
   int addrlen, /*sock,*/ cnt;
   char message[1050];

   /* set up socket */
   pbe->fd = socket(AF_INET, SOCK_DGRAM, 0);
   if (pbe->fd < 0) {
     perror("socket");
     exit(1);
   }

   unsigned char loop = 1;
   setsockopt(pbe->fd, IPPROTO_IP,
		 IP_MULTICAST_LOOP,
		 &loop,
		 sizeof(loop));

    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = 0;
    addr.sin_port = 0;
    addrlen = sizeof(addr);
    bind(pbe->fd, (struct sockaddr*)&addr, addrlen);

    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(EXAMPLE_GROUP);
    addr.sin_port = htons(12077);
    addrlen = sizeof(addr);

    connect(pbe->fd, (struct sockaddr*)&addr, addrlen);


	int value[3];
	int i;

	value[0] = *pformat & VPREFERRED_SNE_AFMT;
	value[1] = *pformat & VPREFERRED_SLE_AFMT;
	value[2] = *pformat & VPREFERRED_SBE_AFMT;

	for (i = 0; i != 3; i++) {
		if (value[i] == 0)
			continue;
		*pformat = value[i];
		return (0);
	}
	return (-1);
}

static void
rtp_wait(void)
{
	struct timespec ts;
	uint64_t delay;
	uint64_t nsec;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	nsec = ((unsigned)ts.tv_sec) * 1000000000ULL + ts.tv_nsec;

	delay = voss_dsp_samples;
	delay *= 1000000000ULL;
	delay /= voss_dsp_sample_rate;

	usleep((delay - (nsec % delay)) / 1000);
}

static int
rtp_rec_transfer(struct voss_backend *pbe, void *ptr, int len)
{

	if (voss_has_synchronization == 0)
		rtp_wait();
	memset(ptr, 0, len);
	return (len);
}


uint32_t timestamp = 0;
uint16_t seqnum = 0;
uint32_t ssrc = 3779552618U;
    

static int
rtp_play_transfer(struct voss_backend *pbe, void *ptr, int len)
{
    uint32_t* rtp_header32 = ptr;

    uint8_t* rtp_header8 = ptr;
    
    uint16_t* rtp_header16 = ptr;
    rtp_header16[1] = seqnum++;

    rtp_header8[0] = 2 << 6;
    rtp_header8[1] = 10; // PAYLOAD t
    rtp_header8[2] = 0;
    rtp_header8[3] = 0;

    rtp_header32[1] = timestamp;
    rtp_header32[2] = ssrc;

    timestamp += len;

    
	return send(pbe->fd, ptr, 1024, 0);
}

static void
rtp_delay(struct voss_backend *pbe, int *pdelay)
{
	*pdelay = -1;
}

//struct voss_backend voss_backend_null_rec = {
//	.open = null_open,
//	.close = null_close,
//	.transfer = null_rec_transfer,
//	.delay = null_delay,
//};

struct voss_backend voss_backend_rtp_play = {
	.open = rtp_open,
	.close = rtp_close,
	.transfer = rtp_play_transfer,
	.delay = rtp_delay,
};

