#include <stdio.h>
#include <stdlib.h>
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


#define SHELL 's'

char buff[256];
int to_child_pipe[2];
int from_child_pipe[2];
pid_t child_pid;
struct termios save_mode;
struct pollfd fd[2];
int shell_flag = 0;

void sigPipehandler(int signum)
{
    if (signum == SIGPIPE)
    {
        kill(child_pid, SIGINT);
        exit(0);
    }
}


void set_shell()
{
    close(to_child_pipe[1]);
    close(from_child_pipe[0]);
    dup2(to_child_pipe[0], 0);
    dup2(from_child_pipe[1], 1);
    dup2(from_child_pipe[1], 2);
    close(to_child_pipe[0]);
    close(from_child_pipe[1]);

}

void set_pipe(int fd[2])
{
    if(pipe(fd) == -1)
    {
        fprintf(stderr, "error creating the pipe.\n");
        exit(1);
    }
}
void restore_mode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &save_mode);
    if (shell_flag)
    {
        int status = 0;
        if (waitpid(child_pid, &status, 0) == -1)
        {
            fprintf(stderr, "waitpaid fail");
            exit(1);
        }
        if (WIFEXITED(status))
        {
            fprintf(stderr, "SHELL EXIT SIGNAL=%d, STATUS=%d\n", WIFSIGNALED(status), WEXITSTATUS(status));
            exit(0);
        }
        
    }
}



int main(int argc, char * argv[]) {
    static struct option long_options[] =
    {
        {"shell", no_argument, 0, 's'},
        {0,0,0,0}
    };
    int ret = 0;
    while ((ret = getopt_long(argc, argv, "s", long_options, NULL)) != -1)
    {
        switch (ret)
        {
            case 's':
                signal(SIGPIPE, sigPipehandler);
                shell_flag = 1;
                break;
            default:
                fprintf(stderr, "Error: Unrecognized argument.\nUsage: ./lab1a [--shell]\n");
                exit(1);
        }
    }
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
    set_pipe(to_child_pipe);
    set_pipe(from_child_pipe);
    if (shell_flag)
    {
        child_pid = fork();
        if (child_pid == -1)
        {
            fprintf(stderr, "error forking child process.\n");
            exit(1);
        }
        if (child_pid == 0)
        {
            set_shell();
            char *execvp_argv[2];
            char execvp_filename[] = "/bin/bash";
            execvp_argv[0] = execvp_filename;
            execvp_argv[1] = NULL;
            if(execvp(execvp_filename, execvp_argv) == -1)
            {
                fprintf(stderr, "error setting up the shell.\n");
                exit(1);
            }
        }
        else
        {
            close(to_child_pipe[0]);
            close(from_child_pipe[1]);
            fd[0].fd = STDIN_FILENO;
            fd[0].events = POLLIN | POLLHUP | POLLERR;
            fd[1].fd = from_child_pipe[0];
            fd[1].events = POLLIN | POLLHUP | POLLERR;
            while (1)
            {
                int retp = poll(fd, 2, 0);
                if (retp < 0)
                {
                    fprintf(stderr, "error in polling.\n");
                    exit(1);
                }
                ssize_t readb;
                int i;
                if (fd[0].revents & POLLIN)
                {
                    readb = read(STDIN_FILENO, buff, 256);
                    i = 0;
                    while (i < readb)
                    {
                        if (buff[i] == 0x03)
                        {
                            kill(child_pid, SIGINT);
                            close(to_child_pipe[1]);
                            exit(0); 
                        }
                        else if (buff[i] == 0x04)
                        {
                            close(to_child_pipe[1]);
                            exit(0);
                        }
                        else if (buff[i] == '\n' || buff[i] == '\r')
                        {
                            char zixuan[2] = {'\r', '\n'};
                            char xinnan = '\n';
                            write(STDOUT_FILENO, zixuan, 2);
                            write(to_child_pipe[1], &xinnan, 1);
                            i += 1;
                        }
                        else
                        {
                            write(to_child_pipe[1], &buff[i], 1);
                            write(STDOUT_FILENO, &buff[i], 1);
                            i += 1;
                        }
                     }
                }
                if (fd[1].revents & POLLIN)
                {
                    readb = read(from_child_pipe[0], buff, 256);
                    i = 0;
                    while (i < readb)
                    {
                        if (buff[i] == 0x04)
                        {
                          exit(0); 
                        }
                        
                        else if (buff[i] == '\n' || buff[i] == '\r')
                        {
                            char zixuan[2] = {'\r', '\n'};
                            write(STDOUT_FILENO, zixuan, 2);
                            i += 1;
                        }
                        else
                        {
                            write(STDOUT_FILENO, &buff[i], 1);
                            i += 1;
                        }
                    }
                    
                   }
                if ((fd[1].revents & (POLLHUP | POLLERR)))
                {
                    exit(0);
                }
            }
            
            
            
        }
    }
    else
    {
        char temp;
        while(read(0, &temp, 1))
        {
            if (temp == '\004')
            {
                exit(0);
            }
            else if (temp == '\n' || temp == '\r')
            {
                char zixuan[2] = {'\r', '\n'};
                write(STDOUT_FILENO, zixuan, 2);
                continue;
            }
            else
            {
                write(STDOUT_FILENO, &temp, 1);
            }
           
        }
        
    }
    exit(0);
}

