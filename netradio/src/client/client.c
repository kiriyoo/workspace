#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <net/if.h>

#include <proto.h>
#include "client.h"

/*
 *  -M --mgroup 指定多波组
 *  -P --port   指定接收端口
 *  -p --player 指定播放器
 *  -H --help   显示帮助
 */

struct client_conf_st client_conf = {
    .rcvport = DEFAULT_RCVPORT,
    .mgroup = DEFAULT_MGROUP,
    .player_cmd = DEFAULT_PLAYERCMD};

static void print_help(void)
{
    printf("-M --mgroup 指定多波组\n \
            -P --port   指定接收端口\n \
            -p --player 指定播放器\n \
            -H --help   显示帮助\n");
}

static ssize_t writen(int fd, const uint8_t *buf,int len)
{
    int pos = 0;
    int ret = 0;
    while (1)
    {
        ret = writen(fd,buf+pos,len);
        if (pos < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("writen()");
            return -1;
        }
        pos += ret;
        len -= ret;
        if (len <= 0)
        {
            return 1;
        }
    }
}

int main(int argc, char **argv)
{
    int sd;
    int val;
    int pipefd[2];
    pid_t pid;
    struct sockaddr_in laddr,serveraddr,raddr;
    socklen_t serveraddr_len,raddr_len;
    struct ip_mreqn mreq;

    int index = 0;
    struct option opt[] = {{"prot", 1, NULL, 'P'}, {"mgroup", 1, NULL, 'M'}, {"player", 1, NULL, 'p'}, {"help", 0, NULL, 'H'}, {NULL, 0, NULL, '\0'}};

    char c;
    while (1)
    {
        c = getopt_long(argc, argv, "-M:P:p:H", opt, &index);
        if (c < 0)
        {
            break;
        }

        switch (c)
        {
        case 'P':
            client_conf.rcvport = optarg;
            break;
        case 'M':
            client_conf.mgroup = optarg;
            break;
        case 'p':
            client_conf.player_cmd = optarg;
            break;
        case 'H':
            print_help();
            exit(0);
            break;
        default:
            abort();
            break;
        }
    }

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("socket()");
        exit(1);
    }

    inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_address);
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_multiaddr);
    // 指定网卡设置
    mreq.imr_ifindex = if_nametoindex("lo");
    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }

    val = 1;
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (void *)&val, sizeof(val)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }

    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(client_conf.rcvport));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    if (bind(sd, (void *)&laddr, sizeof(laddr)) < 0)
    {
        perror("bind()");
        exit(1);
    }

    if (pipe(pipefd) < 0)
    {
        perror("pipe()");
        exit(1);
    }

    pid = fork();
    if (pid < 0)
    {
        perror("fork()");
        exit(1);
    }
    if (pid == 0) // child
    {
        close(pipefd[1]);
        close(sd);
        dup2(pipefd[0], fileno(stdin));
        if (pipefd[0] > 0)
        {
            close(pipefd[0]);
        }
        execl(client_conf.player_cmd, "-", NULL);
        perror("execl()");
        exit(1);
    }
    else // parent
    {
        close(pipefd[0]);
        close(sd);

        struct msg_list_st *msg_list;
        int len;
        int chosenid;
        int ret;

        msg_list = malloc(MAX_LIST_MAX);
        if (msg_list == NULL)
        {
            perror("malloc()");
            exit(1);
        }

        // serveraddr.sin_family = AF_INET;
        // serveraddr.sin_port = ;
        // serveraddr.sin_addr = ;
        while (1)
        {
            len = recvfrom(sd, msg_list, MAX_LIST_MAX,0,(void *)&serveraddr,&serveraddr_len);
            if(len < sizeof(struct msg_list_st))
            {
                fprintf(stderr,"message is too small.\n");
                continue;
            }
            if(msg_list->chnid != LISTCHNID)
            {
                fprintf(stderr,"chnid is not match.\n");
                continue;
            }
            break;
        }
        
        struct msg_listentry_st *pos;
        for (pos = msg_list->entry; (char *)pos < (((char *)msg_list) + len); pos = (void *)(((char *)pos) + ntohs(pos->len)))
        {
            printf("channel %d:%s\n",pos->chnid,pos->desc);
        }
        free(msg_list);
        while(1)
        {
            ret = scanf("%d",&chosenid);
            if (ret != 1)
            {
                printf("Please enter a valid integer.\n");
                while(getchar() != '\n');
                continue;
            }
            break;
        }

        
        struct msg_channel_st *msg_channel;

        msg_channel = malloc(MSG_CHANNEL_MAX);
        if (msg_channel == NULL)
        {
            perror("malloc()");
            exit(1);
        }
        
        while (1)
        {
            len = recvfrom(sd, msg_channel, MSG_CHANNEL_MAX,0,(void *)&raddr,&raddr_len);
            if(raddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr || raddr.sin_port != serveraddr.sin_port)
            {
                fprintf(stderr,"Ignore:address not match.");
                continue;
            }
            if (len < sizeof(struct msg_channel_st))
            {
                fprintf(stderr,"Ignore:message too small.");
                continue;
            }
            if(msg_channel->chnid == chosenid)
            {
                fprintf(stdout,"Accept msg:%d recieved.\n",msg_channel->chnid);
                if(writen(pipefd[1],msg_channel->data,len-sizeof(chnid_t))<0)
                {
                    exit(1);
                }
            }
            
        }

        free(msg_channel);
        
        close(sd);
        exit(0);
    }
}