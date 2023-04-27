#include "memsim.h"
#include <cassert>
#include <iostream>
#include <list>
#include <math.h>
#include <unordered_map>
#include <set>
#include <algorithm>

struct Partition {
  int tag;
  int64_t size, address;

  Partition(int itag, int64_t isize, int64_t iaddress)
  {
    tag = itag;
    size = isize;
    address = iaddress;
  }
};

typedef std::list<Partition>::iterator PartitionRef;

struct scmp {
  bool operator()(const PartitionRef & c1, const PartitionRef & c2) const 
  {
    if (c1->size == c2->size)
      return c1->address < c2->address;
    else
      return c1->size > c2->size;
  }
};

struct Simulator {
  std::list<Partition> all_blocks;
  std::unordered_map<long, std::vector<PartitionRef>> tagged_blocks;
  std::set<PartitionRef,scmp> free_blocks;
  int64_t pageSize;
  int64_t n_pages_requested = 0;

  Simulator(int64_t page_size)
  {
    pageSize = page_size;
  }

  void allocate(int tag, int size)
  {
    bool found = false;
    PartitionRef cptr;
    int64_t maxSize = size;

    if (!free_blocks.empty()) // if there is a free partition, check to see if largest free block size is greater than allocation size
    {
      auto iter = free_blocks.begin();
      if ((*iter)->size >= size) // if not found, flag "found" = false
      {
        maxSize = (*iter)->size;
        found = true;
        cptr = (*iter);
      }
    }

    if (maxSize > size && found) // if free block and size > incoming size
    {
      int64_t newSize = cptr->size - size; // calculate leftover size which will be a new free partition after current one
      int64_t newAddress = cptr->address + size;
      free_blocks.erase(cptr);
      cptr->size = size;
      cptr->tag = tag;
      tagged_blocks[tag].push_back(cptr);  // update current partition and append it to tagged_blocks
      Partition insertP(-1, newSize, newAddress);
      auto nextcptr = std::next(cptr);
      all_blocks.insert(nextcptr, insertP); // append leftover free partition to all blocks, and the iterator to free_blocks
      free_blocks.insert(std::prev(nextcptr));  
    }
    else if (maxSize == size && found)  // or if free block size is exactly equal to allocation size
    {
      free_blocks.erase(cptr);
      cptr->tag = tag;
      tagged_blocks[tag].push_back(cptr); // append to tagged_blocks
    }
    
    if (!found) // if there is no free partition with greater size found
    {
      cptr = std::prev(all_blocks.end()); // point to the end of the list
      if (cptr->tag == -1) // if end of list is free
      {      
        int64_t nSize = size - cptr->size; // nSize is the size actually required after taking last free block into consideration
        int pages;// = nSize / pageSize + 1;
        if ((nSize % pageSize) == 0) pages = nSize / pageSize;
        else pages = nSize / pageSize + 1; // request the number of pages (int/int) rounds down, so add one
        n_pages_requested += pages; // add to variable for stats
        nSize = (pages * pageSize) - nSize; // now nSize is the remaining free size
        free_blocks.erase(cptr);
        cptr->tag = tag;
        cptr->size = size;
        tagged_blocks[tag].push_back(cptr); // update partition and append to tagged_blocks
        if (nSize > 0) // only if nSize > 0, append new free partition of that size
        {
          Partition insertP(-1, nSize, size + cptr->address);
          all_blocks.push_back(insertP);
          free_blocks.insert(std::prev(all_blocks.end())); // append iterator to free_blocks
        }
      }
      else // otherwise, last partition is not free
      {
        int pages;
        if ((size % pageSize) == 0) pages = size / pageSize; // calculate pages needed and add to n_pages_requested
        else pages = size / pageSize + 1;
        n_pages_requested += pages;
        int64_t remainingSize = (pages * pageSize) - size; // see if any leftover size that will be free partition
        if (all_blocks.empty()) // but if all_blocks empty, push back tag, size, and correct address
        {
          Partition dinsertP(tag, size, 0);
          all_blocks.push_front(dinsertP);
          tagged_blocks[tag].push_back(all_blocks.begin()); // append to tagged_blocks too
        }
        else
        {
          Partition dinsertP(tag, size, cptr->address + cptr->size); // otherwise, not empty and append using correct address
          all_blocks.push_back(dinsertP);
          tagged_blocks[tag].push_back(std::prev(all_blocks.end())); // append to tagged_blocks
        }
        if (remainingSize > 0)
        {
          cptr = std::prev(all_blocks.end());
          Partition insertP(-1, remainingSize, size + cptr->address); // if still remaining, append as free partitioon
          all_blocks.push_back(insertP);
          free_blocks.insert(std::prev(all_blocks.end())); // insert it to free_blocks too
        }
      }
    }
  }
  void deallocate(int tag)
  {
    for (int i = 0 ; size_t(i) < tagged_blocks[tag].size() ; i++) // go through all iterators in tagged_blocks[tag]
    {
      auto cptr = tagged_blocks[tag][i]; // create temporary iterator
      bool flag = false; 
      if (cptr != all_blocks.begin() && std::prev(cptr)->tag == -1) // if it is not the beginning, and previous is also tag = -1, merge them
      {
        auto prevcptr = std::prev(cptr);
        cptr->tag = -1;
        cptr->size += prevcptr->size; // add previous partition size
        cptr->address = prevcptr->address; // address is always earlier partition
        free_blocks.erase(prevcptr);
        all_blocks.erase(prevcptr); // erase the previous partition
        flag = true;
      } 
      if (cptr != std::prev(all_blocks.end()) && std::next(cptr)->tag == -1) // if it is not the last element of list, and next partition tag = -1, merge them
      {
        auto nextcptr = std::next(cptr);
        cptr->tag = -1;
        cptr->size += nextcptr->size; // add size, update tag, but keep address
        free_blocks.erase(nextcptr);
        all_blocks.erase(nextcptr); // remove from all_blocks
        flag = true;
      }
      if (!flag)  cptr->tag = -1; // if it didn't go in the above 2 if statements, just set current block tag = -1
      free_blocks.insert(cptr); // in all cases, append current partition to free_blocks
    }
    tagged_blocks.erase(tag); // then erase the entire entry from tagged_blocks as all are free now
  }

  MemSimResult getStats()
  {
    MemSimResult result; // set initial values as stated in assignment doc
    result.max_free_partition_size = 0;
    result.max_free_partition_address = 0;
    result.n_pages_requested = n_pages_requested;
    if (!free_blocks.empty()) // but if there is a free partition
    {
      auto ptr = *(free_blocks.begin()); // use largest one with lowest address (first element of free_blocks)
      result.max_free_partition_address = ptr->address;
      result.max_free_partition_size = ptr->size;
    }
    return result;
  }
  void check_consistency()
  {
    //check_consistency_internal();
  }
  void check_consistency_internal()
  {
    // used for debugging
  }
 
};

MemSimResult mem_sim(int64_t page_size, const std::vector<Request> & requests)
{
  Simulator sim(page_size);
  for (const auto & req : requests) {
    if (req.tag < 0) {
      sim.deallocate(-req.tag);
    } else {
      sim.allocate(req.tag, req.size);
    }
    sim.check_consistency();
  }
  return sim.getStats();
}
