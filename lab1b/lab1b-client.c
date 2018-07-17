//NAME: Zixuan Wang
//EMAIL: zixuan14@g.ucla.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <zlib.h>

z_stream shell_in;
z_stream shell_out;
struct pollfd fd[2];
struct termios save_mode;
int log_fd = -1;

void setup_compress()
{
    shell_in.zalloc = Z_NULL;
    shell_in.zfree = Z_NULL;
    shell_in.opaque = Z_NULL;
    if (deflateInit(&shell_in, Z_DEFAULT_COMPRESSION)!= Z_OK)
    {
        fprintf(stderr, "error deflateInit: %s \n", strerror(errno));
        exit(1);
    }
}

void setup_decompress()
{
    shell_out.zalloc = Z_NULL;
    shell_out.zfree = Z_NULL;
    shell_out.opaque = Z_NULL;

    if (inflateInit(&shell_out) != Z_OK)
    {
        fprintf(stderr, "error inflateInit: %s \n", strerror(errno));
        exit(1);
    }
}


void restore_mode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &save_mode);
}

int main(int argc, char * argv[]) {
    static struct option long_options[] =
{
    {"port", required_argument, 0, 'p'},
    {"log", required_argument, 0, 'l'},
    {"compress", no_argument, 0, 'c'},
    {0,0,0,0}
};
    
    int port_n = 0;
    int port_flag = 0;
    int log_flag = 0;
    int comp_flag = 0;
    
    int ret = 0;
    while((ret = getopt_long(argc, argv, "", long_options, NULL)) != -1)
    {
        switch(ret)
        {
            case 'p':
                port_flag = 1;
                port_n = atoi(optarg);
                break;
            case 'l':
                log_flag = 1;
                log_fd = creat(optarg, 0666);
                if (log_fd < 0)
                {
                    fprintf(stderr, "Error in creating the file: %s \n", strerror(errno));
                    exit(1);
                }
                break;
            case 'c':
                comp_flag = 1;
                break;
            default:
                fprintf(stderr, "Error: Unrecognized argument.\n Usage: ./lab1b-client --port=# [--log=filename] [--compress]\n");
                exit(1);
        }
    }
    if (port_flag != 1 || port_n < 0)
    {
        fprintf(stderr, "invalid port number.\n");
        exit(1);
    }
    
   
    
    int sockfd;
    
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "error opening socket.\n");
        exit(1);
    }
    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr,"error, no such host\n");
        exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port_n);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "error connecting the client.\n");
        exit(1);
    }
    
       
        
    fd[0].fd = STDIN_FILENO;
    fd[0].events = POLLIN;
    fd[1].fd = sockfd;
    fd[1].events = POLLIN;

    struct termios aixinnan;
    if (!isatty(STDIN_FILENO))
    {
        fprintf(stderr, "Stdin: Not a terminal.\n");
        exit(1);
    }
    tcgetattr(STDIN_FILENO, &save_mode);
    atexit(restore_mode);
    tcgetattr(STDIN_FILENO, &aixinnan);
    aixinnan.c_iflag = ISTRIP;
    aixinnan.c_oflag = 0;
    aixinnan.c_lflag = 0;
    if(tcsetattr(STDIN_FILENO, TCSANOW, &aixinnan) < 0)
    {
        fprintf(stderr, "error setting terminal modes.\n");
        exit(1);
    }



    char buffer[2048];
    while(1)
    {
        int ai = poll(fd, 2, 0);
        if (ai < 0)
        {
            fprintf(stderr, "error polling: %s \n", strerror(errno));
            exit(1);
        }
        if (fd[0].revents & POLLIN)
        {
            
            ssize_t bytes = read(STDIN_FILENO, buffer, 2048);
            if (bytes < 0)
            {
                fprintf(stderr, "error reading: %s \n", strerror(errno));
                exit(1);
            }
            int i;
            for (i = 0; i < bytes; i++)
            {
                if (buffer[i] == '\n' || buffer[i] == 'r')
                    {
                        char zixuan[2] = {'\r', '\n'};
                        write(STDOUT_FILENO, zixuan, 2);
                    }
                else
                {
                    write(STDOUT_FILENO, buffer + i, 1);
                }
            }
            
            for (i = 0; i < bytes; i++)
            {
                if (buffer[i] == '\r')
                {
                    buffer[i] = '\n';
                }
            }
            
            if (comp_flag == 1)
            {
                setup_compress();
                char temp[2048];
                memcpy(temp, buffer, bytes);
                shell_in.avail_in = bytes;
                shell_in.next_in = (Bytef *)buffer;
                shell_in.avail_out = 2048;
                shell_in.next_out = (Bytef *)temp;
                
                do
                {
                    deflate(&shell_in, Z_SYNC_FLUSH);
                }while (shell_in.avail_in > 0);
                bytes = 2048 - shell_in.avail_out;
            }
            if (write(fd[1].fd, buffer, bytes) < 0)
            {
                fprintf(stderr, "error writing: %s \n", strerror(errno));
                exit(1);
            }
            if (log_flag == 1 && log_fd >= 0)
            {
                if(dprintf(log_fd, "SENT %zd bytes: ", bytes) < 0)
                {
                    fprintf(stderr, "error printing: %s \n", strerror(errno));
                    exit(1);
                }
                if(write(log_fd, buffer, bytes) < 0)
                {
                    fprintf(stderr, "error printing: %s \n", strerror(errno));
                    exit(1);
                }
                if(dprintf(log_fd, "\n") < 0)
                {
                    fprintf(stderr, "error printing: %s \n", strerror(errno));
                    exit(1);
                }
            }
        }
        if (fd[1].revents & POLLIN)
        {
            ssize_t bytes = read(STDIN_FILENO, buffer, 2048);
            if (bytes < 0)
            {
                fprintf(stderr, "error reading: %s \n", strerror(errno));
                exit(1);
            }
            if (bytes == 0)
            {
                exit(0);
            }
            if (log_flag == 1 && log_fd >= 0)
            {
                if(dprintf(log_fd, "RECEIVED %zd bytes: ", bytes) < 0)
                {
                    fprintf(stderr, "error printing: %s \n", strerror(errno));
                    exit(1);
                }
                if(write(log_fd, buffer, bytes) < 0)
                {
                    fprintf(stderr, "error printing: %s \n", strerror(errno));
                    exit(1);
                }
                if(dprintf(log_fd, "\n") < 0)
                {
                    fprintf(stderr, "error printing: %s \n", strerror(errno));
                    exit(1);
                }
            }
            
            if (comp_flag == 1)
            {
                setup_decompress();
                char temp[2048];
                memcpy(temp, buffer, bytes);
                shell_out.avail_in = bytes;
                shell_out.next_in = (Bytef *)buffer;
                shell_out.avail_out = 2048;
                shell_out.next_out = (Bytef *)temp;
                
                do
                {
                    inflate(&shell_out, Z_SYNC_FLUSH);
                }while (shell_out.avail_in > 0);
                bytes = 2048 - shell_out.avail_out;
                
            }
            int k;
            for (k = 0; k < bytes; k++)
            {
                if (buffer[k] == '\n')
                {
                    char zixuan[2] = {'\r', '\n'};
                    write(STDOUT_FILENO, zixuan, 2);
                }
                else
                {
                    write(STDOUT_FILENO, buffer + k, 1);
                    
                }
            }
        }
                
        
        if ((fd[1].revents & (POLLHUP | POLLERR)))
        {
            exit(0);
        }
        
    }
    exit(0);
        
}

