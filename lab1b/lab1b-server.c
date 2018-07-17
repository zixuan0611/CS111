//Name: Zixuan Wang
//EMAIL:zixuan14@g.ucla.edu

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

struct termios save_mode;
struct pollfd fd[2];
char buffer[2048];

z_stream client_in;
z_stream client_out;
int to_child_pipe[2];
int from_child_pipe[2];
pid_t child_pid;

void finish_mode()
{
        int status = 0;
        if (waitpid(0, &status, 0) == -1)
        {
            fprintf(stderr, "waitpaid fail");
            exit(1);
        }
        
        
            fprintf(stderr, "SHELL EXIT SIGNAL=%d, STATUS=%d\n", WIFSIGNALED(status), WEXITSTATUS(status));
            exit(0);
        
        
}

void sigPipehandler(int signum)
{
    if (signum == SIGPIPE)
    {
        kill(child_pid, SIGINT);
        finish_mode();
        exit(0);
    }
}

void setup_compress()
{
    client_out.zalloc = Z_NULL;
    client_out.zfree = Z_NULL;
    client_out.opaque = Z_NULL;
    if (deflateInit(&client_out, Z_DEFAULT_COMPRESSION)!= Z_OK)
    {
        fprintf(stderr, "error deflateInit: %s \n", strerror(errno));
        exit(1);
    }
}

void setup_decompress()
{
    client_in.zalloc = Z_NULL;
    client_in.zfree = Z_NULL;
    client_in.opaque = Z_NULL;
    
    if (inflateInit(&client_in) != Z_OK)
    {
        fprintf(stderr, "error inflateInit: %s \n", strerror(errno));
        exit(1);
    }
}




int main(int argc, char * argv[]) {
    
    static struct option long_options[] =
    {
        {"port", required_argument, 0, 'p'},
        {"compress", no_argument, 0, 'c'},
        {0,0,0,0}
    };
    int port_n = 0;
    int port_flag = 0;
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
    signal(SIGPIPE, sigPipehandler);
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "error opening socket.\n");
        exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_n);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "error on binding. \n");
        exit(1);
    }
    
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
    {
        fprintf(stderr, "error on accept");
        exit(1);
    }
    
    if (pipe(to_child_pipe) == -1)
    {
        fprintf(stderr, "error pipe to_child_pipe. \n");
        close(newsockfd);
        close(sockfd);
        exit(1);
    }
    
    if (pipe(from_child_pipe) == -1)
    {
        fprintf(stderr, "error pipe from_child_pipe. \n");
        close(newsockfd);
        close(sockfd);
        exit(1);
    }
    
    child_pid = fork();
    
    if (child_pid == 0)
    {
        close(to_child_pipe[1]);
        close(from_child_pipe[0]);
        dup2(to_child_pipe[0], 0);
        dup2(from_child_pipe[1], 1);
        dup2(from_child_pipe[1], 2);
        close(to_child_pipe[0]);
        close(from_child_pipe[1]);
        char *execvp_argv[2];
        char execvp_filename[] = "/bin/bash";
        execvp_argv[0] = execvp_filename;
        execvp_argv[1] = NULL;
        if(execvp(execvp_filename, execvp_argv) == -1)
        {
            fprintf(stderr, "error setting up the shell.\n");
            close(newsockfd);
            close(sockfd);
            exit(1);
        }
    }
    
    
    else if (child_pid > 0)
    {
        close(to_child_pipe[0]);
        close(from_child_pipe[1]);
        fd[0].fd = newsockfd;
        fd[0].events = POLLIN;
        fd[1].fd = from_child_pipe[0];
        fd[1].events = POLLIN;
        
        signal(SIGPIPE, sigPipehandler);
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
                
                if (comp_flag == 1)
                {
                    setup_decompress();
                    char temp[2048];
                    memcpy(temp, buffer, bytes);
                    client_in.avail_in = bytes;
                    client_in.next_in = (Bytef *)buffer;
                    client_in.avail_out = 2048;
                    client_in.next_out = (Bytef *)temp;
                    
                    do
                    {
                        inflate(&client_in, Z_SYNC_FLUSH);
                    }while (client_in.avail_in > 0);
                    bytes = 2048 - client_in.avail_out;
                    
                }
                
                int i;
                for (i = 0; i < bytes; i++)
                {
                    if (buffer[i] == '\n' || buffer[i] == 'r')
                    {
                        char zixuan = '\n';
                        write(to_child_pipe[1], &zixuan, 1);
                    }
                    if (buffer[i] == 0x03)
                    {
                        kill(child_pid, SIGINT);
                    }
                    if (buffer[i] == 0x04)
                    {
                        close(to_child_pipe[1]);
                    }
                    else
                    {
                        if (write(to_child_pipe[1], buffer + i, 1) < 0)
                        {
                            fprintf(stderr, "error writing to pipe.\n");
                            exit(1);
                        }
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
                
                int k;
                for (k = 0; k < bytes; k++)
                {
                    if (buffer[k] == 0x04)
                    {
                        finish_mode();
                        exit(0);
                    }
                }
                if (comp_flag == 1)
                {
                    setup_compress();
                    char temp[2048];
                    memcpy(temp, buffer, bytes);
                    client_out.avail_in = bytes;
                    client_out.next_in = (Bytef *)buffer;
                    client_out.avail_out = 2048;
                    client_out.next_out = (Bytef *)temp;
                    
                    do
                    {
                        deflate(&client_out, Z_SYNC_FLUSH);
                    }while (client_out.avail_in > 0);
                    bytes = 2048 - client_out.avail_out;
                    
                }
                if (write(newsockfd, buffer, bytes) < 0)
                {
                    fprintf(stderr, "error printing: %s \n", strerror(errno));
                    exit(1);
                }
                
            }
            
            
            if ((fd[1].revents & (POLLHUP | POLLERR)))
            {
                finish_mode();
                exit(0);
            }
        }
    }
    else
    {
       fprintf(stderr, "error fork.\n");
       exit(1);
    }
    close(newsockfd);
    close(sockfd);
    finish_mode();
    exit(0);
}

