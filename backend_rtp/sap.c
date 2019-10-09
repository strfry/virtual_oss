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



int send_sap(int sock, const char* sdp_data, uint8_t goodbye) {
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
