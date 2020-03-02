/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/*  Integer radix sort of non-negative integers.                         */
/*                                                                       */
/*  Command line options:                                                */
/*                                                                       */
/*  -pP : P = number of processors.                                      */
/*  -rR : R = radix for sorting.  Must be power of 2.                    */
/*  -nN : N = number of keys to sort.                                    */
/*  -mM : M = maximum key value.  Integer keys k will be generated such  */
/*        that 0 <= k <= M.                                              */
/*  -s  : Print individual processor timing statistics.                  */
/*  -t  : Check to make sure all keys are sorted correctly.              */
/*  -o  : Print out sorted keys.                                         */
/*  -h  : Print out command line options.                                */
/*                                                                       */
/*  Default: RADIX -p1 -n262144 -r1024 -m524288                          */
/*                                                                       */
/*  Note: This version works under both the FORK and SPROC models        */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>

#define DEFAULT_P                    1
#define DEFAULT_N               262144
#define DEFAULT_R                 1024 
#define DEFAULT_M               524288
#define MAX_PROCESSORS              64    
#define RADIX_S                8388608.0e0
#define RADIX           70368744177664.0e0
#define SEED                 314159265.0e0
#define RATIO               1220703125.0e0
#define PAGE_SIZE                 4096
#define PAGE_MASK     (~(PAGE_SIZE-1))
#define MAX_RADIX                 4096



#include <pthread.h>

#include <sys/time.h>

#include <unistd.h>

#include <stdlib.h>

#define MAX_THREADS 32

pthread_t PThreadTable[MAX_THREADS];



struct prefix_node {
   int32_t densities[MAX_RADIX];
   int32_t ranks[MAX_RADIX];
   

struct {

	pthread_mutex_t	Mutex;

	pthread_cond_t	CondVar;

	uint32_t	Flag;

} done;


   char pad[PAGE_SIZE];
};

struct global_memory {
   int32_t Index;                             /* process ID */
   pthread_mutex_t (lock_Index);                    /* for fetch and add to get ID */
   pthread_mutex_t (rank_lock);                     /* for fetch and add to get ID */
/*   pthread_mutex_t section_lock[MAX_PROCESSORS];*/  /* key locks */
   

struct {

	pthread_mutex_t	mutex;

	pthread_cond_t	cv;

	uint32_t	counter;

	uint32_t	cycle;

} (barrier_rank);

                   /* for ranking process */
   

struct {

	pthread_mutex_t	mutex;

	pthread_cond_t	cv;

	uint32_t	counter;

	uint32_t	cycle;

} (barrier_key);

                    /* for key sorting process */
   double *ranktime;
   double *sorttime;
   double *totaltime;
   int32_t final;
   uint32_t starttime;
   uint32_t rs;
   uint32_t rf;
   struct prefix_node prefix_tree[2 * MAX_PROCESSORS];
} *global;

struct global_private {
  char pad[PAGE_SIZE];
  int32_t *rank_ff;         /* overall processor ranks */
} gp[MAX_PROCESSORS];

int32_t *key[2];            /* sort from one index into the other */
int32_t **rank_me;          /* individual processor ranks */
int32_t *key_partition;     /* keys a processor works on */
int32_t *rank_partition;    /* ranks a processor works on */

int32_t number_of_processors = DEFAULT_P;
int32_t max_num_digits;
int32_t radix = DEFAULT_R;
int32_t num_keys = DEFAULT_N;
int32_t max_key = DEFAULT_M;
int32_t log2_radix;
int32_t log2_keys;
int32_t dostats = 0;
int32_t test_result = 0;
int32_t doprint = 0;

void slave_sort(void);
double product_mod_46(double t1, double t2);
double ran_num_init(uint32_t k, double b, double t);
int32_t get_max_digits(int32_t max_key);
int32_t get_log2_radix(int32_t rad);
int32_t get_log2_keys(int32_t num_keys);
int32_t log_2(int32_t number);
void printerr(const char *s);
void init(int32_t key_start, int32_t key_stop, int32_t from);
void test_sort(int32_t final);
void printout(void);

