#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define INPUT 'i'
#define OUTPUT 'o'
#define SEGFAULT 's'
#define CATCH 'c'

void handler(int signum){
    fprintf(stderr, "Segmentation fault. Signal number: %d received.\n",signum);
    exit(4);
}

int main(int argc, char * argv[]) {
    struct option long_options[] =
    {
        {"input", required_argument, 0, INPUT},
        {"output", required_argument, 0, OUTPUT},
        {"segfault", no_argument, 0, SEGFAULT},
        {"catch", no_argument, 0, CATCH},
        {0, 0, 0, 0}
    };
    char* inputFile = NULL;
    char* outputFile =NULL;
    int flag = 0;
    int ret = 0;
    int fd0, fd1;
    while ((ret = getopt_long(argc, argv, "", long_options, 0)) != -1)
    {
        switch (ret)
        {
            case INPUT:
                inputFile = optarg;
                fd0 = open(inputFile, O_RDONLY);
                if (fd0 < 0)
                {
                    fprintf(stderr, "Error --INPUT: %s\n cannot open the input file %s.\n", strerror(errno), inputFile);
                    exit(2);
                }
                else
                {
                    close(0);
                    dup2(fd0, STDIN_FILENO);
                    close(fd0);
                }
                break;
            case OUTPUT:
                outputFile = optarg;
                fd1 = creat(outputFile, 0644);
                if (fd1 < 0)
                {
                    fprintf(stderr, "Error --OUTPUT: %s\n cannot create the output file %s.\n", strerror(errno), outputFile);
                    exit(3);
                }
                else
                {
                    close(1);
                    dup2(fd1, STDOUT_FILENO);
                    close(fd1);
                }
                break;
            case SEGFAULT:
                flag = 1;
                break;
            case CATCH:
                signal(SIGSEGV, handler);
                break;
            default:
                fprintf(stderr, "unrecognized argument: %s: Usage: ./lab0 [--input=filename] [--output=filename] [--segfault] [--catch] \n", strerror(errno));
                exit(1);
        }
     }
    char* p = NULL;
    if (flag)
    {
        *p = 'A';
    }
    ssize_t temp;
    char buff[2001];
    while ((temp = read(STDIN_FILENO, &buff, 2000)) > 0)
    {
        write(STDOUT_FILENO, &buff, temp);
    }
    exit(0);
}

