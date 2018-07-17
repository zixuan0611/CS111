//
//  main.c
//  project 4C
//
//  Created by Zixuan Wang on 6/10/18.
//  Copyright © 2018 Zoe. All rights reserved.
//
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <math.h>
#include <mraa.h>

#define C_SCALE 0
#define F_SCALE 1

int sockfd;
char *logfile = NULL;
char* id;
char* host;
FILE* log_fd = 0;
int log_flag = 0;
int scale = 0;
int period = 1;
int port = 0;
int stop = 0;
mraa_aio_context temp_context;

double get_temp(int temp)
{
    double tem = 1023.0 / ((double)temp) - 1.0;
    tem = 100000 * tem;
    double c = 1.0/(log(tem/100000)/4275+1/298.15)-273.15;
    if (scale == C_SCALE)
    {
        return c;
    }
    else
    {
        return c * 9/5 +32;
    }
}


int main(int argc, char * argv[]) {
    int opt;
    static struct option long_options[]=
    {
        {"period", required_argument, 0, 'p'},
        {"scale", required_argument, 0, 's'},
        {"log", required_argument, 0, 'l'},
        {"id", required_argument, 0, 'i'},
        {"host", required_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    while(1)
    {
        int option_index = 0;
        opt = getopt_long(argc, argv, "", long_options, &option_index);
        if (opt == -1)
        {
            break;
        }
        switch(opt)
        {
            case 'p':
                period = atoi(optarg);
                if (period == 0)
                {
                    fprintf(stderr, "illegal period. \n");
                    exit(1);
                }
                break;
            case 's':
                if (optarg[0] == 'C')
                    scale = C_SCALE;
                else if (optarg[0] == 'F')
                    scale = F_SCALE;
                else
                {
                    fprintf(stderr, "illegal scale argument. \n");
                    exit(1);
                }
                break;
            case 'l':
                log_flag = 1;
                logfile = optarg;
                log_fd = fopen(logfile, "w");
                break;
            case 'i':
                id = optarg;
                break;
            case 'h':
                host = optarg;
                break;
            case '?':
            default:
                fprintf(stderr, "unrecognized arguments. \n");
                exit(1);
        }
    }
    port = atoi(argv[argc - 1]);
    if (port == 0)
    {
        fprintf(stderr, "invalid port number. \n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "error openning socket. \n");
        exit(2);
    }
    struct hostent *server = gethostbyname(host);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host as %s\n", host);
        exit(2);
    }
    struct sockaddr_in server_address;
    memset((char *) &server_address, 0, sizeof(server_address));
    memcpy((char *) &server_address.sin_addr.s_addr,
           (char*) server->h_addr,
           server->h_length);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (connect(sockfd , (struct sockaddr *)&server_address , sizeof(server_address)) < 0)
    {
        fprintf(stderr, "ERROR connecting. \n");
        exit(2);
    }
    temp_context = mraa_aio_init(1);
    
    dprintf(sockfd, "ID=%s\n", id);
    if (log_flag == 1)
    {
        fprintf(log_fd, "ID=%s\n", id);
        fflush(log_fd);
    }
    
    struct pollfd pfds[1];
    pfds[0].fd = sockfd;
    pfds[0].events = POLLIN;
    struct timeval cur_time;
    time_t next_time = 0;
    struct tm *now;
    double temp;
    char buffer[200];
    ssize_t readn;
    
    while (1)
    {
        gettimeofday(&cur_time, 0);
        if (!stop && cur_time.tv_sec >= next_time)
        {
            now = localtime(&(cur_time.tv_sec));
            int aixinnan = mraa_aio_read(temp_context);
            temp = get_temp(aixinnan);
            dprintf(sockfd, "%02d:%02d:%02d %.1f\n", now->tm_hour, now->tm_min, now->tm_sec, temp);
            if (log_flag ==1)
            {
                fprintf(log_fd, "%02d:%02d:%02d %.1f\n", now->tm_hour, now->tm_min, now->tm_sec, temp);
                fflush(log_fd);
            }
            
            next_time = cur_time.tv_sec + period;
        }
        if (poll(pfds, 1, 0) > 0)
        {
            if (pfds[0].revents & POLLIN){
                readn = read(pfds[0].fd, buffer, 200);
                if (readn < 0)
                {
                    fprintf(stderr, "read error. \n");
                    exit(2);
                }
            
            else if (readn == 0)
              {
                gettimeofday(&cur_time, 0);
                now = localtime(&(cur_time.tv_sec));
                dprintf(sockfd, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);
                if (log_flag ==1)
                {
                    fprintf(log_fd, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);
                    fflush(log_fd);
                }
                
                mraa_aio_close(temp_context);
                exit(0);
              }
             else
             {
                char *comm = strtok(buffer, "\n");
                while (comm != NULL)
                {
                    if (strcmp(comm, "STOP") == 0)
                    {
                        stop = 1;
                    }
                    else if (strcmp(comm, "START") == 0)
                    {
                        stop = 0;
                    }
                    else if (strcmp(comm, "SCALE=F") == 0)
                    {
                        scale = F_SCALE;
                    }
                    else if (strcmp(comm, "SCALE=C") == 0)
                    {
                        scale = C_SCALE;
                    }
                    else if (strcmp(comm, "OFF") == 0)
                    {
                        now = localtime(&(cur_time.tv_sec));
                        dprintf(sockfd, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);
                        if (log_flag == 1)
                        {
                            fprintf(log_fd, "%s\n", comm);
                            fprintf(log_fd, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);
                            fflush(log_fd);
                        }
                        mraa_aio_close(temp_context);
                        exit(0);
                    }
                    else if (strstr(comm, "PERIOD")!=NULL)
                    {
                        period = atoi(strchr(comm, '=') + 1);
                    }
                    if (log_flag == 1)
                    {
                        fprintf(log_fd, "%s\n", comm);
                        fflush(log_fd);
                    }
                    
                    comm = strtok(NULL, "\n");
                }
                
            }
        }
    }
 }
    
    mraa_aio_close(temp_context);
    return 0;
    
}
