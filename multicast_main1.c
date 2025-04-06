#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include <sys/utsname.h>   // dla uname()
#include <pthread.h> 
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <signal.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include 	<sys/ioctl.h>
#include 	<unistd.h>
#include 	<net/if.h>
#include	<netdb.h>
#include	<sys/utsname.h>
#include	<linux/un.h>

#define MAXLINE 1024
#define SA      struct sockaddr
#define IPV6 1

void *receive_loop(void *arg) {
    int recvfd = *((int *)arg);
    char msgbuf[1024];
    struct sockaddr_storage src_addr;
    socklen_t addrlen = sizeof(src_addr);

    while (1) {
        int n = recvfrom(recvfd, msgbuf, sizeof(msgbuf) - 1, 0,
                         (struct sockaddr *)&src_addr, &addrlen);
        if (n > 0) {
            msgbuf[n] = '\0';
            printf("💬 %s\n", msgbuf);
        }
    }
    return NULL;
}


unsigned int
_if_nametoindex(const char *ifname)
{
	int s;
	struct ifreq ifr;
	unsigned int ni;
    struct utsname sysinfo;
    uname(&sysinfo);
	s = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (s != -1) {

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	
	if (ioctl(s, SIOCGIFINDEX, &ifr) != -1) {
			close(s);
			return (ifr.ifr_ifindex);
	}
		close(s);
		return -1;
	}
}

// #include <net/if.h>
// unsigned int if_nametoindex(const char *ifname);
// char *if_indextoname(unsigned int ifindex, char *ifname);

int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp)
{
	int sockfd, n;
	struct addrinfo	hints, *res, *ressave;
	struct sockaddr_in6 *pservaddrv6;
	struct sockaddr_in *pservaddrv4;

	*saptr = malloc( sizeof(struct sockaddr_in6));
	
	pservaddrv6 = (struct sockaddr_in6*)*saptr;

	bzero(pservaddrv6, sizeof(struct sockaddr_in6));

	if (inet_pton(AF_INET6, serv, &pservaddrv6->sin6_addr) <= 0){
	
		free(*saptr);
		*saptr = malloc( sizeof(struct sockaddr_in));
		pservaddrv4 = (struct sockaddr_in*)*saptr;
		bzero(pservaddrv4, sizeof(struct sockaddr_in));

		if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0){
			fprintf(stderr,"AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
			return -1;
		}else{
			pservaddrv4->sin_family = AF_INET;
			pservaddrv4->sin_port   = htons(port);
			*lenp =  sizeof(struct sockaddr_in);
			if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
				fprintf(stderr,"AF_INET socket error : %s\n", strerror(errno));
				return -1;
			}
		}

	}else{
		pservaddrv6 = (struct sockaddr_in6*)*saptr;
		pservaddrv6->sin6_family = AF_INET6;
		pservaddrv6->sin6_port   = htons(port);	/* daytime server */
		*lenp =  sizeof(struct sockaddr_in6);

		if ( (sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
			fprintf(stderr,"AF_INET6 socket error : %s\n", strerror(errno));
			return -1;
		}

	}

	return(sockfd);
}
/* end send_udp_socket */

int family_to_level(int family)
{
	switch (family) {
	case AF_INET:
		return IPPROTO_IP;
#ifdef	IPV6
	case AF_INET6:
		return IPPROTO_IPV6;
#endif
	default:
		return -1;
	}
}

int mcast_join(int sockfd, const SA *grp, socklen_t grplen,
		   const char *ifname, u_int ifindex)
{
	struct group_req req;
	if (ifindex > 0) {
		req.gr_interface = ifindex;
	} else if (ifname != NULL) {
		if ( (req.gr_interface = if_nametoindex(ifname)) == 0) {
			errno = ENXIO;	/* if name not found */
			return(-1);
		}
	} else
		req.gr_interface = 0;
	if (grplen > sizeof(req.gr_group)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(&req.gr_group, grp, grplen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),
			MCAST_JOIN_GROUP, &req, sizeof(req)));
}
/* end mcast_join */

int mcast_join_org(int sockfd, const SA *grp, socklen_t grplen,
		   const char *ifname, u_int ifindex)
{
#ifdef MCAST_JOIN_GROUP
	struct group_req req;
	if (ifindex > 0) {
		req.gr_interface = ifindex;
	} else if (ifname != NULL) {
		if ( (req.gr_interface = if_nametoindex(ifname)) == 0) {
			errno = ENXIO;	/* if name not found */
			return(-1);
		}
	} else
		req.gr_interface = 0;
	if (grplen > sizeof(req.gr_group)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(&req.gr_group, grp, grplen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),
			MCAST_JOIN_GROUP, &req, sizeof(req)));
