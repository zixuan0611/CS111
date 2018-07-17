#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include<sys/time.h>
#include<time.h>
#include <mraa.h>
#include <mraa/gpio.h>

#define C_SCALE 0
#define F_SCALE 1
#define SHUTDOWN 1

int scale = 0;
int period = 1;
int logFlag = 0;
FILE *logfile = 0;
int shut_down = 0;
int stop = 0;
int commandlog=0;

mraa_aio_context sensor;
mraa_gpio_context button;

void do_shutdown(FILE *logfile)
{
     shut_down = 1;
     char tstring[10];
     time_t timer;
     struct tm* infotime;
     time(&timer);
     infotime = localtime(&timer);
     strftime(tstring, 10, "%H:%M:%S", infotime);
     fprintf(stdout, "%s SHUTDOWN\n", tstring);
     if (logFlag == 1)
     {
        fprintf(logfile, "%s SHUTDOWN\n", tstring);
	fflush(logfile);
     }
     mraa_aio_close(sensor);
     mraa_gpio_close(button);
     exit(0);
}

double get_temp()
{
      int temp = mraa_aio_read(sensor);
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
    struct option long_options[] =
    {
           {"period", required_argument, 0, 'p'},
	   {"scale", required_argument, 0, 's'},
	   {"log", required_argument, 0, 'l'},
	   {0, 0, 0, 0}
    };
    int ret = 0;
    while ((ret = getopt_long(argc, argv, "", long_options, 0)) != -1)
    {
          switch(ret)
	  {
	      case 'p':
	      period = atoi(optarg);
	      if (period == 0)
	      {
	         fprintf(stderr, "illegal period.\n");
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
	      logFlag = 1;
	      logfile = fopen(optarg, "w");
	      break;
	      default:
	      fprintf(stderr, "unrecognized argument. \n");
	      exit(1);
	  }
       }

       sensor = mraa_aio_init(1);
       button = mraa_gpio_init(60);
       mraa_gpio_dir(button, MRAA_GPIO_IN);
       
       struct pollfd fds[2];
       fds[0].fd = STDIN_FILENO;
       fds[0].revents = POLLHUP & POLLERR;
       fds[0].events = POLLIN;

       char tstring[10];
       time_t timer, start, end;
       struct tm* infotime;



       while(1)
       {
          int opt;
	  double ntemp = get_temp();
	  time(&timer);
	  infotime = localtime(&timer);
	  strftime(tstring, 10, "%H:%M:%S", infotime);
	  fprintf(stdout, "%s %.1f\n", tstring, ntemp);
	  if (logFlag == 1)
	  {
	     fprintf(logfile, "%s %.1f\n", tstring, ntemp);
	  }
	  fflush(logfile);
	  time(&start);
	  time(&end);
	  while (difftime(end, start) < period)
	  {
	     if (mraa_gpio_read(button))
	     {
	        do_shutdown(logfile);
	     }
	     opt = poll(fds, 1, 0);
	     if (opt < 0)
	     {
	        fprintf(stderr, "error polling.\n");
		exit(1);
	     }
	     if ((fds[0].revents & POLLIN))
	     {
	        char comm[256];
                memset(comm, 0, 256);
		int m;
		m = read(STDIN_FILENO, &comm, 256);
		if (m < 0)
		{
                   fprintf(stderr, "error reading.\n");
		   exit(1);
		}
		if (m == 0)
		{
                   do_shutdown(logfile);
	        }
		if (comm[0]=='S'&&comm[1]=='C'&&comm[2]=='A'&&comm[3]=='L'&&comm[4]=='E'&&comm[5]=='='&&comm[6]=='F')
		{
		   scale = F_SCALE;
		   if (logFlag == 1)
		   {
		      fprintf(logfile, "%s\n", comm);
		      fflush(logfile);
		   }
		 }
		 else if (comm[0]=='S'&&comm[1]=='C'&&comm[2]=='A'&&comm[3]=='L'&&comm[4]=='E'&&comm[5]=='='&&comm[6]=='C')
		 {
		    scale = C_SCALE;
		    if (logFlag == 1)
		    {
		       fprintf(logfile, "%s\n", comm);
		       fflush(logfile);
		    }
		 }
		 else if (comm[0]=='S'&&comm[1]=='T'&&comm[2]=='O'&&comm[3]=='P')
		 {
		     stop = 1;
		     if (logFlag == 1)
		     {
		        fprintf(logfile, "%s\n", comm);
			fflush(logfile);
		     }
		 }
		 else if (comm[0]=='S'&&comm[1]=='T'&&comm[2]=='A'&&comm[3]=='R'&&comm[4]=='T')
		 {
		     stop = 0;
		     if (logFlag == 1)
		     {
		        fprintf(logfile, "%s\n", comm);
			fflush(logfile);
		     }
		 }
		 else if(comm[0]=='O'&&comm[1]=='F'&&comm[2]=='F')
		 {
		     if (logFlag == 1)
		     {
		        fprintf(logfile, "%s\n", comm);
			fflush(logfile);
	             }
		     do_shutdown(logfile);
		 }
		 else if(comm[0]=='P'&&comm[1]=='E'&&comm[2]=='R'&&comm[3]=='I'&&comm[4]=='O'&&comm[5]=='D'&&comm[6]=='=')
		 {
		      int temperiod = atoi(comm+7);
		      if (temperiod < 1)
		      {
		          fprintf(stderr, "invalid period. \n");
			  exit(1);
		      }
		      period = temperiod;
		      if (logFlag == 1)
		      {
		         fprintf(logfile, "%s\n", comm);
			 fflush(logfile);
		      }
		  }
		 else if(comm[0]=='L'&&comm[1]=='O'&&comm[2]=='G')
                 {
		      
		      if (logFlag == 1)
		      {
		         
			 commandlog=1;
			 fprintf(logfile, "LOG\n");
			 fflush(logfile);
		      }
		  }
		  else
		  {
		     if (commandlog ==1)
		     {
		         int i = 0;
                         while(comm[i] != '\0')
			 {
			    fprintf(logfile, "%c", comm[i]);
			    i++;
			 }
			 fprintf(logfile, "\n");
			 fflush(logfile);
			 commandlog = 0;
		     }
		     else
		     fprintf(stderr, "invalid command.\n");
		    
		  }
		}
		  if (stop == 0)
		  {
		     time(&end);
		  }
	     }
	  }
	  exit(0);
}