int main(int argc, char *argv[])
{
   int32_t i;
   int32_t p;
   int32_t quotient;
   int32_t remainder;
   int32_t sum_i; 
   int32_t sum_f;
   int32_t size;
   int32_t **temp;
   int32_t **temp2;
   int32_t *a;
   int32_t c;
   int32_t Error;
   double mint, maxt, avgt;
   double minrank, maxrank, avgrank;
   double minsort, maxsort, avgsort;
   uint32_t start;


   {

	struct timeval	FullTime;



	gettimeofday(&FullTime, NULL);

	(start) = (uint32_t)(FullTime.tv_usec + FullTime.tv_sec * 1000000);

}

   while ((c = getopt(argc, argv, "p:r:n:m:stoh")) != -1) {
     switch(c) {
      case 'p': number_of_processors = atoi(optarg);
                if (number_of_processors < 1) {
                  printerr("P must be >= 1\n");
                  exit(-1);
                }
                if (number_of_processors > MAX_PROCESSORS) {
                  printerr("Maximum processors (MAX_PROCESSORS) exceeded\n"); 
		  exit(-1);
		}
                break;
      case 'r': radix = atoi(optarg);
                if (radix < 1) {
                  printerr("Radix must be a power of 2 greater than 0\n");
                  exit(-1);
                }
                log2_radix = log_2(radix);
                if (log2_radix == -1) {
                  printerr("Radix must be a power of 2\n");
                  exit(-1);
                }
                break;
      case 'n': num_keys = atoi(optarg);
                if (num_keys < 1) {
                  printerr("Number of keys must be >= 1\n");
                  exit(-1);
                }
                break;
      case 'm': max_key = atoi(optarg);
                if (max_key < 1) {
                  printerr("Maximum key must be >= 1\n");
                  exit(-1);
                }
                break;
      case 's': dostats = !dostats;
                break;
      case 't': test_result = !test_result;
                break;
      case 'o': doprint = !doprint;
                break;
      case 'h': printf("Usage: RADIX <options>\n\n");
                printf("   -pP : P = number of processors.\n");
                printf("   -rR : R = radix for sorting.  Must be power of 2.\n");
                printf("   -nN : N = number of keys to sort.\n");
                printf("   -mM : M = maximum key value.  Integer keys k will be generated such\n");
                printf("         that 0 <= k <= M.\n");
                printf("   -s  : Print individual processor timing statistics.\n");
                printf("   -t  : Check to make sure all keys are sorted correctly.\n");
                printf("   -o  : Print out sorted keys.\n");
                printf("   -h  : Print out command line options.\n\n");
                printf("Default: RADIX -p%1d -n%1d -r%1d -m%1d\n",
                        DEFAULT_P,DEFAULT_N,DEFAULT_R,DEFAULT_M);
		exit(0);
     }
   }

   {;}

   log2_radix = log_2(radix); 
   log2_keys = log_2(num_keys);
   global = (struct global_memory *) malloc(sizeof(struct global_memory));
   if (global == NULL) {
	   fprintf(stderr,"ERROR: Cannot malloc enough memory for global\n");
	   exit(-1);
   }
   key[0] = (int32_t *) malloc(num_keys*sizeof(int32_t));
   key[1] = (int32_t *) malloc(num_keys*sizeof(int32_t));
   key_partition = (int32_t *) malloc((number_of_processors+1)*sizeof(int32_t));
   rank_partition = (int32_t *) malloc((number_of_processors+1)*sizeof(int32_t));
   global->ranktime = (double *) malloc(number_of_processors*sizeof(double));
   global->sorttime = (double *) malloc(number_of_processors*sizeof(double));
   global->totaltime = (double *) malloc(number_of_processors*sizeof(double));
   size = number_of_processors*(radix*sizeof(int32_t)+sizeof(int32_t *));
   rank_me = (int32_t **) malloc(size);
   if ((key[0] == NULL) || (key[1] == NULL) || (key_partition == NULL) || (rank_partition == NULL) || 
       (global->ranktime == NULL) || (global->sorttime == NULL) || (global->totaltime == NULL) || (rank_me == NULL)) {
     fprintf(stderr,"ERROR: Cannot malloc enough memory\n");
     exit(-1); 
   }

   temp = rank_me;
   temp2 = temp + number_of_processors;
   a = (int32_t *) temp2;
   for (i=0;i<number_of_processors;i++) {
     *temp = (int32_t *) a;
     temp++;
     a += radix;
   }
   for (i=0;i<number_of_processors;i++) {
     gp[i].rank_ff = (int32_t *) malloc(radix*sizeof(int32_t)+PAGE_SIZE);
   }
   {pthread_mutex_init(&(global->lock_Index), NULL);}
   {pthread_mutex_init(&(global->rank_lock), NULL);}
/*   {

	uint32_t	i, Error;



	for (i = 0; i < MAX_PROCESSORS; i++) {

		Error = pthread_mutex_init(&global->section_lock[i], NULL);

		if (Error != 0) {

			printf("Error while initializing array of locks.\n");

			exit(-1);

		}

	}

}*/
	Error = pthread_mutex_init(&(global->barrier_rank).mutex, NULL);

	if (Error != 0) {

		printf("Error while initializing barrier.\n");

		exit(-1);

	}



	Error = pthread_cond_init(&(global->barrier_rank).cv, NULL);

	if (Error != 0) {

		printf("Error while initializing barrier.\n");

		pthread_mutex_destroy(&(global->barrier_rank).mutex);

		exit(-1);

	}



	(global->barrier_rank).counter = 0;

	(global->barrier_rank).cycle = 0;

	Error = pthread_mutex_init(&(global->barrier_key).mutex, NULL);

	if (Error != 0) {

		printf("Error while initializing barrier.\n");

		exit(-1);

	}



	Error = pthread_cond_init(&(global->barrier_key).cv, NULL);

	if (Error != 0) {

		printf("Error while initializing barrier.\n");

		pthread_mutex_destroy(&(global->barrier_key).mutex);

		exit(-1);

	}

	(global->barrier_key).counter = 0;

	(global->barrier_key).cycle = 0;

   
   for (i=0; i<2*number_of_processors; i++) {
     {

	pthread_mutex_init(&global->prefix_tree[i].done.Mutex, NULL);

	pthread_cond_init(&global->prefix_tree[i].done.CondVar, NULL);

	global->prefix_tree[i].done.Flag = 0;

}

;
   }

   global->Index = 0;
   max_num_digits = get_max_digits(max_key);
   printf("\n");
   printf("Integer Radix Sort\n");
   printf("     %d Keys\n",num_keys);
   printf("     %d Processors\n",number_of_processors);
   printf("     Radix = %d\n",radix);
   printf("     Max key = %d\n",max_key);
   printf("\n");

   quotient = num_keys / number_of_processors;
   remainder = num_keys % number_of_processors;
   sum_i = 0;
   sum_f = 0;
   p = 0;
   while (sum_i < num_keys) {
      key_partition[p] = sum_i;
      p++;
      sum_i = sum_i + quotient;
      sum_f = sum_f + remainder;
      sum_i = sum_i + sum_f / number_of_processors;
      sum_f = sum_f % number_of_processors;
   }
   key_partition[p] = num_keys;

   quotient = radix / number_of_processors;
   remainder = radix % number_of_processors;
   sum_i = 0;
   sum_f = 0;
   p = 0;
   while (sum_i < radix) {
      rank_partition[p] = sum_i;
      p++;
      sum_i = sum_i + quotient;
      sum_f = sum_f + remainder;
      sum_i = sum_i + sum_f / number_of_processors;
      sum_f = sum_f % number_of_processors;
   }
   rank_partition[p] = radix;

/* POSSIBLE ENHANCEMENT:  Here is where one might distribute the key,
   rank_me, rank, and gp data structures across physically 
   distributed memories as desired. 
   
   One way to place data is as follows:  
      
   for (i=0;i<number_of_processors;i++) {
     Place all addresses x such that:
       &(key[0][key_partition[i]]) <= x < &(key[0][key_partition[i+1]])
            on node i
       &(key[1][key_partition[i]]) <= x < &(key[1][key_partition[i+1]])
            on node i
       &(rank_me[i][0]) <= x < &(rank_me[i][radix-1]) on node i
       &(gp[i]) <= x < &(gp[i+1]) on node i
       &(gp[i].rank_ff[0]) <= x < &(gp[i].rank_ff[radix]) on node i
   }
   start_p = 0;
   i = 0;

   for (toffset = 0; toffset < number_of_processors; toffset ++) {
     offset = toffset;
     level = number_of_processors >> 1;
     base = number_of_processors;
     while ((offset & 0x1) != 0) {
       offset >>= 1;
       index = base + offset;
       Place all addresses x such that:
         &(global->prefix_tree[index]) <= x < 
              &(global->prefix_tree[index + 1]) on node toffset
       base += level;
       level >>= 1;
     }  
   }  */

   /* Fill the random-number array. */
   
	for (i = 0; i < (number_of_processors) - 1; i++) {

		Error = pthread_create(&PThreadTable[i], NULL, (void * (*)(void *))(slave_sort), NULL);

		if (Error != 0) {

			printf("Error in pthread_create().\n");

			exit(-1);

		}

	}



	slave_sort();

	for (i = 0; i < (number_of_processors) - 1; i++) {

		Error = pthread_join(PThreadTable[i], NULL);

		if (Error != 0) {

			printf("Error in pthread_join().\n");

			exit(-1);

		}

	}

   printf("\n");
   printf("                 PROCESS STATISTICS\n");
   printf("               Total            Rank            Sort\n");
   printf(" Proc          Time             Time            Time\n");
   printf("    0     %10.0f      %10.0f      %10.0f\n",
           global->totaltime[0],global->ranktime[0],
           global->sorttime[0]);
   if (dostats) {
     maxt = avgt = mint = global->totaltime[0];
     maxrank = avgrank = minrank = global->ranktime[0];
     maxsort = avgsort = minsort = global->sorttime[0];
     for (i=1; i<number_of_processors; i++) {
       if (global->totaltime[i] > maxt) {
         maxt = global->totaltime[i];
       }
       if (global->totaltime[i] < mint) {
         mint = global->totaltime[i];
       }
       if (global->ranktime[i] > maxrank) {
         maxrank = global->ranktime[i];
       }
       if (global->ranktime[i] < minrank) {
         minrank = global->ranktime[i];
       }
       if (global->sorttime[i] > maxsort) {
         maxsort = global->sorttime[i];
       }
       if (global->sorttime[i] < minsort) {
         minsort = global->sorttime[i];
       }
       avgt += global->totaltime[i];
       avgrank += global->ranktime[i];
       avgsort += global->sorttime[i];
     }
     avgt = avgt / number_of_processors;
     avgrank = avgrank / number_of_processors;
     avgsort = avgsort / number_of_processors;
     for (i=1; i<number_of_processors; i++) {
       printf("  %3d     %10.0f      %10.0f      %10.0f\n",
               i,global->totaltime[i],global->ranktime[i],
               global->sorttime[i]);
     }
     printf("  Avg     %10.0f      %10.0f      %10.0f\n",avgt,avgrank,avgsort);
     printf("  Min     %10.0f      %10.0f      %10.0f\n",mint,minrank,minsort);
     printf("  Max     %10.0f      %10.0f      %10.0f\n",maxt,maxrank,maxsort);
     printf("\n");
   }

   printf("\n");
   global->starttime = start;
   printf("                 TIMING INFORMATION\n");
   printf("Start time                        : %16u\n",
           global->starttime);
   printf("Initialization finish time        : %16u\n",
           global->rs);
   printf("Overall finish time               : %16u\n",
           global->rf);
   printf("Total time with initialization    : %16u\n",
           global->rf-global->starttime);
   printf("Total time without initialization : %16u\n",
           global->rf-global->rs);
   printf("\n");

   if (doprint) {
     printout();
   }
   if (test_result) {
     test_sort(global->final);  
   }
  
   {exit(0);};
}

