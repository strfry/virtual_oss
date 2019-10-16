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
//#define SAP_MULTICAST_GROUP "224.2.127.254"
#define SAP_MULTICAST_PORT 9875

#define PULSEAUDIO_MULTICAST_GROUP "224.0.0.56"

#define MAGIC_RTP_PORT  12077


const char* simple_sdp = 
"v=0\n"
"o=root %ld 0 IN IP4 172.16.42.2\n"
"s=VirtualOSS RTP Stream on thor.strfry.org\n"
"c=IN IP4 224.0.0.56\n"
"t=%lu 0\n"
"a=recvonly\n"
"m=audio 12077 RTP/AVP 10\n"
"a=rtpmap:10 L16/44100/2\n"
"a=type:broadcast\n"
;


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
         
   unsigned char ttl = 255;
   setsockopt(pbe->fd, IPPROTO_IP,
		 IP_MULTICAST_TTL,
		 &ttl,
		 sizeof(ttl));

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

    char buffer[1024];

    uint64_t timestamp = time(0) + 2208988800ULL;

    sprintf(buffer, simple_sdp, timestamp, timestamp);
    
    send_sap(pbe->fd, buffer, 0);
    //send_sap(pbe->fd, simple_sdp, 0);
    
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(PULSEAUDIO_MULTICAST_GROUP);
    addr.sin_port = htons(MAGIC_RTP_PORT);
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
uint32_t ssrc = 3780246724U;

const int MAX_MTU = 1400;

static int
rtp_play_transfer(struct voss_backend *pbe, void *ptr, int len)
{
   // printf("rtp_play_transfer(len=%d)\n", len);
    
    uint8_t pkg[MAX_MTU];
    
    const int hdr_len = 12;
    
    len = len > (MAX_MTU - hdr_len) ? (MAX_MTU - hdr_len) : len;
    len = len & (~0x3); // full frames
    
    int samples = len / sizeof(int16_t) / 2/*->channels*/;
    timestamp += samples;
    
    uint8_t* rtp_header8 = pkg;
    uint16_t* rtp_header16 = pkg;
    uint32_t* rtp_header32 = pkg;

    rtp_header8[0] = 2 << 6;
    rtp_header8[1] = 10; // PAYLOAD type 10: L16/44100
    
    rtp_header16[1] = seqnum++;

    rtp_header32[1] = timestamp;
    // rtp_header32[1] = 0; // no timestamp makes pulseaudio guess the sample rate
    rtp_header32[2] = ssrc;


    memcpy(pkg + hdr_len, ptr, len);

    send(pbe->fd, pkg, len + hdr_len, 0);
    return len;
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

