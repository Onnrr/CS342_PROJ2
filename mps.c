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
struct Node* doneProcessesHead;
struct Node* doneProcessesTail;
pthread_mutex_t* doneProcessesLock;
pthread_mutex_t** locks;
struct timeval start;

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
typedef struct Arguments {
    int index;
    char* algorithm;
    int q;
} Arguments;

int timeval_diff_ms(struct timeval* t1, struct timeval* t2) {
    int diff_sec = t2->tv_sec - t1->tv_sec;
    int diff_usec = t2->tv_usec - t1->tv_usec;
    int diff_ms = diff_sec * 1000 + diff_usec / 1000;
    return diff_ms;
}

void displayList(struct Node* root) {
    if (root == NULL) {
        printf("Queue is empty.\n");
    }
    else {
        struct Node* current = root;

        while (current != NULL) {
            printf("----------------\n");
            printf("- id: %d\n", current->p->pid);
            printf("- burst length: %d\n", current->p->burst_length);
            printf("- arrival time: %d\n", current->p->arrival_time);
            current = current->next;
        }
        printf("----------------\n");
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

Node* findShortest(int queue_index) {
    Node* cur = queues[queue_index];

    Node* minNode = cur;

    while (cur != NULL) {
        if (cur->p->remaining_time < minNode->p->remaining_time) {
            minNode = cur;
        }
        cur = cur->next;
    }
    return minNode;
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

struct Node* retrieveNode(struct Node** root, struct Node** tail, struct Node* deleteNode) {

    // empty list
    if (*root == NULL || deleteNode == NULL)
        return NULL;
    else {
        // delete head
        if (*root == deleteNode) {
            if (*root == *tail) {
                *root = NULL;
                *tail = NULL;
            }
            else {
                deleteNode->next->prev = NULL;
                *root = deleteNode->next;
            }
        }
        else if (*tail == deleteNode) {
            (*tail)->prev->next = NULL;
            (*tail) = deleteNode->prev;
            deleteNode->prev = NULL;
        }
        else {
            deleteNode->next->prev = deleteNode->prev;
            deleteNode->prev->next = deleteNode->next;
        }   

        return deleteNode;
    }
}

struct Node* retrieveFirstNode(struct Node** root, struct Node** tail) {
    return retrieveNode(&root, &tail, root);
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

void insertAsc(struct Node** root, struct Node** tail, struct Node* newNode) {
    if (*root == NULL) {
        *root = newNode;
        *tail = newNode;
    }
    else if ((*root)->p->pid < newNode->p->pid) {
        newNode->next = *root;
        newNode->next->prev = newNode;
        *root = newNode;
    }
    else {
        // insert in the middle or end
        struct Node* cur = *root;

        while (cur->next != NULL && cur->next->p->pid  >= newNode->p->pid ) {
            cur = cur->next;
        }

        if (cur->next != NULL) {
            newNode->next = cur->next;
            newNode->next->prev = newNode;
            cur->next = newNode;
            newNode->prev = cur;
        }
        else {
            newNode->next = (*tail)->next;
            (*tail)->next = newNode;
            newNode->prev = *tail;
            *tail = newNode;
        }

    }
}

void* process_thread(void *arguments) {
    struct Arguments *args = arguments;
    int index = args->index;
    
    while (1) {
        mutex_lock(&locks[index]);
        if (queues[index] == NULL) {
            sleep(1);
        }
        else {
            int burstFinished = 0;
            struct Node* cur;

            if (cur->p->pid = -1) {
                mutex_unlock(&locks[index]);
                pthread_exit(0);
            }
            else if (strcmp(args->algorithm, "FCFS")) {
                // retrieve process from the queue
                cur = retrieveFirstNode(&queues[index], &tails[index]);
                mutex_unlock(&locks[index]);

                // sleep for the duration of burst
                sleep(cur->p->burst_length);
                burstFinished = 1;
            }
            else if (strcmp(args->algorithm, "SJF")) {
                // find and retrieve shortest node
                cur = retrieveNode(&queues[index], &tails[index], findShortest(index));
                mutex_unlock(&locks[index]);

                // sleep for duration of burst
                sleep(cur->p->burst_length);
                burstFinished = 1;
            }
            else { // default RR
                // retrieve process from the queue
                cur = retrieveFirstNode(&queues[index], &tails[index]);
                mutex_unlock(&locks[index]);

                if (cur->p->remaining_time <= args->q) {
                    sleep(cur->p->remaining_time);
                    burstFinished = 1;
                }
                else {
                    sleep(args->q);
                    // update remaining time and add to tail
                    cur->p->remaining_time = cur->p->remaining_time - args->q;
                    mutex_lock(&locks[index]);
                    insertToEnd(&queues[index], &tails[index], cur);
                    mutex_unlock(&locks[index]);
                }
            }
            

            if (burstFinished) {
                // update information of process
                struct Process* p = cur->p;
                p->finish_time = gettimeofday(&start, NULL);
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->remaining_time = 0;
                p->processor_id = index;

                // add to finished processes list
                mutex_lock(&doneProcessesLock);
                insertAsc(&doneProcessesHead, &doneProcessesTail, cur);
                mutex_unlock(&doneProcessesLock);
            }
        }
    }       
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

    // Get and record start time
    gettimeofday(&start, NULL);

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

    // Create processor threads
    pthread_t threads[num_of_processors];
    for (int i = 0; i < num_of_processors; i++){
        Arguments args;
        args.index = i;
        args.algorithm = algorithm;
        args.q = quantum;

        pthread_create(&threads[i], NULL, process_thread, (void*) &args);
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

    int cur_id = 0;

    while ((read = getline(&line, &len, fp)) != -1) {
        // Process the line
        char* keyword = strtok(line, " ");
        char* burst = strtok(NULL, " ");
        int burst_l = atoi(burst);

        struct timeval arrival;
        gettimeofday(&arrival, NULL);

        Process *p = (Process*)malloc(sizeof(struct Process));
        p->pid = cur_id;
        p->burst_length = burst_l;
        p->remaining_time = burst_l;
        p->arrival_time = timeval_diff_ms(&start, &arrival);

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
                printf("Selected queue = %d\n", q_index);
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

        sleep(iat / 1000);
    }

    // Add dummy Nodes
    Process *p = (Process*)malloc(sizeof(struct Process));
    p->pid = -1;
    Node* dummy = createNode(p);
    if (strcmp(scheduling_approach, "S") == 0) {
        pthread_mutex_lock(locks[0]);
        insertToEnd(&queues[0],&tails[0],dummy);
        pthread_mutex_unlock(locks[0]);
    }
    else {
        for (int i = 0; i < num_of_processors; i++) {
            pthread_mutex_lock(locks[i]);
            insertToEnd(&queues[i],&tails[i],dummy);
            pthread_mutex_unlock(locks[i]);
        }
    }

    // Free the memory allocated for the line
    free(line);

    // Close the file
    fclose(fp);

    // Wait for threads
    for (int i = 0; i < num_of_processors; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < num_of_processors; i++) {
        printf("Queue %d\n", i);
        displayList(queues[i]);
    }

    return 0;
}



