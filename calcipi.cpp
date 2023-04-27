#include "calcpi.h"
#include <pthread.h>
#include <cmath>

struct info { // struct containing pthread, begin, end and r value, and the pixel count for that section
  pthread_t tid;
  int begin, end, r;
  uint64_t count = 0;
} tarr[256]; // create array of 256 as nthreads <= 256


void* counter(void* targ) // thread function
{
  struct info * temp = (struct info *) targ; // create a pointer to the thread argument, which will be of type info
  double rsq = double(temp ->r) * temp->r; // local variable max, as in original algorithm
  uint64_t count = 0; // initialize local count
  for( double x = temp->begin ; x <= temp->end ; x ++) // start at the begin, all the way to the end
    for( double y = 0 ; y <= temp->r ; y ++) // always start from 0 all the way to r as in original code
      if( x*x + y*y <= rsq) count ++; 
  temp->count = count; // then save count the thread argument
  pthread_exit(NULL); // exit thread after the loops
  return 0;
}

uint64_t count_pixels(int r, int n_threads)
{
  uint64_t fcount = 0; // final pixel count of all threads

  int work = ceil(r / n_threads); // split the work between the number of threads, using ceil to round up
  int begin = 1; // always begin at x = 1
  int end = work; // the end will be split work for initial thread
  
  for (int i = 0 ; i < n_threads ; i++) // create pthreads n_threads times
  {
    tarr[i].r = r; // assign values to thread struct
    tarr[i].begin = begin; // assign values to thread struct
    if (i == n_threads - 1)
    {
      tarr[i].end = r; // assign values to thread struct, if last thread, want to go all the way to r
      pthread_create(&tarr[i].tid, 0, counter, &tarr[i]); // create pthread and tell it to run thread function counter
    }
    else
    {
      tarr[i].end = end; // otherwise want this thread to go to the end, which is some multiple of work
      pthread_create(&tarr[i].tid, 0, counter, &tarr[i]); // create pthread and tell it to run thread function counter
    }
    begin = end + 1; // then start after the last end, but add one since it is <= in the loops
    if (end + work > r) end = r;  // if the end + work is greater than r, then end = r, we do not want it to go past r since using ceil()
    else end += work; // else, add work
  }

  for (int i = 0 ; i < n_threads ; i++) 
  {
    pthread_join(tarr[i].tid, 0); // wait for all threads to be finished, join back together
    fcount += tarr[i].count; // add all thread results to the final pixel count
  }

  return fcount * 4 + 1; // return final count * 4 + 1 as in the original code
}