#else
/* end mcast_join1 */

/* include mcast_join2 */
	switch (grp->sa_family) {
	case AF_INET: {
		struct ip_mreq		mreq;
		struct ifreq		ifreq;

		memcpy(&mreq.imr_multiaddr,
			   &((const struct sockaddr_in *) grp)->sin_addr,
			   sizeof(struct in_addr));

		if (ifindex > 0) {
			if (if_indextoname(ifindex, ifreq.ifr_name) == NULL) {
				errno = ENXIO;	/* i/f index not found */
				return(-1);
			}
			goto doioctl;
		} else if (ifname != NULL) {
			strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
doioctl:
			if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
				return(-1);
			memcpy(&mreq.imr_interface,
				   &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr,
				   sizeof(struct in_addr));
		} else
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);

		return(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
						  &mreq, sizeof(mreq)));
	}
/* end mcast_join2 */

/* include mcast_join3 */
#ifdef	IPV6
#ifndef	IPV6_JOIN_GROUP		/* APIv0 compatibility */
#define	IPV6_JOIN_GROUP		IPV6_ADD_MEMBERSHIP
#endif
	case AF_INET6: {
		struct ipv6_mreq	mreq6;

		memcpy(&mreq6.ipv6mr_multiaddr,
			   &((const struct sockaddr_in6 *) grp)->sin6_addr,
			   sizeof(struct in6_addr));

		if (ifindex > 0) {
			mreq6.ipv6mr_interface = ifindex;
		} else if (ifname != NULL) {
			if ( (mreq6.ipv6mr_interface = if_nametoindex(ifname)) == 0) {
				errno = ENXIO;	/* i/f name not found */
				return(-1);
			}
		} else
			mreq6.ipv6mr_interface = 0;

		return(setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
						  &mreq6, sizeof(mreq6)));
	}
#endif

	default:
		errno = EAFNOSUPPORT;
		return(-1);
	}
#endif
}
/* end mcast_join3_org */

int sockfd_to_family(int sockfd)
{
	struct sockaddr_storage ss;
	socklen_t	len;

	len = sizeof(ss);
	if (getsockname(sockfd, (SA *) &ss, &len) < 0)
		return(-1);
	return(ss.ss_family);
}

int mcast_set_loop(int sockfd, int onoff)
{
	switch (sockfd_to_family(sockfd)) {
	case AF_INET: {
		u_char		flag;

		flag = onoff;
		return(setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
						  &flag, sizeof(flag)));
	}

#ifdef	IPV6
	case AF_INET6: {
		u_int		flag;

		flag = onoff;
		return(setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
						  &flag, sizeof(flag)));
	}
#endif

	default:
		errno = EAFNOSUPPORT;
		return(-1);
	}
}
/* end mcast_set_loop */


void *recv_loop(void *args) {
    int recvfd = *((int *)args);
    char buffer[MAXLINE+1];
    struct sockaddr_storage src_addr;
    socklen_t addrlen = sizeof(src_addr);

    while (1) {
        int n = recvfrom(recvfd, buffer, MAXLINE, 0, (struct sockaddr *) &src_addr, &addrlen);
        if (n < 0) {
            perror("recvfrom error");
            continue;
        }

        buffer[n] = '\0';
        printf("\n💬 %s\n> ", buffer);
        fflush(stdout);
    }

    return NULL;
}

void *send_loop(void *args) {
    int sockfd = ((int *)args)[0];
    SA *dest = (SA *) ((void **)args)[1];
    socklen_t salen = *((socklen_t *) ((void **)args)[2]);

    struct utsname myname;
    uname(&myname);

    char input[900];
    char message[1024];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = 0; // usuń \n
        snprintf(message, sizeof(message), "%s: %s", myname.nodename, input);

        if (sendto(sockfd, message, strlen(message), 0, dest, salen) < 0)
            perror("sendto error");
    }

    return NULL;
}


#define	SENDRATE	5		/* send one datagram every five seconds */

