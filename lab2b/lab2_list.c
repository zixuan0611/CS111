#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define NONE 0
#define MUTEX 1
#define SPIN_LOCK 2

int length = 0;
int n_ele;
int n_threads = 1;
int n_iterations = 1;
int n_lists = 1;
int opt_yield = 0;
int aixinnan = NONE;
long long counter = 0;
long long l_time = 0;
pthread_mutex_t* m_zixuan;
int* spinl;

int* listnum;

SortedList_t* listA;
SortedListElement_t* ele;

void signal_handler(int signum)
{
    if(signum ==SIGSEGV)
    {
        fprintf(stderr, "Segmentation fault. \n");
        exit(2);
    }
}
void initRandomkeys(int num)
{
    srand(time(NULL));
    int i;
    for (i = 0; i < num; i++)
    {
        char* rkey = malloc(2*sizeof(char));
        int rnum = rand()%26;
        rkey[0] = 'a' + rnum;
        rkey[1] = '\0';
        ele[i].key = rkey;
    }
}

int hash(const char* k)
{
    return (k[0] % n_lists);
}

void* listop(void* index)
{
    struct timespec l_start, l_end;
    int num = *((int*)index);
    int i;
    for(i = num; i < n_ele; i+=n_threads)
    {
        if (aixinnan == NONE)
        {
            SortedList_insert(&listA[listnum[i]], &ele[i]);
        }
        else if (aixinnan == MUTEX)
        {
            if (clock_gettime(CLOCK_MONOTONIC, &l_start) == -1)
            {
                fprintf(stderr, "error starting a timer. \n");
                exit (1);
            }
            pthread_mutex_lock(&m_zixuan[listnum[i]]);
            
            if (clock_gettime(CLOCK_MONOTONIC, &l_end) == -1)
            {
                fprintf(stderr, "error stopping a timer. \n");
                exit (1);
            }
            l_time += 1000000000L * (l_end.tv_sec - l_start.tv_sec) + (l_end.tv_nsec - l_start.tv_nsec);
            SortedList_insert(&listA[listnum[i]], &ele[i]);
            pthread_mutex_unlock(&m_zixuan[listnum[i]]);
        }
        else if (aixinnan == SPIN_LOCK)
        {
            if (clock_gettime(CLOCK_MONOTONIC, &l_start) == -1)
            {
                fprintf(stderr, "error starting a timer. \n");
                exit (1);
            }
            while(__sync_lock_test_and_set(&spinl[listnum[i]], 1));
            if (clock_gettime(CLOCK_MONOTONIC, &l_end) == -1)
            {
                fprintf(stderr, "error stopping a timer. \n");
                exit (1);
            }
            l_time += 1000000000L * (l_end.tv_sec - l_start.tv_sec) + (l_end.tv_nsec - l_start.tv_nsec);
            SortedList_insert(&listA[listnum[i]], &ele[i]);
            __sync_lock_release(&spinl[listnum[i]]);
        }
    }
    
    int list_length = 0;
    
    if (aixinnan == NONE)
    {
        for (i = 0; i < n_lists; i++)
     {
         int temp = SortedList_length(&listA[i]);
         if (temp == -1)
         {
            fprintf(stderr, "error getting the length of the list. \n");
            exit(2);
         }
         list_length += temp;
     }
    }
    else if (aixinnan == MUTEX)
    {
        for (i = 0; i < n_lists; i++)
        {
            if (clock_gettime(CLOCK_MONOTONIC, &l_start) == -1)
            {
                fprintf(stderr, "error starting a timer. \n");
                exit (1);
            }
            pthread_mutex_lock(&m_zixuan[listnum[i]]);
            
            if (clock_gettime(CLOCK_MONOTONIC, &l_end) == -1)
            {
                fprintf(stderr, "error stopping a timer. \n");
                exit (1);
            }
            l_time += 1000000000L * (l_end.tv_sec - l_start.tv_sec) + (l_end.tv_nsec - l_start.tv_nsec);
            int temp = SortedList_length(&listA[i]);
            if (temp == -1)
            {
                fprintf(stderr, "error getting the length of the list. \n");
                exit(2);
            }
            list_length += temp;
            pthread_mutex_unlock(&m_zixuan[listnum[i]]);
        }
    }
    else if (aixinnan == SPIN_LOCK)
    {
        for (i = 0; i < n_lists; i++)
        {
            if (clock_gettime(CLOCK_MONOTONIC, &l_start) == -1)
            {
                fprintf(stderr, "error starting a timer. \n");
                exit (1);
            }
            while(__sync_lock_test_and_set(&spinl[listnum[i]], 1));
            if (clock_gettime(CLOCK_MONOTONIC, &l_end) == -1)
            {
                fprintf(stderr, "error stopping a timer. \n");
                exit (1);
            }
            l_time += 1000000000L * (l_end.tv_sec - l_start.tv_sec) + (l_end.tv_nsec - l_start.tv_nsec);
            int temp = SortedList_length(&listA[i]);
            if (temp == -1)
            {
                fprintf(stderr, "error getting the length of the list. \n");
                exit(2);
            }
            list_length += temp;
            __sync_lock_release(&spinl[listnum[i]]);
        }
    }
    
    SortedListElement_t *temp;
    for (i = num; i < n_ele; i+=n_threads)
    {
        if (aixinnan == NONE)
        {
            temp = SortedList_lookup(&listA[listnum[i]], ele[i].key);
            if (temp == NULL)
            {
                fprintf(stderr, "error looking up the element. \n");
                exit(2);
            }
            if (SortedList_delete(temp))
            {
                fprintf(stderr, "error deleting the element. \n");
                exit(2);
            }
        }
        
        else if (aixinnan == MUTEX)
        {
            if (clock_gettime(CLOCK_MONOTONIC, &l_start) == -1)
            {
                fprintf(stderr, "error starting a timer. \n");
                exit (1);
            }
            pthread_mutex_lock(&m_zixuan[listnum[i]]);
            
            if (clock_gettime(CLOCK_MONOTONIC, &l_end) == -1)
            {
                fprintf(stderr, "error stopping a timer. \n");
                exit (1);
            }
            l_time += 1000000000L * (l_end.tv_sec - l_start.tv_sec) + (l_end.tv_nsec - l_start.tv_nsec);
            temp = SortedList_lookup(&listA[listnum[i]], ele[i].key);
            if (temp == NULL)
            {
                fprintf(stderr, "error looking up the element. \n");
                exit(2);
            }
            if (SortedList_delete(temp))
            {
                fprintf(stderr, "error deleting the element. \n");
                exit(2);
            }
            pthread_mutex_unlock(&m_zixuan[listnum[i]]);
        }
        else if (aixinnan == SPIN_LOCK)
        {
            if (clock_gettime(CLOCK_MONOTONIC, &l_start) == -1)
            {
                fprintf(stderr, "error starting a timer. \n");
                exit (1);
            }
            while(__sync_lock_test_and_set(&spinl[listnum[i]], 1));
            if (clock_gettime(CLOCK_MONOTONIC, &l_end) == -1)
            {
                fprintf(stderr, "error stopping a timer. \n");
                exit (1);
            }
            l_time += 1000000000L * (l_end.tv_sec - l_start.tv_sec) + (l_end.tv_nsec - l_start.tv_nsec);
            temp = SortedList_lookup(&listA[listnum[i]], ele[i].key);
            if (temp == NULL)
            {
                fprintf(stderr, "error looking up the element. \n");
                exit(2);
            }
            if (SortedList_delete(temp))
            {
                fprintf(stderr, "error deleting the element. \n");
                exit(2);
            }
            __sync_lock_release(&spinl[listnum[i]]);
        }
    }
    
    return NULL;
}


