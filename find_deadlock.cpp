#include "find_deadlock.h"
#include "common.h"
#include <limits>

/// this is the function you need to (re)implement
///
/// parameter edges[] contains a list of request- and assignment- edges
///   example of a request edge, process "p1" resource "r1"
///     "p1 -> r1"
///   example of an assignment edge, process "XYz" resource "XYz"
///     "XYz <- XYz"
///
/// You need to process edges[] one edge at a time, and run a deadlock
/// detection after each edge. As soon as you detect a deadlock, your function
/// needs to stop processing edges and return an instance of Result structure
/// with 'index' set to the index that caused the deadlock, and 'procs' set
/// to contain names of processes that are in the deadlock.
///
/// To indicate no deadlock was detected after processing all edges, you must
/// return Result with index=-1 and empty procs.
///

class Graph
{
public:
    std::vector<std::vector<int>> adj_list; // as described in assignment document
    std::vector<int> out_counts;            // as described in assignment document
    std::vector<std::string> nodes;         // used to store the string names of nodes before converted to ints
} graph;

static bool ends_with(const std::string &str, const std::string &suffix) // this function was taken from the given code in assignment 2 from Professor Pavol
{
    if (str.size() < suffix.size())
        return false;
    else
        return 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix); // returns true if string str ends with string suffix
}

std::vector<std::string> topoSort() // topological sort, returns a vector of strings where out[n] > 0, if none then empty vector
{
    std::vector<int> out = graph.out_counts; // copy out_counts locally so it can be manipulated
    std::vector<int> zeros;
    for (int i = 0; size_t(i) < out.size(); i++) // populate zeros vector with all nodes where they have 0 outgoing edges
        if (out[i] == 0)
            zeros.push_back(i);

    int n;                                                // will contain entries of zeros
    std::vector<std::vector<int>> &adjM = graph.adj_list; // pass by reference the adj_list since it will not be modified
    std::vector<std::string> result;                      // vector of strings to be returned - empty initially
    while (!zeros.empty())
    {
        n = zeros[zeros.size() - 1]; // get last element of zeros and store in n
        zeros.pop_back();            // remove that element from zeros
        for (int i = 1; size_t(i) < adjM[n].size(); i++)
        {
            out[adjM[n][i]]--; // for all nodes connected to n (or adj[n][i], i being the nodes connected to n) decrement their entries in out
            if (out[adjM[n][i]] == 0)
                zeros.push_back(adjM[n][i]); // if the entry in out is 0, append it to the zeros vector
        }
    }

    for (int i = 0; size_t(i) < out.size(); i++) // parse through the out vector and check for non-zero entries
    {
        if (out[i] > 0)
        {
            if (ends_with(graph.nodes[i], "_p")) // if it is zero, check if it is a process by the unique tag added to processes
            {
                std::string proc = graph.nodes[i];
                proc.pop_back();
                proc.pop_back();
                result.push_back(proc); // add process to the returning result
            }
        }
    }

    return result; // return empty vector if no processes in deadlock, else return the processes involved in a deadlock
}

Result find_deadlock(const std::vector<std::string> &edges)
{
    Result result;
    std::vector<std::string> temp;   // temporary vector to parse each edge
    std::vector<std::string> sorted; // used to collect results from topological sort
    Word2Int w2i;                    // used to convert the unique node strings into unique ints
    int adjSize;                     // can use ints to represent size of vectors as per the limit of 30,000 lines, will not surpass the maximum value of an int
    int outSize;
    int nodeSize;
    for (int i = 0; size_t(i) < edges.size(); i++) // loop through every element in edges
    {
        temp = split(edges[i]);           // use split function to parse the edges entry, giving a vector of length 3
        int n1 = w2i.get(temp[0] + "_p"); // convert to int, appending _p to show it is a process (regular entries in edges will only contain alphanumeric, so _p will not be in the input)
        int n2 = w2i.get(temp[2]);        // convert resource to int, no unique appending needed for this
        nodeSize = graph.nodes.size();    // assign the sizes of vectors to the local ints
        adjSize = graph.adj_list.size();
        outSize = graph.out_counts.size();

        if (n1 > nodeSize - 1)
            graph.nodes.push_back(temp[0] + "_p"); // if n1 > number of elements in node, then append the string to nodes, same for n2
        if (n2 > nodeSize - 1)
            graph.nodes.push_back(temp[2]);

        if (temp[1] == "->") // if it is a request edge (pointing from process to resource)
        {
            if (outSize - 1 >= n1)
                graph.out_counts[n1]++; // check to see if n1 is already in out_counts
            else
                graph.out_counts.push_back(1); // if not, append 1 to show that there is 1 outgoing edge for n1 currently
            if (outSize - 1 < n2)
                graph.out_counts.push_back(0); // check to see if n2 is not yet in out_counts, if it isn't append 0 for a placeholder

            if (adjSize - 1 < n1) // if n1 is not in the adj_list
            {
                std::vector<int> ntemp; // add n1 by storing it in a vector, then appending that vector to adj_list
                ntemp.push_back(n1);
                graph.adj_list.push_back(ntemp);
            } // otherwise, n1 is already in adj_list and we don't have to do anything
            if (adjSize - 1 >= n2)
                graph.adj_list[n2].push_back(n1); // check to see if n2 is already in the adj_list, if it is, add n1 to its vector of ints
            else                                  // otherwise if it is not, then create a vector of ints with the correct order: n2, n1 and append it to the adj_list
            {
                std::vector<int> n21;
                n21.push_back(n2);
                n21.push_back(n1);
                graph.adj_list.push_back(n21);
            }
        }
        else // otherwise, the edge is a assignment edge (pointing from process <- resource)
        {
            if (outSize - 1 < n1)
                graph.out_counts.push_back(0); // check to see if n1 is not yet in out_counts, if it isn't append 0 as placeholder
            if (outSize - 1 >= n2)
                graph.out_counts[n2]++; // check if n2 is already in out_counts, increment, else append 1 to out_counts
            else
                graph.out_counts.push_back(1);

            if (adjSize - 1 >= n1)
                graph.adj_list[n1].push_back(n2); // if n1 is already in adj_list, append n2 to it's vector
            else
            {
                std::vector<int> n12; // otherwise, like above create vector with correct order: n1, n2 and append it to the adj_list
                n12.push_back(n1);
                n12.push_back(n2);
                graph.adj_list.push_back(n12);
            }
            if (adjSize - 1 < n2) // if n2 is not yet in adj_list, then add it similar to above
            {
                std::vector<int> ntemp;
                ntemp.push_back(n2);
                graph.adj_list.push_back(ntemp);
            } // otherwise, nothing is needed to be done for n2
        }
        sorted = topoSort(); // run topoSort on the current state of the graph, and store it's return value in sorted variable
        if (!sorted.empty()) // if sorted is not an empty vector, then topoSort found processes that are involved in a deadlock
        {
            result.procs = sorted; // assign result.proc sorted vector to show the processes in a deadlock
            result.index = i;      // index set to i as statement in assignment specs
            return result;         // return the result, which is the first time it found a deadlock
        }
    }

    result.index = -1; // topological sort did not find any deadlocks, so return empty vector and -1 as index
    return result;
}
