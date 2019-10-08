/* 

multicast.c

The following program sends or receives multicast packets. If invoked
with one argument, it sends a packet containing the current time to an
arbitrarily chosen multicast group and UDP port. If invoked with no
arguments, it receives and prints these packets. Start it as a sender on
just one host and as a receiver on all the other hosts

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>

#define EXAMPLE_PORT 9875
//#define EXAMPLE_GROUP "sap.mcast.net"
#define EXAMPLE_GROUP "224.0.0.56"

  

const char mime_type[] = "application/sdp";

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


int sap_send(int sock, const char* sdp_data, uint8_t goodbye) {
    uint32_t header;
    struct sockaddr_storage sa_buf;
    struct sockaddr *sa = (struct sockaddr*) &sa_buf;
    socklen_t salen = sizeof(sa_buf);
    struct iovec iov[4];
    struct msghdr m;
    ssize_t k;

    if (getsockname(sock, sa, &salen) < 0) {
        perror("getsockname() failed: ");
        return -1;
    }

    int msg_id_hash = 0;

    header = htonl(((uint32_t) 1 << 29) |
                   (goodbye ? (uint32_t) 1 << 26 : 0) |
                   (msg_id_hash));

    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(header);

    if (sa->sa_family == AF_INET) {
        iov[1].iov_base = (void*) &((struct sockaddr_in*) sa)->sin_addr;
        iov[1].iov_len = 4U;
    }
    
    iov[2].iov_base = (char*) mime_type;
    iov[2].iov_len = sizeof(mime_type);

    iov[3].iov_base = (void*)sdp_data;
    iov[3].iov_len = strlen(sdp_data);

    m.msg_name = NULL;
    m.msg_namelen = 0;
    m.msg_iov = iov;
    m.msg_iovlen = 4;
    m.msg_control = NULL;
    m.msg_controllen = 0;
    m.msg_flags = 0;

    if ((k = sendmsg(sock, &m, MSG_DONTWAIT)) < 0)
        perror("sendmsg() failed: ");

    return (int) k;
}

int
main(int argc, char** argv)
{
   struct sockaddr_in addr;
   int addrlen, sock, cnt;
   struct ip_mreq mreq;
   char message[1050];

   /* set up socket */
   sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0) {
     perror("socket");
     exit(1);
   }

   unsigned char loop = 1;
   setsockopt(sock, IPPROTO_IP,
		 IP_MULTICAST_LOOP,
		 &loop,
		 sizeof(loop));

   bzero((char *)&addr, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = 0;
   addr.sin_port = 0;
   //addrlen = sizeof(addr);
   bind(sock, (struct sockaddr*)&addr, sizeof(addr));
   
   bzero((char *)&addr, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = inet_addr(EXAMPLE_GROUP);
   addr.sin_port = htons(EXAMPLE_PORT);
   addrlen = sizeof(addr);

   connect(sock, (struct sockaddr*)&addr, addrlen);

   /* send */
   while (1) {
      struct msghdr msg;

      
      //sprintf(MES, simple_sdp);
      printf("sending: SAP HEADER \n%s\n\n", simple_sdp);
      sap_send(sock, simple_sdp, 0);
      return 0;
      sleep(5);
   }
}

