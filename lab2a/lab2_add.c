#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <string.h>
#include <time.h>

#define NONE 0
#define MUTEX 1
#define SPIN_LOCK 2
#define COMPARE_AND_SWAP 3

int n_threads = 1;
int n_iterations = 1;
int opt_yield = 0;
int aixinnan = NONE;
long long counter = 0;
pthread_mutex_t m_zixuan = PTHREAD_MUTEX_INITIALIZER;
int spinl = 0;

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
}

void* spec_add()
{
    int i;
    for (i = 0; i < n_iterations; i++)
    {
        if (aixinnan == NONE)
        {
            add(&counter, 1);
        }
        else if (aixinnan == MUTEX)
        {
            pthread_mutex_lock(&m_zixuan);
            add(&counter, 1);
            pthread_mutex_unlock(&m_zixuan);
        }
        else if (aixinnan == SPIN_LOCK)
        {
            while(__sync_lock_test_and_set(&spinl, 1));
            add(&counter, 1);
            __sync_lock_release(&spinl);
        }
        else if(aixinnan == COMPARE_AND_SWAP)
        {
            long long prev, next;
            do
            {
                prev = counter;
                next = prev + 1;
                if (opt_yield) {sched_yield();}
            }while (__sync_val_compare_and_swap(&counter, prev, next) != prev);
                    
        }
    }
    
    for (i = 0; i < n_iterations; i++)
    {
        if (aixinnan == NONE)
        {
            add(&counter, -1);
        }
        else if (aixinnan == MUTEX)
        {
            pthread_mutex_lock(&m_zixuan);
            add(&counter, -1);
            pthread_mutex_unlock(&m_zixuan);
        }
        else if (aixinnan == SPIN_LOCK)
        {
            while(__sync_lock_test_and_set(&spinl, 1));
            add(&counter, -1);
            __sync_lock_release(&spinl);
        }
        else if(aixinnan == COMPARE_AND_SWAP)
        {
            long long prev, next;
            do
            {
                prev = counter;
                next = prev - 1;
                if (opt_yield) {sched_yield();}
            }while (__sync_val_compare_and_swap(&counter, prev, next) != prev);
            
        }
    }
    
    return NULL;
    
}

int main(int argc, char * argv[]) {
    static struct option long_options[] =
    {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", no_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    int ret = 0;
    while((ret = getopt_long(argc, argv, "", long_options, NULL)) != -1)
    {
        switch (ret)
        {
            case 't':
                n_threads = atoi(optarg);
                break;
            case 'i':
                n_iterations = atoi(optarg);
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                if (optarg[0] == 'm')
                {
                    aixinnan = MUTEX;
                }
                else if (optarg[0] == 's')
                {
                    aixinnan = SPIN_LOCK;
                }
                else if (optarg[0] == 'c')
                {
                    aixinnan = COMPARE_AND_SWAP;
                }
                else
                {
                    fprintf(stderr, "unrecognized arguments for sync. \n");
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "unrecognized arguments. Usage: lab2_add --threads=# --iterations=# --yield --sync=[msc]\n");
                exit(1);
            }
      }
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
    {
        fprintf(stderr, "error starting a timer. \n");
        exit(1);
    }
    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    int i;
    for(i = 0; i < n_threads; i++)
    {
        if(pthread_create(threads + i, NULL, spec_add, NULL))
        {
            fprintf(stderr, "error creating threads. \n");
            exit(1);
        }
    }
    for (i = 0; i < n_threads; i++)
    {
        if (pthread_join(*(threads + i), NULL))
        {
            fprintf(stderr, "error: error in joining threads. \n");
            exit(1);
        }
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) == -1)
    {
        fprintf(stderr, "eror stopping the timer. \n");
        exit(1);
    }
    long long n_operations = n_threads * n_iterations * 2;
    long long total_run_time = 1000000000L * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
    long long avg_peropt = total_run_time / n_operations;
    
    if (opt_yield)
    {
        if (aixinnan == NONE)
        {
            fprintf(stdout, "add-yield-none,");
        }
        else if(aixinnan == MUTEX)
        {
            fprintf(stdout, "add-yield-m,");
        }
        else if(aixinnan == SPIN_LOCK)
        {
            fprintf(stdout, "add-yield-s,");
        }
        else if(aixinnan == COMPARE_AND_SWAP)
        {
            fprintf(stdout, "add-yield-c,");
        }
    }
    else
    {
        if (aixinnan == NONE)
        {
            fprintf(stdout, "add-none,");
        }
        else if(aixinnan == MUTEX)
        {
            fprintf(stdout, "add-m,");
        }
        else if(aixinnan == SPIN_LOCK)
        {
            fprintf(stdout, "add-s,");
        }
        else if(aixinnan == COMPARE_AND_SWAP)
        {
            fprintf(stdout, "add-c,");
        }
    }
    fprintf(stdout, "%d,%d,%lld,%lld,%lld,%lld\n", n_threads, n_iterations, n_operations, total_run_time, avg_peropt, counter);
    exit(0);
}