void slave_sort()
{
   int32_t i;
   int32_t MyNum;
   int32_t this_key;
   int32_t tmp;
   int32_t loopnum;
   int32_t shiftnum;
   int32_t bb;
   int32_t my_key;
   int32_t key_start;
   int32_t key_stop;
   int32_t rank_start;
   int32_t rank_stop;
   int32_t from=0;
   int32_t to=1;
   int32_t *key_density;       /* individual processor key densities */
   uint32_t time1;
   uint32_t time2;
   uint32_t time3;
   uint32_t time4;
   uint32_t time5;
   uint32_t time6;
   double ranktime=0;
   double sorttime=0;
   int32_t *key_from;
   int32_t *key_to;
   int32_t *rank_me_mynum;
   int32_t *rank_ff_mynum;
   int32_t stats;
   struct prefix_node* n;
   struct prefix_node* r;
   struct prefix_node* l;
   struct prefix_node* my_node;
   struct prefix_node* their_node;
   int32_t index;
   int32_t level;
   int32_t base;
   int32_t offset;

   stats = dostats;

   {pthread_mutex_lock(&(global->lock_Index));}
     MyNum = global->Index;
     global->Index++;
   {pthread_mutex_unlock(&(global->lock_Index));}

   {;};
   {;};

/* POSSIBLE ENHANCEMENT:  Here is where one might pin processes to
   processors to avoid migration */

   key_density = (int32_t *) malloc(radix*sizeof(int32_t));

   /* Fill the random-number array. */

   key_start = key_partition[MyNum];
   key_stop = key_partition[MyNum + 1];
   rank_start = rank_partition[MyNum];
   rank_stop = rank_partition[MyNum + 1];

   if (rank_start == radix) {
       printf("WARNING: rank_start == radix!\n");
   }

   if (rank_stop == radix) {
     rank_stop--;
   }

   init(key_start,key_stop,from);

   {

	uint32_t	Error, Cycle;

	int32_t		Cancel, Temp;



	Error = pthread_mutex_lock(&(global->barrier_key).mutex);

	if (Error != 0) {

		printf("Error while trying to get lock in barrier.\n");

		exit(-1);

	}



	Cycle = (global->barrier_key).cycle;

	if (++(global->barrier_key).counter != (number_of_processors)) {

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);

		while (Cycle == (global->barrier_key).cycle) {

			Error = pthread_cond_wait(&(global->barrier_key).cv, &(global->barrier_key).mutex);

			if (Error != 0) {

				break;

			}

		}

		pthread_setcancelstate(Cancel, &Temp);

	} else {

		(global->barrier_key).cycle = !(global->barrier_key).cycle;

		(global->barrier_key).counter = 0;

		Error = pthread_cond_broadcast(&(global->barrier_key).cv);

	}

	pthread_mutex_unlock(&(global->barrier_key).mutex);

} 