int main(int argc, char * argv[]) {
    static struct option long_options[] =
    {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {"lists", required_argument, 0, 'l'},
        {0, 0, 0, 0}
    };
    int ret = 0;
    char* y_option;
    while((ret = getopt_long(argc, argv, "", long_options, NULL)) != -1)
    {
        switch (ret){
            case 't':
                n_threads = atoi(optarg);
                break;
            case 'i':
                n_iterations = atoi(optarg);
                break;
            case 'y':
                y_option = optarg;
                if (strlen(y_option) > 3)
                {
                    fprintf(stderr, "Unrecognized option for yield. \n");
                    exit(1);
                }
                unsigned int i;
                for (i = 0; i < strlen(y_option); i ++)
                {
                    switch (y_option[i])
                    {
                        case 'i':
                            opt_yield |= INSERT_YIELD;
                            break;
                        case 'd':
                            opt_yield |= DELETE_YIELD;
                            break;
                        case 'l':
                            opt_yield |= LOOKUP_YIELD;
                            break;
                        default:
                            fprintf(stderr, "Unrecognized option for yield. \n");
                            exit(1);
                    }
                }
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
                else
                {
                    fprintf(stderr, "unrecognized arguments for sync. \n");
                    exit(1);
                }
                break;
            case 'l':
                n_lists = atoi(optarg);
                break;
            default:
                fprintf(stderr, "unrecognized arguments. Usage: lab2_list --threads=# --iterations=# --yield[idl] --sync=[ms]\n");
                exit(1);
        }
    }
    
    signal(SIGSEGV, signal_handler);
    
    listA = malloc(n_lists * sizeof(SortedList_t));
    int a;
    for (a = 0; a < n_lists; a++)
    {
        listA[a].key = NULL;
        listA[a].prev = &listA[a];
        listA[a].next = &listA[a];
    }
    
    n_ele = n_threads * n_iterations;
    ele = malloc(n_ele * sizeof(SortedListElement_t));
    
    if (aixinnan == MUTEX)
    {
        m_zixuan = malloc(n_lists * sizeof(pthread_mutex_t));
        int b;
        for (b = 0; b < n_lists; b++)
        {
            pthread_mutex_init(&m_zixuan[b], NULL);
        }
    }
    if (aixinnan == SPIN_LOCK)
    {
        spinl = malloc(n_lists * sizeof(int));
        int c;
        for (c = 0; c < n_lists; c++)
        {
            spinl[c] = 0;
        }
    }
    
    
    initRandomkeys(n_ele);
    
    listnum = malloc(n_ele * sizeof(int));
    int n;
    for (n = 0; n < n_ele; n++)
    {
        listnum[n] = hash(ele[n].key);
    }
    
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
    {
        fprintf(stderr, "error starting a timer. \n");
        exit(1);
    }
    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    int* threads_id = malloc(n_threads * sizeof(int));
    int i;
    for(i = 0; i < n_threads; i++)
    {
        threads_id[i] = i;
        if(pthread_create(threads + i, NULL, listop, &threads_id[i]))
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
    
    length = 0;
    int d;
    for (d = 0; d < n_lists; d++)
    {
        length += SortedList_length(&listA[d]);
    }
    
    if (length != 0)
    {
        fprintf(stderr, "error! length is not zero. \n");
        exit(2);
    }
    
    long long n_operations = n_threads * n_iterations * 3;
    long long total_run_time = 1000000000L * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
    long long avg_peropt = total_run_time / n_operations;
    l_time = l_time / n_operations;
    fprintf(stdout, "list-");
    if (opt_yield == 0)
    {
        fprintf(stdout, "none");
    }
    if(opt_yield & INSERT_YIELD)
    {
        fprintf(stdout, "i");
    }
    if (opt_yield & DELETE_YIELD)
    {
        fprintf(stdout, "d");
    }
    if (opt_yield & LOOKUP_YIELD)
    {
        fprintf(stdout, "l");
    }
    
    if(aixinnan == NONE)
    {
        fprintf(stdout, "-none,");
    }
    else if(aixinnan == MUTEX)
    {
        fprintf(stdout, "-m,");
    }
    else if(aixinnan == SPIN_LOCK)
    {
        fprintf(stdout, "-s,");
    }
    
    fprintf(stdout, "%d,%d,%d,%lld,%lld,%lld,%lld\n", n_threads, n_iterations, n_lists, n_operations, total_run_time, avg_peropt, l_time);
    
    free(m_zixuan);
    free(ele);
    free(threads);
    
    exit(0);
    
}



