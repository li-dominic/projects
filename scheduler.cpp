#include "scheduler.h"
#include "common.h"

// this is the function you should implement
//
// runs Round-Robin scheduling simulator
// input:
//   quantum = time slice
//   max_seq_len = maximum length of the reported executing sequence
//   processes[] = list of process with populated IDs, arrivals, and bursts
// output:
//   seq[] - will contain the execution sequence but trimmed to max_seq_len size
//         - idle CPU will be denoted by -1
//         - other entries will be from processes[].id
//         - sequence will be compressed, i.e. no repeated consecutive numbers
//   processes[]
//         - adjust finish_time and start_time for each process
//         - do not adjust other fields
//
void simulate_rr(
    int64_t quantum,
    int64_t max_seq_len,
    std::vector<Process> &processes,
    std::vector<int> &seq)
{
    int64_t curr_time = 0;                   // current time
    int cpu;                                 // process that is being executed by the CPU now
    std::vector<int> RQ, JQ;                 // JQ - job queue and RQ - ready queue
    std::vector<int64_t> remaining_bursts;   // remaining bursts of the processes
    std::vector<int> sequence;               // local sequence variable - will store the sequence of executions up to max_seq_len
    std::vector<int64_t> RQremaining_bursts; // this variable contains the remaining bursts of the processes in RQ
    int psize = processes.size();            // use int to represent processes.size() - safe to use since the upper limit of processes size much less than max value of int

    for (auto proc : processes) // fill JQ and remaining_bursts with values from processes argument
    {
        JQ.push_back(proc.id);
        remaining_bursts.push_back(proc.burst);
    }

    while (1) // repeat forever, until JQ and RQ are both empty
    {
        if (JQ.empty() && RQ.empty())
            break; // if both empty, break loop

        if (RQ.empty() && !JQ.empty()) // if only RQ is empty, have to get next process from the JQ
        {
            RQ.push_back(JQ[0]);                               // append the next process from JQ to RQ
            RQremaining_bursts.push_back(remaining_bursts[0]); // and update RQ bursts variable accordingly as well
            for (int i = 0; i < psize; i++)
            {
                if (processes[i].id == JQ[0]) // find the arrival time of the process that was just taken from JQ, to see if a time skip is needed
                {
                    if (curr_time < processes[i].arrival) // if arrival time is greater than curr_time, then cpu was idle for that entire time, then skip curr_time to the arrival time
                    {
                        cpu = -1;
                        if (sequence.empty() || sequence[sequence.size() - 1] != -1)
                            sequence.push_back(cpu);      // push back -1 if it is not already in the sequence, and less than max length
                        curr_time = processes[i].arrival; // skip time to arrival time
                    }
                    JQ.erase(JQ.begin());                             // then erase the process entry in the JQ
                    remaining_bursts.erase(remaining_bursts.begin()); // same for the remaining_bursts variable

                    for (int j = i + 1; j < psize; j++)
                    {
                        if (processes[j].id == JQ[0] && processes[i].arrival == processes[j].arrival) // next, find any other processes that have the same arrival time
                        {
                            RQ.push_back(JQ[0]); // and remove it from JQ and append it to RQ, same for the remaining_bursts
                            RQremaining_bursts.push_back(remaining_bursts[0]);
                            JQ.erase(JQ.begin());
                            remaining_bursts.erase(remaining_bursts.begin());
                        }
                        if (curr_time < processes[j].arrival)
                            break; // if a process with a arrival time > curr_time, then break this loop
                    }
                    break; // break outer loop too, since found all processes with the same arrival time and skipped curr_time if needed
                }
            }
        }

        if (JQ.empty() && !RQ.empty()) // if only JQ is empty, then have to execute processes in RQ
        {
            bool flag = false;             // this flag is used to determine if we want to increment the current time by very large multiples of quantum
            int64_t N = 1;                 // N is the variable described in the hints, the largest multiple of quantum that we can skip curr_time to
            int64_t burst;                 // burst is the value of the remaining burst of the next process in the RQ
            cpu = RQ[0];                   // cpu gets the next process in the RQ
            burst = RQremaining_bursts[0]; // same as burst, but for the remaining burst of that process
            if (sequence.size() == size_t(max_seq_len))
                flag = true; // only apply this optimization if we already have all entries needed in the sequence
            for (int i = 0; i < psize; i++)
            {
                if (processes[i].id == cpu) // use this to see if the start time has already been set, if not - then set it to curr_time
                {
                    if (processes[i].start_time == -1)
                        processes[i].start_time = curr_time;
                }
                if (processes[i].start_time == -1 && flag) // if flag has been set, use this to check if all processes has been started (since JQ is empty)
                {
                    flag = false; // if a process hasn't been started, set flag to false and break loop
                    break;
                }
            }
            if (burst <= quantum) // in the case where the burst of the next process in RQ will finish in the quantum
            {
                RQ.erase(RQ.begin());                                 // remove the process from RQ
                RQremaining_bursts.erase(RQremaining_bursts.begin()); // remove it's remaining burst from RQr_b var
                if (sequence.empty() || (sequence[sequence.size() - 1] != cpu && sequence.size() < size_t(max_seq_len)))
                    sequence.push_back(cpu);    // append it to sequence if it is not duplicated
                curr_time += burst;             // add burst to curr_time since it will be less time than the quantum
                for (int i = 0; i < psize; i++) // this loop is to populate the finish_time of the process that just finished
                {
                    if (processes[i].id == cpu)
                    {
                        processes[i].finish_time = curr_time;
                        break;
                    }
                }
                flag = false; // set flag to false to make sure it does not execute any unwanted code
            }
            if (flag) // if flag is set, we want to calculate the maximum multiple (N) of quantum that can be executed, but if a process will finish in that time, we do not want to execute this
            {
                N = RQremaining_bursts[0] / (int(RQ.size()) * quantum); // first set N to the initial value of the remaining burst of the first process in RQ / RQ.size() * quantum
                for (int i = 1; size_t(i) < RQremaining_bursts.size(); i++)
                {
                    int64_t temp = RQremaining_bursts[i] / (int(RQ.size()) * quantum); // then go through the processes to see if there is a multiple less than N
                    if (temp < N)
                        N = temp; // if so, set that to N
                }
                if (N == 0 || N == 1)
                    flag = false; // if N is 0 or 1, that means there is a process in the RQ that will finish in the next quantum execution, so set flag to false
            }
            if (flag) // if flag is still set, then can execute the optimization
            {
                for (int i = 0; size_t(i) < RQremaining_bursts.size(); i++) // subtract the N*quantum from the remaining bursts of the RQ
                {
                    RQremaining_bursts[i] -= N * quantum;
                }
                curr_time += N * int(RQ.size()) * quantum; // add appropriate time to the curr_time
            }
            else if (burst > quantum && !flag)
            { // otherwise, if all the above fails, just execute 1 quantum of the process in the cpu right now
                RQ.erase(RQ.begin());
                RQremaining_bursts.erase(RQremaining_bursts.begin());
                burst -= quantum;
                curr_time += quantum;
                if (sequence.empty() || (sequence[sequence.size() - 1] != cpu && sequence.size() < size_t(max_seq_len)))
                    sequence.push_back(cpu);
                RQ.push_back(cpu); // add it to the end of the RQ again as well as the remaining burst
                RQremaining_bursts.push_back(burst);
            }
        }

        if (!JQ.empty() && !RQ.empty()) // if both the RQ and JQ are not empty
        {
            bool flag = false; // flag, N and burst are the 3 same variables as described above
            int64_t N = 1;
            int64_t burst;
            cpu = RQ[0];                   // cpu gets the next process in the RQ
            burst = RQremaining_bursts[0]; // same as burst gets the next remaining burst of the process in cpu now
            for (int i = 0; i < psize; i++)
            {
                if (processes[i].id == cpu) // check to see if the start time has been set yet
                {
                    if (processes[i].start_time == -1)
                        processes[i].start_time = curr_time; // if not, set it to curr_time
                }
                if (processes[i].id == JQ[0] && (sequence.size() == size_t(max_seq_len) || RQ.size() == 1)) // check the arrival time of the next process in the JQ
                {                                                                                           // and check to make sure the sequence size is already max length OR if there is only one process in the RQ
                    if (processes[i].arrival >= curr_time + int(RQ.size()) * quantum)                       // check to see if the process arrival time is greater
                    {                                                                                       // than or equal to if we executed a quantum of all processes in the RQ and added to curr_time
                        flag = true;                                                                        // if so, that means we can execute the multiple before or right at when a process arrives
                        N = (processes[i].arrival - curr_time) / (int(RQ.size()) * quantum);                // set N and break loop
                        break;
                    }
                }
            }
            if (burst <= quantum) // if the remaining burst will finish in the next quantum
            {
                RQ.erase(RQ.begin()); // remove process from RQ and it's remaining burst from the RQremaining_bursts variable
                RQremaining_bursts.erase(RQremaining_bursts.begin());
                curr_time += burst; // add burst to curr_time
                if (sequence.empty() || (sequence[sequence.size() - 1] != cpu && sequence.size() < size_t(max_seq_len)))
                    sequence.push_back(cpu);    // if sequence's last entry isn't cpu or it is not full, append cpu
                for (int i = 0; i < psize; i++) // populate the finish time of the process with curr_time
                {
                    if (processes[i].id == cpu)
                    {
                        processes[i].finish_time = curr_time;
                        break;
                    }
                }
                flag = false; // set flag to false to make sure it does not go into any other if statements
            }
            else if (flag) // if the flag is set, want to determine the largest N
            {
                for (int i = 0; size_t(i) < RQ.size(); i++) // go through the processes in RQ
                {
                    for (auto proc : processes) // make sure all processes in RQ have a start time, if not, set flag to false and exit both loops
                    {
                        if (proc.id == RQ[i])
                        {
                            if (proc.start_time == -1)
                            {
                                flag = false;
                            }
                            break;
                        }
                    }

                    if (!flag)
                        break;
                    if (RQremaining_bursts[i] <= quantum) // if any of the remaining bursts in RQ are less than quantum
                    {
                        flag = false; // set flag to false and break
                        break;
                    }
                    else
                    {
                        int64_t temp = RQremaining_bursts[i] / (int(RQ.size()) * quantum); // otherwise calculate temp to see if it is less than N, if so, set N to that value
                        if (temp < N)
                            N = temp;
                    }
                }
                if (N == 0 || N == 1)
                    flag = false; // if N is 0 then set flag to false as process may finish / arrive at the same time - which is the special case that has to be dealt with
            }

            if (flag) // if flag is still true, then can execute multiple N
            {
                for (int i = 0; size_t(i) < RQ.size(); i++)
                {
                    RQremaining_bursts[i] -= N * quantum; // subtract N*quantum from all remaining bursts in the RQ
                }
                curr_time += N * RQ.size() * quantum; // update curr_time
                if (RQ.size() == 1)                   // if RQ only contains one element, see if it needs to be appended to sequence
                {
                    if (sequence.empty() || (sequence[sequence.size() - 1] != RQ[0] && sequence.size() < size_t(max_seq_len)))
                        sequence.push_back(RQ[0]);
                }
            }
            else if (burst > quantum && !flag)
            {                         // otherwise just execute one quantum of the next process in RQ
                RQ.erase(RQ.begin()); // remove the process from RQ and it's burst from RQremaining_bursts too
                RQremaining_bursts.erase(RQremaining_bursts.begin());
                burst -= quantum; // update it's burst and curr_time
                curr_time += quantum;
                if (sequence.empty() || (sequence[sequence.size() - 1] != cpu && sequence.size() < size_t(max_seq_len))) // check to see if last entry of sequence is not cpu and is not max size, then append cpu
                    sequence.push_back(cpu);
                for (int i = 0; i < psize; i++) // check to see if any processes arrived in the time where it executed a quantum
                {
                    if (curr_time > processes[i].arrival && JQ[0] == processes[i].id) // if they arrived during that time
                    {
                        RQ.push_back(JQ[0]); // append it to RQ from the JQ, same goes for the RQremaining_bursts variable from remaining_bursts
                        RQremaining_bursts.push_back(remaining_bursts[0]);
                        JQ.erase(JQ.begin());
                        remaining_bursts.erase(remaining_bursts.begin()); // remove the entries from JQ and remaining_bursts
                    }                                                     // repeat this just in case there are more than one processes that arrived during that time
                    if (curr_time == processes[i].arrival)
                        break; // but if it finds a process that arrived at the same time the slice ended, break
                }
                RQ.push_back(cpu); // append cpu back to the RQ first, and it's remaining burst
                RQremaining_bursts.push_back(burst);
                for (int i = 0; i < psize; i++)
                {
                    if (curr_time == processes[i].arrival && JQ[0] == processes[i].id) // if process arrived at the same time the slice ended, append it to the RQs
                    {
                        RQ.push_back(JQ[0]); // just like above, append to RQ from JQ and then remove from JQ
                        RQremaining_bursts.push_back(remaining_bursts[0]);
                        JQ.erase(JQ.begin());
                        remaining_bursts.erase(remaining_bursts.begin());
                    }
                    if (curr_time < processes[i].arrival)
                        break; // if find a process with arrival time > curr_time, break loop
                }
            }
        }
    }
    while (!seq.empty()) // initial sequence may be filled with invalid processes, so clear them
    {
        seq.pop_back();
    }
    if (!sequence.empty()) // if the sequence is not empty (meaning there was processes to execute), assign seq to sequence
    {
        seq = sequence;
    }
}
