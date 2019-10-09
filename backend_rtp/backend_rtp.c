#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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

#define SAP_MULTICAST_GROUP "sap.mcast.net"
#define SAP_MULTICAST_PORT 9875

//#define PULSEAUDIO_MULTICAST_GROUP "224.0.0.56"
#define PULSEAUDIO_MULTICAST_GROUP "224.2.127.254"

#define MAGIC_RTP_PORT  12077


const char* simple_sdp = 
"v=0\n"
"o=pulse 3779552618 0 IN IP4 172.16.42.2\n"
"s=VirtualOSS RTP Stream on thor.strfry.org\n"
"c=IN IP4 224.0.0.56\n"
"t=3779552618 0\n"
"a=recvonly\n"
"m=audio 12077 RTP/AVP 10\n"
"a=rtpmap:10 L16/44100/2\n"
"a=type:broadcast\n"
;

uint16_t rtp_port = MAGIC_RTP_PORT;

static void
rtp_close(struct voss_backend *pbe)
{
    close(pbe->fd);
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
    addr.sin_addr.s_addr = inet_addr(PULSEAUDIO_MULTICAST_GROUP);
    addr.sin_port = htons(SAP_MULTICAST_PORT);
    addrlen = sizeof(addr);


    connect(pbe->fd, (struct sockaddr*)&addr, addrlen);
    send_sap(pbe->fd, simple_sdp, 0);
    
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(PULSEAUDIO_MULTICAST_GROUP);
    addr.sin_port = htons(rtp_port);
    addrlen = sizeof(addr);

    connect(pbe->fd, (struct sockaddr*)&addr, addrlen);


    // old backend_null stuff
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

uint32_t timestamp = 0;
uint16_t seqnum = 0;
uint32_t ssrc = 3779552618U;

static int
rtp_play_transfer(struct voss_backend *pbe, void *ptr, int len)
{
    uint8_t* rtp_header8 = ptr;
    uint16_t* rtp_header16 = ptr;
    uint32_t* rtp_header32 = ptr;

    int samples = len / sizeof(int16_t) / 2/*->channels*/;
    //timestamp += samples;
    
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

struct voss_backend voss_backend_rtp_play = {
	.open = rtp_open,
	.close = rtp_close,
	.transfer = rtp_play_transfer,
	.delay = rtp_delay,
};