/* POSSIBLE ENHANCEMENT:  Here is where one might reset the
   statistics that one is measuring about the parallel execution */

   {

	uint32_t	Error, Cycle;

	int32_t		Cancel, Temp;



	Error = pthread_mutex_lock(&(global->barrier_key).mutex);

	if (Error != 0) {

		printf("Error while trying to get lock in barrier.\n");

		exit(-1);

	}



	Cycle = (global->barrier_key).cycle;

	if (++(global->barrier_key).counter != (number_of_processors)) {

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);

		while (Cycle == (global->barrier_key).cycle) {

			Error = pthread_cond_wait(&(global->barrier_key).cv, &(global->barrier_key).mutex);

			if (Error != 0) {

				break;

			}

		}

		pthread_setcancelstate(Cancel, &Temp);

	} else {

		(global->barrier_key).cycle = !(global->barrier_key).cycle;

		(global->barrier_key).counter = 0;

		Error = pthread_cond_broadcast(&(global->barrier_key).cv);

	}

	pthread_mutex_unlock(&(global->barrier_key).mutex);

} 

   if ((MyNum == 0) || (stats)) {
     {

	struct timeval	FullTime;



	gettimeofday(&FullTime, NULL);

	(time1) = (uint32_t)(FullTime.tv_usec + FullTime.tv_sec * 1000000);

}
   }

