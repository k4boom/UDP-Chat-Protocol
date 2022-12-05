/*
2 ** talker.c -- a datagram "client" demo
3 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERPORT "4950" // the port users will be connecting to
#define MAXLINE 1024
#define MAXMSGSIZE 15
#define MAXPKGNO 100
struct packet
{
    char message[MAXMSGSIZE];
    char seq_no;
};

struct packet prepare_package(char *buffer, int *seq)
{
    struct packet pck;
    pck.seq_no = *(seq);
    *seq = pck.seq_no + 1;
    strncpy(pck.message, buffer, MAXMSGSIZE);
    buffer = buffer + MAXMSGSIZE;
    return pck;
}

struct pcknode
{
    struct packet pkg;
    struct pcknode *next;
};

void freeList(struct pcknode *head)
{
    struct pcknode *tmp;

    while (head != NULL)
    {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

/*
Parse the buffer to different packages and put them into a linked list
Return the number of packages
*/
int prepare_packages(char *buffer, int *seq, struct pcknode *packages)
{
    int len = strlen(buffer);
    int current = 0, pckglen = 0;
    struct pcknode *currNode;
    currNode = packages;

    while (current < len - 1)
    {
        int msglen = MAXMSGSIZE;
        struct packet pck;
        pck.seq_no = *(seq);
        *seq = pck.seq_no + 1;
        if (strlen(buffer) < MAXMSGSIZE)
            msglen = strlen(buffer);

        printf("Msglen: %d\n", msglen);

        strncpy(pck.message, buffer, msglen);
        pck.message[msglen] = '\0';
        buffer = buffer + msglen;
        printf("Message: %s, Buffer: %s\n", pck.message, buffer);

        currNode->pkg = pck;
        printf("Package assigned\n");

        if (msglen == MAXMSGSIZE)
        { // if equals it is not the last package to be created
            currNode->next = malloc(sizeof(struct pcknode));
            currNode = currNode->next;
        }
        else
        {
            currNode->next = NULL;
            //printf("elseMALLOC\n");
        }

        pckglen++;
        current += msglen;
    }

    return pckglen;
}

int main(int argc, char *argv[])
{
    // struct packet pkg;
    int newLineCounter = 0;
    int sequenceNo = 0;
    int sockfd;
    char buffer[MAXLINE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_in servaddr;
    struct pcknode *pkgs, *looppkg;
    pkgs = malloc(sizeof(struct pcknode));
    struct packet queue[MAXPKGNO];



    if (argc != 3)
    {
        fprintf(stderr, "usage: talker hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST;

    if ((rv = getaddrinfo("172.24.0.10", SERVERPORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    while (1)
    {
        //memset(buffer, '\0', strlen(buffer));
        memset(buffer, '\0', MAXLINE);

        scanf("%[^\n]%*c", buffer);
        if (strlen(buffer) == 0)
        { // if new line, increment and check counter
            if (++newLineCounter > 2)
            {
                exit(1);
            }
            continue;
        }

        int num = prepare_packages(buffer, &sequenceNo, pkgs);
        printf("Packages are ready\n");

        for (looppkg = pkgs; looppkg != NULL; looppkg = looppkg->next)
        {
            struct packet pkg = looppkg->pkg;
            printf("pkgsize : %lu pkg message: %s . seqNo: %d", sizeof(pkg), pkg.message, pkg.seq_no);

            if ((numbytes = sendto(sockfd, &pkg, sizeof(pkg), 0,
                                   p->ai_addr, p->ai_addrlen)) == -1)
            {
                perror("talker: sendto");
                exit(1);
            }

            printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
        }

    }

    freeList(pkgs); // free prepare_packages linked list
    freeaddrinfo(servinfo);

    close(sockfd);

    return 0;
}
