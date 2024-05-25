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
#include <signal.h>
#include <sys/wait.h>

typedef struct _m
{
    char code; //'l' µÇÂ¼  's' ÈºÁÄ  'q' ÍË³ö
    char name[32];
    char txt[128];
}MSG;

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
    MSG msg;
    memset(&msg, 0, sizeof(MSG));
    msg.code = 'l';
    printf("============== client login ==============\n");
    printf("input name>>>");
    fgets(msg.name, 32, stdin);
    msg.name[strlen(msg.name) - 1] = '\0';

    strcpy(msg.txt, "login successfully");
    if (sendto(sockfd, &msg, sizeof(MSG), 0, (struct sockaddr *)&saddr, saddr_len) == -1)
    {
        perror("sendto");                                      
    	exit(-1);  
    }
    pid_t pid;
    pid = fork();
    if (pid == -1)
    {
    	perror("fork");                                      
        exit(-1);  
    }
    else if (pid == 0)
    {
        while (1)
        {
            memset(&msg, 0, sizeof(msg));
            if (recvfrom(sockfd, &msg, sizeof(MSG), 0, NULL, NULL) == -1)
            {
        		perror("recvfrom");                                      
        		exit(-1);  
            }
            printf("%s:[%s]\n", msg.name, msg.txt);
        }
    }
    else if (pid > 0)
    {
        while (1)
        {   
            msg.code = 's';
            fgets(msg.txt, 128, stdin);
            msg.txt[strlen(msg.txt) - 1] = '\0';
            if (strcmp(msg.txt, "quit") == 0)
            {
                msg.code = 'q';
                strcpy(msg.txt, "is leave");
            }
            if (sendto(sockfd, &msg, sizeof(MSG), 0, (struct sockaddr *)&saddr, saddr_len) == -1)
            {
                perror("sendto");
                exit(-1);
            }
            if (strcmp(msg.txt, "is leave") == 0)
            {
                break;
            }
        }
        kill(pid,SIGKILL);
        wait(NULL);
        close(sockfd);
    }
    return 0;
}
