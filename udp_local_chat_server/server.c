#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct _m
{
    char code; // 'l' login  's' chat  'q' quit
    char name[32];
    char txt[128];
} MSG;

typedef struct _NODE
{
    struct sockaddr_in c_addr;
    struct _NODE *next;
} node_t;

int _login(int sockfd, MSG msg, struct sockaddr_in caddr, node_t *phead)
{

    node_t *p = phead;
    while (p->next)
    {
        p = p->next;
        if (sendto(sockfd, &msg, sizeof(MSG), 0, (struct sockaddr *)&(p->c_addr), sizeof(p->c_addr)) == -1)
        {
        	perror("_login_sendto");                                        
        	exit(-1);                                             
        }
    }

    node_t *newp = (node_t *)malloc(sizeof(node_t));
    newp->c_addr = caddr;
    newp->next = phead->next;
    phead->next = newp;
    return 0;
}

int _chat(int sockfd, MSG msg, struct sockaddr_in caddr, node_t *phead)
{
    node_t *p = phead;
    while (p->next)
    {
        p = p->next;
        if (memcmp(&(p->c_addr), &caddr, sizeof(caddr)))
        {
            if (sendto(sockfd, &msg, sizeof(MSG), 0, (struct sockaddr *)&(p->c_addr), sizeof(p->c_addr)) == -1)
            {
        		perror("_chat_sendto");                                        
        		exit(-1);                                             
            }
        }
    }
    return 0;
}

int _quit(int sockfd, MSG msg, struct sockaddr_in caddr, node_t *phead)
{
    node_t *p = phead;
    while (p->next)
    {
        if (memcmp(&(p->next->c_addr), &caddr, sizeof(caddr)))
        {
            p = p->next;
            if (sendto(sockfd, &msg, sizeof(MSG), 0, (struct sockaddr *)&(p->c_addr), sizeof(p->c_addr)) == -1)
            {
        		perror("_quit_sendto");                                        
        		exit(-1);                                           
            }
        }
        else
        {
            node_t *pnew;
            pnew = p->next;
            p->next = pnew->next;
            pnew->next = NULL;
            free(pnew);
            pnew = NULL;
        }
    }
    return 0;
}

int main(int argc, const char *argv[])
{
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        		perror("socket");                                      
        		exit(-1);  
    }
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr("192.168.12.157");
    saddr.sin_port = htons(8888);
    socklen_t saddr_len = sizeof(saddr);
    if (bind(sockfd, (struct sockaddr *)&saddr, saddr_len) == -1)
    {
        perror("bind");                                        
        exit(-1);                                           
    }
    struct sockaddr_in caddr;
    memset(&caddr, 0, sizeof(caddr));
    socklen_t caddr_len = sizeof(caddr);
    MSG msg;

    pid_t pid;
    pid = fork();
    if (pid == -1)
    {
        perror("fork");                                        \
        exit(-1);                                           
    }
    else if (pid == 0)
    {
        node_t *phead = (node_t *)malloc(sizeof(node_t));
        phead->next = NULL;

        while (1)
        {
            memset(&msg, 0, sizeof(msg));
            memset(&caddr, 0, sizeof(caddr));
            if ((recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&caddr, &caddr_len)) == -1)
            {
        		perror("recvfrom");                                      
        		exit(-1);  
            }
            printf("%s : [%s]\n", msg.name, msg.txt);
            switch (msg.code)
            {
            case 'l':
                _login(sockfd, msg, caddr, phead);
                break;
            case 's':
                _chat(sockfd, msg, caddr, phead);
                break;
            case 'q':
                _quit(sockfd, msg, caddr, phead);
                break;
            }
        }
    }
    else
    {
        msg.code = 's';
        strcpy(msg.name,"server");
        while(1)
        {
            fgets(msg.txt,128,stdin);
            msg.txt[strlen(msg.txt)-1]='\0';
            if(sendto(sockfd,&msg,sizeof(MSG),0,(struct sockaddr *)&saddr,saddr_len)==-1)
            {
        		perror("sendto");                                        
        		exit(-1);                                           
            }
        }
    }
    close(sockfd);
    return 0;
}




