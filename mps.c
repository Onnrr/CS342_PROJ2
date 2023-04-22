#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

struct Node** queues;
struct Node** tails;
pthread_mutex_t** locks;

typedef struct Process {
    int pid;
    int burst_length;
    int arrival_time;
    int remaining_time;
    int finish_time;
    int turnaround_time;
    int processor_id;
} Process;
typedef struct Node {
    struct Node* prev;
    struct Process* p;
    struct Node* next;
} Node;

void displayList(struct Node* root) {
    if (root == NULL) {
        printf("Queue is empty.\n");
    }
    else {
        struct Node* current = root;

        while (current != NULL) {
            printf("- %d\n", current->p->pid);
            current = current->next;
        }
        printf("\n");
    }
}

struct Node* createNode(struct Process* p) {
    struct Node* newNode = (struct Node*) malloc(sizeof(struct Node));

    newNode->prev = newNode->next = NULL;
    // kontrol
    newNode->p = p;
    return newNode;
}

void deleteNode(struct Node** root, struct Node** tail) {

    struct Node* deleteNode = *root;
    if (*root == NULL)
        return;
    if (*tail == deleteNode) {
        *root = NULL;
        *tail = NULL;
    }
    else {
        *root = deleteNode->next;
        deleteNode->next->prev = deleteNode->prev;
    }
    free(deleteNode);
}

void insertToEnd(struct Node** root, struct Node** tail, struct Node* newNode) {
    // check if list is empty
    if (*root == NULL) {
        *root = newNode;
        *tail = newNode;
    }
    else {
        // insert to end
        newNode->next = (*tail)->next;
        (*tail)->next = newNode;
        newNode->prev = *tail;
        *tail = newNode;
    }
}

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

int findLeastLoad(int num_of_proc) {
    int min;
    int minIndex = 0;


    for (int i = 0; i < num_of_proc; i++) {
        Node* cur = queues[i];
        int sum = 0;
        while (cur != NULL) {
            sum += cur->p->remaining_time;
            cur = cur->next;
        }
        if (i == 0) {
            min = sum;
        }
        if (sum < min) {
            min = sum;
            minIndex = i;
        }
    }
    return minIndex;
}

int main(int argc, char *argv[]) {
    // Get and set args
    int num_of_processors = 2;
    char* scheduling_approach = "M";
    char* queue_selection_method = "RM";
    char* algorithm = "RR";
    int quantum = 20;
    char* infile_name = "in.txt";
    int out_mode = 1;
    char* outfile_name = "out.txt";

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            num_of_processors = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-a") == 0) {
            scheduling_approach = argv[i + 1];
            queue_selection_method = argv[i + 2];
        }
        else if (strcmp(argv[i], "-s") == 0) {
            algorithm = argv[i + 1];
            quantum = atoi(argv[i + 2]);
        }
        else if (strcmp(argv[i], "-i") == 0) {
            infile_name = argv[i + 1];
        }
        else if (strcmp(argv[i], "-m") == 0) {
            out_mode = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-o") == 0) {
            outfile_name = argv[i + 1];
        }
        else if (strcmp(argv[i], "-r") == 0) {
            // TODO
        }
    }

    printf("Num of processors = %d\n", num_of_processors);
    printf("Sched approach = %s\n", scheduling_approach);
    printf("Queue selection method = %s\n", queue_selection_method);
    printf("Algorithm = %s\n", algorithm);
    printf("quantum = %d\n", quantum);
    printf("infile name = %s\n", infile_name);
    printf("out mode = %d\n", out_mode);
    printf("outfile name = %s\n", outfile_name);

    // Random variables

    // Get and record cur time
    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    printf("1\n");
    // Create queue(s)
    if (strcmp(scheduling_approach, "S") == 0) {
        queues = (Node**)malloc(sizeof(struct Node*));
        tails = (Node**)malloc(sizeof(struct Node*));
        locks = (pthread_mutex_t**)malloc(sizeof(pthread_mutex_t*));
        queues[0] = NULL;
        tails[0] = NULL;
        pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(mutex, NULL);
        locks[0] = mutex;
    }
    else {
        queues = (Node**)malloc(num_of_processors * sizeof(struct Node*));
        tails = (Node**)malloc(num_of_processors * sizeof(struct Node*));
        locks = (pthread_mutex_t**)malloc(num_of_processors * sizeof(pthread_mutex_t*));
        for (int i = 0; i < num_of_processors; i++) {
            queues[i] = NULL;
            tails[i] = NULL;
            pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(mutex, NULL);
            locks[i] = mutex;
        }
    }
    printf("2\n");

    // Create processor threads
    for (int i = 0; i < num_of_processors; i++){
        
    }

    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    
    // Open the file for reading
    fp = fopen(infile_name, "r");
    if (fp == NULL) {
        printf("Error: Unable to open file\n");
        return 1;
    }
    else {
        printf("Opened File\n");
    }

    int cur_id = 0;

    while ((read = getline(&line, &len, fp)) != -1) {
        // Process the line
        char* keyword = strtok(line, " ");
        char* burst = strtok(NULL, " ");
        int burst_l = atoi(burst);

        Process *p = (Process*)malloc(sizeof(struct Process));
        p->pid = cur_id;
        p->burst_length = burst_l;
        p->remaining_time = burst_l;

        struct timeval arrival;
        gettimeofday(&arrival, NULL);
        p->arrival_time = arrival.tv_sec;

        if (strcmp(scheduling_approach,"S") == 0) {
            pthread_mutex_lock(locks[0]);
            insertToEnd(&queues[0],&tails[0],createNode(p));
            pthread_mutex_unlock(locks[0]);
        }
        else if (strcmp(scheduling_approach,"M") == 0) {
            int q_index;
            if (strcmp(queue_selection_method,"RM") == 0) {
                printf("\nIn queue selection\n");
                printf("Cur id = %d\n", cur_id);
                q_index = cur_id % num_of_processors;
                printf("Selected queue = %d\n", q_index);
            }
            else if (strcmp(queue_selection_method,"LM") == 0) {
                printf("\nIn load queue selection\n");
                q_index = findLeastLoad(num_of_processors);
            }
            pthread_mutex_lock(locks[q_index]);
            insertToEnd(&queues[q_index],&tails[q_index],createNode(p));
            pthread_mutex_unlock(locks[q_index]);
        }

        read = getline(&line, &len, fp);
        if (read == -1) {
            break;
        }
        keyword = strtok(line, " ");
        char* inter_arrival = strtok(NULL, " ");
        int iat = atoi(inter_arrival);
        
        cur_id++;
    }

    // Free the memory allocated for the line
    free(line);

    // Close the file
    fclose(fp);

    for (int i = 0; i < num_of_processors; i++) {
        printf("Queue %d\n", i);
        displayList(queues[i]);
    }

    return 0;
}