void send_all(int sendfd, SA *sadest, socklen_t salen)
{
	char		line[MAXLINE];		/* hostname and process ID */
	struct utsname	myname;

	if (uname(&myname) < 0)
		perror("uname error");
	snprintf(line, sizeof(line), "%s, PID=%d", myname.nodename, getpid());

	for ( ; ; ) {
		if(sendto(sendfd, line, strlen(line), 0, sadest, salen) < 0 )
		  fprintf(stderr,"sendto() error : %s\n", strerror(errno));
		sleep(SENDRATE);
	}
}

void recv_all(int recvfd, socklen_t salen)
{
	int					n;
	char				line[MAXLINE+1];
	socklen_t			len;
	struct sockaddr		*safrom;
	char str[128];
	struct sockaddr_in6*	 cliaddr;
	struct sockaddr_in*	 cliaddrv4;
	char			addr_str[INET6_ADDRSTRLEN+1];

	safrom = malloc(salen);

	for ( ; ; ) {
		len = salen;
		if( (n = recvfrom(recvfd, line, MAXLINE, 0, safrom, &len)) < 0 )
		  perror("recvfrom() error");

		line[n] = 0;	/* null terminate */
		
		if( safrom->sa_family == AF_INET6 ){
		      cliaddr = (struct sockaddr_in6*) safrom;
		      inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr->sin6_addr,  addr_str, sizeof(addr_str));
		}
		else{
		      cliaddrv4 = (struct sockaddr_in*) safrom;
		      inet_ntop(AF_INET, (struct sockaddr  *) &cliaddrv4->sin_addr,  addr_str, sizeof(addr_str));
		}

		printf("Datagram from %s : %s (%d bytes)\n", addr_str, line, n);
		fflush(stdout);
	}
}



int
main(int argc, char **argv)
{
	int sendfd, recvfd;
	const int on = 1;
	socklen_t salen;
	struct sockaddr	*sasend, *sarecv;
	struct sockaddr_in6 *ipv6addr;
	struct sockaddr_in *ipv4addr;

	if (argc != 4){
		fprintf(stderr, "usage: %s  <IP-multicast-address> <port#> <if name>\n", argv[0]);
		return 1;
	}

	sendfd = snd_udp_socket(argv[1], atoi(argv[2]), &sasend, &salen);

	if ( (recvfd = socket(sasend->sa_family, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		return 1;
	}

	sarecv = malloc(salen);
	memcpy(sarecv, sasend, salen);

	setsockopt(sendfd, SOL_SOCKET, SO_BINDTODEVICE, argv[3], strlen(argv[3]));
	
	if(sarecv->sa_family == AF_INET6){
	  ipv6addr = (struct sockaddr_in6 *) sarecv;
	  ipv6addr->sin6_addr =  in6addr_any;

	  int32_t ifindex;
      ifindex = if_nametoindex(argv[3]);
      if(setsockopt(sendfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, 
	  				&ifindex, sizeof(ifindex)) < 0){
	  		perror("setting local interface");
			exit(1);};
	  }

	if(sarecv->sa_family == AF_INET){
	  ipv4addr = (struct sockaddr_in *) sarecv;
	  ipv4addr->sin_addr.s_addr =  htonl(INADDR_ANY);

	  struct in_addr        localInterface;
	  struct ifreq ifr;
	  strncpy(ifr.ifr_name, argv[3], IFNAMSIZ);
	  ioctl(sendfd, SIOCGIFADDR, &ifr);
	  localInterface = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	  if (setsockopt(sendfd, IPPROTO_IP, IP_MULTICAST_IF,
						(char *)&localInterface,
						sizeof(localInterface)) < 0) {
			perror("setting local interface");
			exit(1);
	  }
	}
	
	if( bind(recvfd, sarecv, salen) < 0 ){
	    fprintf(stderr,"bind error : %s\n", strerror(errno));
	    return 1;
	}
	
	if( mcast_join(recvfd, sasend, salen, argv[3], 0) < 0 ){
		fprintf(stderr,"mcast_join() error : %s\n", strerror(errno));
		return 1;
	}
	  
	mcast_set_loop(sendfd, 1);

	pthread_t send_thread, recv_thread;

	void *send_args[3] = { &sendfd, sasend, &salen };
	pthread_create(&recv_thread, NULL, recv_loop, &recvfd);
	pthread_create(&send_thread, NULL, send_loop, send_args);

	pthread_join(send_thread, NULL);

	send_all(sendfd, sasend, salen);	/* parent -> sends */
}
