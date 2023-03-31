#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>

struct Process {
    int pid;
    int burst_length;
    int arrival_time;
    int remaining_time;
    int finish_time;
    int turnaround_time;
    int processor_id;
};
struct Node {
    struct Node* prev;
    struct Process p;
    struct Node* next;
};

void process_thread() {
    // LOOP
        // IF queue empty (multi queue olursa kendi queuesu)
            // sleep 1ms continue;
        // ELSE IF FCFS 
            // sleep for the duration of next process
            // remove process from queue
        // ELSE IF SJF
            // sleep for the duration of next process
            // remove process from queue
        // ELSE IF RR
            // IF next process remaining time < Q
                // sleep for process duration
                // Remove process from queue
            // ELSE
                // Sleep for Q
                // Decrease process remaining time by Q
                // Remove from head add to tail
        
}

int main(int argc, char *argv[]) {
    // Get and set args

    // Get and record cur time

    // Create processor threads
    // Create queue(s)

    // LOOP MAIN THREAD
    // Read in file line by line (or generate randomly)

        // Create process object
        // Set arrival time values (compare with initial)
        // Sleep for the iat value

        // IF single queue
            // IF SJF
                // Add to queue ordered by burst length
            // ELSE
                // Add to queue
        // ELSE (Multi queue)
            // IF RR
                // IF SJF
                    // Add to next queue ordered by burst length
                // ELSE
                    // Add to next queue
            // ELSE IF LB
                // IF SJF
                    // Add to queue with least load ordered by burst length
                // ELSE
                    // Add to queue with least load
        // Read next input

    // Add dummy node to all queues

    return 0;
}