/* Do 1 iteration per digit.  */

   rank_me_mynum = rank_me[MyNum];
   rank_ff_mynum = gp[MyNum].rank_ff;
   for (loopnum=0;loopnum<max_num_digits;loopnum++) {
     shiftnum = (loopnum * log2_radix);
     bb = (radix-1) << shiftnum;

/* generate histograms based on one digit */

     if ((MyNum == 0) || (stats)) {
       {

	struct timeval	FullTime;



	gettimeofday(&FullTime, NULL);

	(time2) = (uint32_t)(FullTime.tv_usec + FullTime.tv_sec * 1000000);

}
     }

     for (i = 0; i < radix; i++) {
       rank_me_mynum[i] = 0;
     }  
     key_from = (int32_t *) key[from];
     key_to = (int32_t *) key[to];
     for (i=key_start;i<key_stop;i++) {
       my_key = key_from[i] & bb;
       my_key = my_key >> shiftnum;  
       rank_me_mynum[my_key]++;
     }
     key_density[0] = rank_me_mynum[0]; 
     for (i=1;i<radix;i++) {
       key_density[i] = key_density[i-1] + rank_me_mynum[i];  
     }

     {

	uint32_t	Error, Cycle;

	int32_t		Cancel, Temp;



	Error = pthread_mutex_lock(&(global->barrier_rank).mutex);

	if (Error != 0) {

		printf("Error while trying to get lock in barrier.\n");

		exit(-1);

	}



	Cycle = (global->barrier_rank).cycle;

	if (++(global->barrier_rank).counter != (number_of_processors)) {

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);

		while (Cycle == (global->barrier_rank).cycle) {

			Error = pthread_cond_wait(&(global->barrier_rank).cv, &(global->barrier_rank).mutex);

			if (Error != 0) {

				break;

			}

		}

		pthread_setcancelstate(Cancel, &Temp);

	} else {

		(global->barrier_rank).cycle = !(global->barrier_rank).cycle;

		(global->barrier_rank).counter = 0;

		Error = pthread_cond_broadcast(&(global->barrier_rank).cv);

	}

	pthread_mutex_unlock(&(global->barrier_rank).mutex);

}  

     n = &(global->prefix_tree[MyNum]);
     for (i = 0; i < radix; i++) {
        n->densities[i] = key_density[i];
        n->ranks[i] = rank_me_mynum[i];
     }
     offset = MyNum;
     level = number_of_processors >> 1;
     base = number_of_processors;
     if ((MyNum & 0x1) == 0) {
        {

	pthread_mutex_lock(&global->prefix_tree[base + (offset >> 1)].done.Mutex);

	global->prefix_tree[base + (offset >> 1)].done.Flag = 1;

	pthread_cond_broadcast(&global->prefix_tree[base + (offset >> 1)].done.CondVar);

	pthread_mutex_unlock(&global->prefix_tree[base + (offset >> 1)].done.Mutex);}

;
     }
     while ((offset & 0x1) != 0) {
       offset >>= 1;
       r = n;
       l = n - 1;
       index = base + offset;
       n = &(global->prefix_tree[index]);
       {

	pthread_mutex_lock(&n->done.Mutex);

	if (n->done.Flag == 0) {

		pthread_cond_wait(&n->done.CondVar, &n->done.Mutex);

	}

};
       {

	n->done.Flag = 0;

	pthread_mutex_unlock(&n->done.Mutex);}

;
       if (offset != (level - 1)) {
         for (i = 0; i < radix; i++) {
           n->densities[i] = r->densities[i] + l->densities[i];
           n->ranks[i] = r->ranks[i] + l->ranks[i];
         }
       } else {
         for (i = 0; i < radix; i++) {
           n->densities[i] = r->densities[i] + l->densities[i];
         }
       }
       base += level;
       level >>= 1;
       if ((offset & 0x1) == 0) {
         {

	pthread_mutex_lock(&global->prefix_tree[base + (offset >> 1)].done.Mutex);

	global->prefix_tree[base + (offset >> 1)].done.Flag = 1;

	pthread_cond_broadcast(&global->prefix_tree[base + (offset >> 1)].done.CondVar);

	pthread_mutex_unlock(&global->prefix_tree[base + (offset >> 1)].done.Mutex);}

;
       }
     }
     {

	uint32_t	Error, Cycle;

	int32_t		Cancel, Temp;



	Error = pthread_mutex_lock(&(global->barrier_rank).mutex);

	if (Error != 0) {

		printf("Error while trying to get lock in barrier.\n");

		exit(-1);

	}



	Cycle = (global->barrier_rank).cycle;

	if (++(global->barrier_rank).counter != (number_of_processors)) {

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);

		while (Cycle == (global->barrier_rank).cycle) {

			Error = pthread_cond_wait(&(global->barrier_rank).cv, &(global->barrier_rank).mutex);

			if (Error != 0) {

				break;

			}

		}

		pthread_setcancelstate(Cancel, &Temp);

	} else {

		(global->barrier_rank).cycle = !(global->barrier_rank).cycle;

		(global->barrier_rank).counter = 0;

		Error = pthread_cond_broadcast(&(global->barrier_rank).cv);

	}

	pthread_mutex_unlock(&(global->barrier_rank).mutex);

};

     if (MyNum != (number_of_processors - 1)) {
       offset = MyNum;
       level = number_of_processors;
       base = 0;
       while ((offset & 0x1) != 0) {
         offset >>= 1;
         base += level;
         level >>= 1;
       }
       my_node = &(global->prefix_tree[base + offset]);
       offset >>= 1;
       base += level;
       level >>= 1;
       while ((offset & 0x1) != 0) {
         offset >>= 1;
         base += level;
         level >>= 1;
       }
       their_node = &(global->prefix_tree[base + offset]);
       {

	pthread_mutex_lock(&my_node->done.Mutex);

	if (my_node->done.Flag == 0) {

		pthread_cond_wait(&my_node->done.CondVar, &my_node->done.Mutex);

	}

};
       {

	my_node->done.Flag = 0;

	pthread_mutex_unlock(&my_node->done.Mutex);}

;
       for (i = 0; i < radix; i++) {
         my_node->densities[i] = their_node->densities[i];
       }
     } else {
       my_node = &(global->prefix_tree[(2 * number_of_processors) - 2]);
     }
     offset = MyNum;
     level = number_of_processors;
     base = 0;
     while ((offset & 0x1) != 0) {
       {

	pthread_mutex_lock(&global->prefix_tree[base + offset - 1].done.Mutex);

	global->prefix_tree[base + offset - 1].done.Flag = 1;

	pthread_cond_broadcast(&global->prefix_tree[base + offset - 1].done.CondVar);

	pthread_mutex_unlock(&global->prefix_tree[base + offset - 1].done.Mutex);}

;
       offset >>= 1;
       base += level;
       level >>= 1;
     }
     offset = MyNum;
     level = number_of_processors;
     base = 0;
     for(i = 0; i < radix; i++) {
       rank_ff_mynum[i] = 0;
     }
     while (offset != 0) {
       if ((offset & 0x1) != 0) {
         /* Add ranks of node to your left at this level */
         l = &(global->prefix_tree[base + offset - 1]);
         for (i = 0; i < radix; i++) {
           rank_ff_mynum[i] += l->ranks[i];
         }
       }
       base += level;
       level >>= 1;
       offset >>= 1;
     }
     for (i = 1; i < radix; i++) {
       rank_ff_mynum[i] += my_node->densities[i - 1];
     }

     if ((MyNum == 0) || (stats)) {
       {

	struct timeval	FullTime;



	gettimeofday(&FullTime, NULL);

	(time3) = (uint32_t)(FullTime.tv_usec + FullTime.tv_sec * 1000000);

};
     }

     {

	uint32_t	Error, Cycle;

	int32_t		Cancel, Temp;



	Error = pthread_mutex_lock(&(global->barrier_rank).mutex);

	if (Error != 0) {

		printf("Error while trying to get lock in barrier.\n");

		exit(-1);

	}



	Cycle = (global->barrier_rank).cycle;

	if (++(global->barrier_rank).counter != (number_of_processors)) {

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);

		while (Cycle == (global->barrier_rank).cycle) {

			Error = pthread_cond_wait(&(global->barrier_rank).cv, &(global->barrier_rank).mutex);

			if (Error != 0) {

				break;

			}

		}

		pthread_setcancelstate(Cancel, &Temp);

	} else {

		(global->barrier_rank).cycle = !(global->barrier_rank).cycle;

		(global->barrier_rank).counter = 0;

		Error = pthread_cond_broadcast(&(global->barrier_rank).cv);

	}

	pthread_mutex_unlock(&(global->barrier_rank).mutex);

};

     if ((MyNum == 0) || (stats)) {
       {

	struct timeval	FullTime;



	gettimeofday(&FullTime, NULL);

	(time4) = (uint32_t)(FullTime.tv_usec + FullTime.tv_sec * 1000000);

};
     }

     /* put it in order according to this digit */

     for (i = key_start; i < key_stop; i++) {  
       this_key = key_from[i] & bb;
       this_key = this_key >> shiftnum;  
       tmp = rank_ff_mynum[this_key];
       key_to[tmp] = key_from[i];
       rank_ff_mynum[this_key]++;
     }   /*  i */  

     if ((MyNum == 0) || (stats)) {
       {

	struct timeval	FullTime;



	gettimeofday(&FullTime, NULL);

	(time5) = (uint32_t)(FullTime.tv_usec + FullTime.tv_sec * 1000000);

};
     }

     if (loopnum != max_num_digits-1) {
       from = from ^ 0x1;
       to = to ^ 0x1;
     }

     {

	uint32_t	Error, Cycle;

	int32_t		Cancel, Temp;



	Error = pthread_mutex_lock(&(global->barrier_rank).mutex);

	if (Error != 0) {

		printf("Error while trying to get lock in barrier.\n");

		exit(-1);

	}



	Cycle = (global->barrier_rank).cycle;

	if (++(global->barrier_rank).counter != (number_of_processors)) {

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);

		while (Cycle == (global->barrier_rank).cycle) {

			Error = pthread_cond_wait(&(global->barrier_rank).cv, &(global->barrier_rank).mutex);

			if (Error != 0) {

				break;

			}

		}

		pthread_setcancelstate(Cancel, &Temp);

	} else {

		(global->barrier_rank).cycle = !(global->barrier_rank).cycle;

		(global->barrier_rank).counter = 0;

		Error = pthread_cond_broadcast(&(global->barrier_rank).cv);

	}

	pthread_mutex_unlock(&(global->barrier_rank).mutex);

}

     if ((MyNum == 0) || (stats)) {
       ranktime += (time3 - time2);
       sorttime += (time5 - time4);
     }
   } /* for */

   {

	uint32_t	Error, Cycle;

	int32_t		Cancel, Temp;



	Error = pthread_mutex_lock(&(global->barrier_rank).mutex);

	if (Error != 0) {

		printf("Error while trying to get lock in barrier.\n");

		exit(-1);

	}



	Cycle = (global->barrier_rank).cycle;

	if (++(global->barrier_rank).counter != (number_of_processors)) {

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);

		while (Cycle == (global->barrier_rank).cycle) {

			Error = pthread_cond_wait(&(global->barrier_rank).cv, &(global->barrier_rank).mutex);

			if (Error != 0) {

				break;

			}

		}

		pthread_setcancelstate(Cancel, &Temp);

	} else {

		(global->barrier_rank).cycle = !(global->barrier_rank).cycle;

		(global->barrier_rank).counter = 0;

		Error = pthread_cond_broadcast(&(global->barrier_rank).cv);

	}

	pthread_mutex_unlock(&(global->barrier_rank).mutex);

}
   if ((MyNum == 0) || (stats)) {
     {

	struct timeval	FullTime;



	gettimeofday(&FullTime, NULL);

	(time6) = (uint32_t)(FullTime.tv_usec + FullTime.tv_sec * 1000000);

}
     global->ranktime[MyNum] = ranktime;
     global->sorttime[MyNum] = sorttime;
     global->totaltime[MyNum] = time6-time1;
   }
   if (MyNum == 0) {
     global->rs = time1;
     global->rf = time6;
     global->final = to;
   }

}

