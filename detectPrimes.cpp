#include "detectPrimes.h"
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <pthread.h>
#include <atomic>

std::vector<int64_t> result; // global variable storing the list of prime numbers
std::vector<int64_t> numbs; // global vector storing the input numbers
std::atomic<bool> finished; // global variable used for telling threads when no more numbers to evaluate (atomic to be safe)

struct thread { // thread structure, storing important information like pthread, number of threads, thread ID, and a boolean flag
  pthread_t p_thread;
  int nthreads, id;
  bool prime;
} tarr[256]; // array of struct up to 256 as threads cannot go above 256 as 

class simple_barrier { // original given barrier class
  std::mutex m_;
  std::condition_variable cv_;
  int n_remaining_, count_;
  bool coin_;

  public:
  simple_barrier(int count = 1) { init(count); }
  void init(int count)
  {
    count_ = count;
    n_remaining_ = count_;
    coin_ = false;
  }
  bool wait()
  {
    if (count_ == 1) return true;
    std::unique_lock<std::mutex> lk(m_);
    if (n_remaining_ == 1) {
      coin_ = !coin_;
      n_remaining_ = count_;
      cv_.notify_all();
      return true;
    }
    auto old_coin = coin_;
    n_remaining_--;
    cv_.wait(lk, [&]() { return old_coin != coin_; });
    return false;
  }
};

int myIndex; // global index used for parsing through num array
simple_barrier barrier; // barrier variable for thread reuse
int64_t n; // used to represent extracted input number
std::atomic<bool> tcancel; // atomic thread cancellation variable

void* threadFunction (void* targ) // thread function that each thread will run
{
  struct thread *temp = (struct thread *) targ; // again create temporary reference variable pointing to argument
  bool primes; // flag used to store if number is a prime or not - unique to each thread
  while(1) // repeat forever - until no more numbers
  {
    primes = true; // reset prime flag to true
    if(barrier.wait()) // wait for all threads, let last thread do some single threaded code
    {
      if (size_t(myIndex) == numbs.size()) finished.store(true); // if index is same value as the size of input numbers array, set finished flag to true
      else { 
        n = numbs[myIndex]; // otherwise, get next number from vector and increment index variable
        myIndex++;
        tcancel.store(false); // reset thread cancellation variable to false
      }
    }
    barrier.wait(); // another barrier wait to end the single threaded code
    if (finished) break; // if finished flag is true, no more numbers - break the loop and return
    else {
      int tid = temp->id; // get the thread ID 
      int nt = temp->nthreads; // get the number of threads
      int m = round(sqrt(n) / (6 * nt)); // use m for partition of work 
      if (m == 0) // if m == 0 this means that n is a relatively small number - so let one thread work through it
      {
        if (barrier.wait()) // then use barrier to let one thread determine if this small number is a prime or not
        {
          uint64_t i = 5; // from original algorithm
          uint64_t max = sqrt(n); // from original algorithm
          // handle trivial cases same algorithm as before -> but changed to handle multi-threading, i.e. does not return when finding false
          if (n < 2) primes = false; // from original algorithm
          else if (n <= 3) primes = true; // 2 and 3 are primes
          if (n % 2 == 0 && n != 2) primes = false; // handle multiples of 2
          if (n % 3 == 0 && n != 3) primes = false; // handle multiples of 3

          while (i <= max) { //  the below loop is from the original algorithm, setting primes flag to false if it finds a divisor
            if (!primes) break; // if found a divisor, break the loop
            if (n % i == 0) primes = false;
            if (n % (i + 2) == 0) primes = false;
            i += 6;
          }

          if (primes == true) result.push_back(n); // only add the number to the list if it is a prime
        }
        barrier.wait(); // end the single threaded code
      }
      else { // otherwise m != 0, partition the work between the threads
        int64_t i = 5; // i always starts at 5
        if (tid != 0) // if it is not the first thread with a thread ID of 0
        {
          for (int t = 0 ; t < tid ; t++)
          {
            i += 6*m; // add 6 * m as many times as the thread ID to ensure it does not repeat the work done by the previous thread
          }
        }

        int64_t max;
      
        if (tid == nt - 1 || floor(sqrt(n)/6) - 1 == tid) max = sqrt(n); // if it is the last thread OR (sqrt(n)/(6*nthreads)) - 1 = TID (same as last thread), then max is the endpoint which is sqrt(n)
        else max = i + 6*(m - 1); // otherwise, you add an 6*m iteration to i as that is how far it is supposed to go. **NOTE the (m-1) is because in the loop it is <=

        if (max > sqrt(n)) max = sqrt(n); // if it is greater than sqrt(n) set it back to sqrt(n), do not want it surpassing this upper limit
        
        // handle trivial cases same algorithm as before -> but changed to handle multi-threading, i.e. does not return when finding false
        if (n < 2) primes = false;
        else if (n <= 3) primes = true; // 2 and 3 are primes
        if (n % 2 == 0 && n != 2) primes = false; // handle multiples of 2
        if (n % 3 == 0 && n != 3) primes = false; // handle multiples of 3

        if (primes == false && !tcancel) // if it found that the number is not a prime, and the tcancel is not already set, then set it to true for thread cancellation
        {
          tcancel.store(true);
        }

        while (i <= max) { // same loop as original algorithm
          if (tcancel) break; // a thread found a divisor, so break loop
          if (n % i == 0) primes = false;
          if (n % (i + 2) == 0) primes = false;
          i += 6;
          if (primes == false && !tcancel) // if found a divisor, and tcancel is not already set, set it to true and break the loop
          {
            tcancel.store(true);
            break;
          }
        }
      
        temp->prime = primes; // now set the temp and argument prime flag to the result of the one in this function (now *.primes in tarr is set)
        if (barrier.wait()) // wait for all threads to finish, use single thread to see if it should add it to the result
        {
          bool add = true; // flag to see if it should be added
          for (int i = 0 ; i < nt ; i++)
          {
            if(tarr[i].prime == false || tcancel) // go through the tarr array for all threads and see if there is a single false value
            {
              add = false; // if there is, set add to false and break the loop
              break;
            }
          }
          if (add) // if add is still true, then add it to the list, otherwise do nothing
          {
            result.push_back(n);
          }
        }
        barrier.wait(); // end single threaded code, now repeat in the while(1) loop
      }
    }
  }

  pthread_exit(NULL); // once done, exit the pthread and return 0
  return 0;
}

std::vector<int64_t> detect_primes(const std::vector<int64_t> & nums, int n_threads)
{
  numbs = nums; // global numbs array so that it can be accessed
  finished = false; // set finished flag = false
  barrier.init(n_threads); // initialize the barrier to the proper number of threads

  for (int i = 0 ; i < n_threads ; i++) // create n_threads
  {
    tarr[i].id = i; // assign the values using i and argument
    tarr[i].nthreads = n_threads;
    pthread_create(&tarr[i].p_thread, NULL, threadFunction, &tarr[i]); // each thread will run the thread function
  }

  for (int i = 0 ; i < n_threads ; i++) pthread_join(tarr[i].p_thread, 0);  // join back all threads

  return result; // return the global list of prime numbers
}
