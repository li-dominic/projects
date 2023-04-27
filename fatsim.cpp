#include "fatsim.h"
#include <iostream>
#include <algorithm>

std::vector<long> fat_check(const std::vector<long> & fat)
{
  std::vector<long> result; // will store set of blocks where the largest block chain(s) start
  std::vector<std::vector<long>> adjList(fat.size() + 1); // reverse adjacency list

  long n;
  for (int i = 0 ; size_t(i) < fat.size() ; i++) // used to create the reverse adjacency list
  {
    n = fat[i];
    adjList[n + 1].push_back(i); // add 1 to n since index 0 will represent the NULL block (-1) from FAT     
  }
  
  if (adjList[0].empty()) return result; // if there are no pointers to NULL, the FAT is 100% corrupted, return empty result

  std::vector<long> stack; // contains the stack to implement the DFS through an iterative recursion
  long maxLen = 0; // will contain the maximum depth of the table
  std::vector<int> temp; // this variable will contain an array of depths for when a block branches off to different blocks - must save the depth at that time
  for (int i = 0 ; size_t(i) < adjList[0].size() ; i++) // for every block that is connected to -1
  {
    stack.push_back(adjList[0][i]); // push back next entry to the stack
    std::pair<long, int> ndpth; // ndpth.first is the node, ndpth.second is the depth that it is at
    ndpth.second = 0; // initialize depth to 0
    temp.clear(); // clear the temp variable just in case, although it should be empty now
    while(!stack.empty()) // this recursive stack implementation was taken from Xining's tutorial (recurseDir.cpp)
    {
      long node = stack.back(); // process the last entry in stack to implement DFS
      stack.pop_back(); // remove that entry from the stack
      ndpth.second++; // increase the depth by one
      ndpth.first = node; // assign the node value
      if (adjList[node + 1].empty()) // this means that it is the end of a block chain
      {
        if (ndpth.second > maxLen) // if the depth is greater than the current maxLen 
        {
          maxLen = ndpth.second; // set the new maximum depth found in the FAT
          result.clear(); // clear and update results accordingly
          result.push_back(ndpth.first);
        }
        else if (ndpth.second == maxLen) result.push_back(ndpth.first); // if the depth is equal to the current maximum depth, add the node to the results

        if (!temp.empty()) // if there was a branch earlier on (more than 1 path)
        {
          ndpth.second = temp.back(); // reset the depth to where that branch was
          temp.pop_back(); // remove that entry from the branch
        }
        // otherwise, no branch and the stack will be empty now, inner loop will end for this node
      }
      else // it is not the end of the block chain, and have to process the node(s) that this one is currently pointing to
      {
        int j;
        for (j = 0 ; size_t(j) < adjList[node + 1].size() ; j++)  stack.push_back(adjList[node + 1][j]); // push back all nodes that "node" is currently pointing to
        if (j > 1)
        {
          for (int k = 0; k < j - 1 ; k++)  temp.push_back(ndpth.second); // if j > 1, that means there is a branch, so have to add the depth to the temp variable as many times as there are branches - 1
        }
      } 
    }
  }
  if (result.size() > 1) std::sort(result.begin(), result.end()); // if the result contains more than one entry, have to sort it, otherwise return result

  return result;
}