/*
 * product_mod_46() returns the product (mod 2^46) of t1 and t2.
 */
double product_mod_46(double t1, double t2)
{
   double a1;
   double b1;
   double a2;
   double b2;
			
   a1 = (double)((int32_t)(t1 / RADIX_S));    /* Decompose the arguments.  */
   a2 = t1 - a1 * RADIX_S;
   b1 = (double)((int32_t)(t2 / RADIX_S));
   b2 = t2 - b1 * RADIX_S;
   t1 = a1 * b2 + a2 * b1;      /* Multiply the arguments.  */
   t2 = (double)((int32_t)(t1 / RADIX_S));
   t2 = t1 - t2 * RADIX_S;
   t1 = t2 * RADIX_S + a2 * b2;
   t2 = (double)((int32_t)(t1 / RADIX));

   return (t1 - t2 * RADIX);    /* Return the product.  */
}

/*
 * finds the (k)th random number, given the seed, b, and the ratio, t.
 */
double ran_num_init(uint32_t k, double b, double t)
{
   uint32_t j;

   while (k != 0) {             /* while() is executed m times
				   such that 2^m > k.  */
      j = k >> 1;
      if ((j << 1) != k) {
         b = product_mod_46(b, t);
      }
      t = product_mod_46(t, t);
      k = j;
   }

   return b;
}

int32_t get_max_digits(int32_t max_key)
{
  int32_t done = 0;
  int32_t temp = 1;
  int32_t key_val;

  key_val = max_key;
  while (!done) {
    key_val = key_val / radix;
    if (key_val == 0) {
      done = 1;
    } else {
      temp ++;
    }
  }
  return temp;
}

int32_t log_2(int32_t number)
{
  int32_t cumulative = 1;
  int32_t out = 0;

  while ((cumulative < number) && (cumulative != number) && (out < 50)) {
    cumulative = cumulative * 2;
    out ++;
  }

  if (cumulative == number) {
    return(out);
  } else {
    return(-1);
  }
}

void printerr(const char *s)
{
  fprintf(stderr,"ERROR: %s\n",s);
}

void init(int32_t key_start, int32_t key_stop, int32_t from)
{
   double ran_num;
   double sum;
   //int32_t tmp;
   int32_t i;
   int32_t *key_from;

   ran_num = ran_num_init((key_start << 2) + 1, SEED, RATIO);
   sum = ran_num / RADIX;
   key_from = (int32_t *) key[from];
   for (i = key_start; i < key_stop; i++) {
      ran_num = product_mod_46(ran_num, RATIO);
      sum = sum + ran_num / RADIX;
      ran_num = product_mod_46(ran_num, RATIO);
      sum = sum + ran_num / RADIX;
      ran_num = product_mod_46(ran_num, RATIO);
      sum = sum + ran_num / RADIX;
      key_from[i] = (int32_t) ((sum / 4.0) *  max_key);
      //tmp = (int32_t) ((key_from[i])/100);
      ran_num = product_mod_46(ran_num, RATIO);
      sum = ran_num / RADIX;
   }
}

void test_sort(int32_t final)
{
   int32_t i;
   int32_t mistake = 0;
   int32_t *key_final;

   printf("\n");
   printf("                  TESTING RESULTS\n");
   key_final = key[final];
   for (i = 0; i < num_keys-1; i++) {
     if (key_final[i] > key_final[i + 1]) {
       fprintf(stderr,"error with key %d, value %d %d \n",
        i,key_final[i],key_final[i + 1]);
       mistake++;
     }
   }

   if (mistake) {
      printf("FAILED: %d keys out of place.\n", mistake);
   } else {
      printf("PASSED: All keys in place.\n");
   }
   printf("\n");
}

void printout()
{
   int32_t i;
   int32_t *key_final;

   key_final = (int32_t *) key[global->final];
   printf("\n");
   printf("                 SORTED KEY VALUES\n");
   printf("%8d ",key_final[0]);
   for (i = 0; i < num_keys-1; i++) {
     printf("%8d ",key_final[i+1]);
     if ((i+2)%5 == 0) {
       printf("\n");
     }
   }
   printf("\n");
}

