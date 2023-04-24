#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

struct Node** queues;
struct Node** tails;
struct Node* doneProcessesHead;
struct Node* doneProcessesTail;
pthread_mutex_t* doneProcessesLock, *startLock;
pthread_mutex_t** locks;
struct timeval start, a, b;

typedef struct Process {
    int pid;
    int burst_length;
    int arrival_time;
    int remaining_time;
    int finish_time;
    int waiting_time; //YYYYYYYYYYYYYYYYYYYOOOOOOOOOOOOOOOOOOOOOOOOKKKKKKKKKKKKKKK EKSİK
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
    int single;
} Arguments;

int queue_length(struct Node *root) {
    if (root == NULL) {
        return 0;
    }
    return 1 + queue_length(root->next);
}

int generateRandomInt(int min, int max, int mean)
{
    double lambda = 1.0 / mean;
    double u, x;
    int a;

    do {
        // Generate exponential random value x
        u = (double)rand() / RAND_MAX;
        x = -log(1 - u) / lambda;

        // Round x to an integer
        a = (int)round(x);

    } while (a < min || a > max);

    return a;
}

void freeQueue(Node* root) {
    Node *ptr = root;
    while (ptr != NULL) {
        Node *temp = ptr->next;
        ptr->next = NULL;
        ptr->prev = NULL;
        if (ptr->p != NULL) {
            free(ptr->p);
        }
        if (ptr != NULL) {
            free(ptr);
        }
        ptr = temp;
    }
}

int timeval_diff_ms(struct timeval* t1, struct timeval* t2) {
    int diff_sec = t2->tv_sec - t1->tv_sec;
    int diff_usec = t2->tv_usec - t1->tv_usec;
    int diff_us = diff_sec * 1000000 + diff_usec;
    return diff_us;
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
            printf("- remaining time: %d\n", current->p->remaining_time);
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
        pthread_mutex_lock(locks[i]);
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
        pthread_mutex_unlock(locks[i]);
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
    return retrieveNode(root, tail, *root);
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

void insertToHead(struct Node** root, struct Node** tail, struct Node* newNode) {
    // check if list is empty
    if (*root == NULL) {
        *root = newNode;
        *tail = newNode;
    }
    else {
        // insert to start
        newNode->next = *root;
        newNode->prev = NULL;
        *root = newNode;
    }
}

void insertAsc(struct Node** root, struct Node** tail, struct Node* newNode) {
    if (*root == NULL) {
        *root = newNode;
        *tail = newNode;
    }
    else if ((*root)->p->pid >= newNode->p->pid) {
        newNode->next = *root;
        newNode->next->prev = newNode;
        *root = newNode;
    }
    else {
        // insert in the middle or end
        struct Node* cur = *root;

        while (cur->next != NULL && cur->next->p->pid < newNode->p->pid ) {
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
    struct Arguments *args = (Arguments*)arguments;
    int index;
    if (args->single)
        index = 0;
    else
        index = args->index;

    printf("Created thread with q index %d\n", args->index);
    while (1) {
        pthread_mutex_lock(locks[index]);
        if (queues[index] == NULL) {
            pthread_mutex_unlock(locks[index]);
            usleep(1);
        }
        else {
            int burstFinished = 0;
            struct Node* cur;
            struct timeval cur_time;
            gettimeofday(&cur_time, NULL);

            /* ADDS TO TURNAROUND TIME
            printf("--------------\n");
            printf("Processor reading from queue %d\n", index);
            displayList(queues[index]);
            */

            if (strcmp(args->algorithm, "FCFS") == 0 || strcmp(args->algorithm, "RR") == 0) 
                cur = retrieveFirstNode(&queues[index], &tails[index]);
            else if (strcmp(args->algorithm, "SJF") == 0)
                cur = retrieveNode(&queues[index], &tails[index], findShortest(index));

            if (cur->p->pid == -1 && queue_length(queues[index]) == 0) {
                    printf("\nQueue length is 0 and we found dummy\n");
                    insertToHead(&queues[index], &tails[index], cur);
                    pthread_mutex_unlock(locks[index]);
                    pthread_exit(0);
            }
            else if (cur->p->pid == -1) {
                insertToEnd(&queues[index], &tails[index], cur);
                pthread_mutex_unlock(locks[index]);
                pthread_exit(0);
            }
            pthread_mutex_unlock(locks[index]);
            
            if (strcmp(args->algorithm, "RR") == 0) {
                if (cur->p->remaining_time <= args->q) {
                    usleep(cur->p->remaining_time);
                    burstFinished = 1;
                }
                else {
                    usleep(args->q);
                    // update remaining time and add to tail
                    cur->p->remaining_time = cur->p->remaining_time - args->q;
                    pthread_mutex_lock(locks[index]);
                    insertToEnd(&queues[index], &tails[index], cur);
                    pthread_mutex_unlock(locks[index]);
                }
            }
            else {
                struct timeval burstStartTime;
                usleep(cur->p->burst_length);
                burstFinished = 1;
            }
            if (burstFinished) {
                // update information of process
                struct Process* p = cur->p;
                struct timeval burstFinishTime;
                gettimeofday(&burstFinishTime, NULL);
                pthread_mutex_lock(startLock);
                p->finish_time = timeval_diff_ms(&start, &burstFinishTime);
                pthread_mutex_unlock(startLock);
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_length;
                p->remaining_time = 0;
                p->processor_id = args->index;

                cur->prev = NULL;
                cur->next = NULL;

                // add to finished processes list
                pthread_mutex_lock(doneProcessesLock);
                insertAsc(&doneProcessesHead, &doneProcessesTail, cur); // could be adding to turnaround time (not sure)
                pthread_mutex_unlock(doneProcessesLock);
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

    // Burst will be generated random if random > 0, read file if random == 0
    int random = 1;

    // Random variables
    int iat_mean = 200;
    int iat_min = 10;
    int iat_max = 1000;

    int burst_mean = 100;
    int burst_min = 10;
    int burst_max = 500;

    int pc = 10;

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
            random--;
            infile_name = argv[i + 1];
        }
        else if (strcmp(argv[i], "-m") == 0) {
            out_mode = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-o") == 0) {
            outfile_name = argv[i + 1];
        }
        else if (strcmp(argv[i], "-r") == 0) {
            random++;
            iat_mean = atoi(argv[i + 1]);
            iat_min = atoi(argv[i + 2]);
            iat_max = atoi(argv[i + 3]);

            burst_mean = atoi(argv[i + 4]);
            burst_min = atoi(argv[i + 5]);
            burst_max = atoi(argv[i + 6]);

            pc = atoi(argv[i + 7]);
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

    printf("Iat variables %d %d %d\n", iat_mean, iat_min, iat_max);
    printf("Burst variables %d %d %d\n", burst_mean, burst_min, burst_max);

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

    doneProcessesHead = NULL;
    doneProcessesTail = NULL;
    doneProcessesLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    startLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(doneProcessesLock, NULL);
    pthread_mutex_init(startLock, NULL);
    
    struct Arguments args[num_of_processors];
    // Create processor threads
    pthread_t threads[num_of_processors];
    for (int i = 0; i < num_of_processors; i++){
        args[i].index = i;
        if (strcmp(scheduling_approach,"S") == 0)
            args[i].single = 1;
        else 
            args[i].single = 0;
            
        args[i].algorithm = algorithm;
        args[i].q = quantum;
        pthread_create(&threads[i], NULL, process_thread, (void*) &args[i]);
    }

    if (random == 0) {
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

        int first = 1;
        
        while ((read = getline(&line, &len, fp)) != -1) {
            if (first) {
                // Get and record start time
                pthread_mutex_lock(startLock);
                gettimeofday(&start, NULL);
                pthread_mutex_unlock(startLock);
                first = 0;
            }
            
            struct timeval arrival;
            gettimeofday(&arrival, NULL);
            
            // Process the line
            strtok(line, " ");
            char* burst = strtok(NULL, " ");
            int burst_l = atoi(burst);

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
                    q_index = cur_id % num_of_processors;
                }
                else if (strcmp(queue_selection_method,"LM") == 0) {
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
            strtok(line, " ");
            char* inter_arrival = strtok(NULL, " ");
            int iat = atoi(inter_arrival);
            cur_id++;
            usleep(iat);
        }
        // Free the memory allocated for the line
        free(line);

        // Close the file
        fclose(fp);
    }
    else { // Random
        int cur_id = 0;
        int count = 0;

        // Get and record start time
        pthread_mutex_lock(startLock);
        gettimeofday(&start, NULL);
        pthread_mutex_unlock(startLock);
        while (count < pc) {
            struct timeval arrival;
            gettimeofday(&arrival, NULL);

            // Generate random int
            int burst_l = generateRandomInt(burst_min, burst_max, burst_mean);

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
                    q_index = cur_id % num_of_processors;
                }
                else if (strcmp(queue_selection_method,"LM") == 0) {
                    q_index = findLeastLoad(num_of_processors);
                }
                pthread_mutex_lock(locks[q_index]);
                insertToEnd(&queues[q_index],&tails[q_index],createNode(p));
                pthread_mutex_unlock(locks[q_index]);
            }

            int iat = generateRandomInt(iat_min, iat_max, iat_mean);
        
            cur_id++;

            usleep(iat);
            count++;
        }
    }

    // Add dummy Nodes
    if (strcmp(scheduling_approach, "S") == 0) {
        Process *p = (Process*)malloc(sizeof(struct Process));
        p->pid = -1;
        p->burst_length = 10000;
        p->remaining_time = 10000;
        p->arrival_time = 10000;
        Node* dummy = createNode(p);
        pthread_mutex_lock(locks[0]);
        insertToEnd(&queues[0],&tails[0],dummy);
        pthread_mutex_unlock(locks[0]);
    }
    else {
        for (int i = 0; i < num_of_processors; i++) {
            Process *p = (Process*)malloc(sizeof(struct Process));
            p->pid = -1;
            p->burst_length = 10000;
            p->remaining_time = 10000;
            p->arrival_time = 10000;
            Node* dummy = createNode(p);
            pthread_mutex_lock(locks[i]);
            insertToEnd(&queues[i],&tails[i],dummy);
            pthread_mutex_unlock(locks[i]);
        }
    }

    // Wait for threads
    for (int i = 0; i < num_of_processors; i++) {
        printf("\nThread %d has finished", i);
        pthread_join(threads[i], NULL);
    }

    printf("\n---- DONE ----\n");
    displayList(doneProcessesHead);

    if (strcmp(scheduling_approach, "S") == 0) {
        printf("\n---- QUEUE ----\n");
        displayList(queues[0]);
    }
    else {
        for (int i = 0; i < num_of_processors; i++) {
            printf("\n---- QUEUE %d ----\n", i);
            displayList(queues[i]);
        }
    }

    // Output results to console

    printf("%-10s %-10s %-10s %-10s %-10s %-12s %-10s\n", "pid", "cpu", "bustlen", "arv", "finish", "waitingtime", "turnaround");
    struct Node* cur = doneProcessesHead;
    while (cur != NULL) {
        struct Process* p = cur->p;
        printf("%-10d %-10d %-10d %-10d %-10d %-12d %-10d\n", p->pid, p->processor_id, p->burst_length, p->arrival_time, p->finish_time, p->waiting_time, p->turnaround_time);
        cur = cur->next;
    }

    // Free memory
    if (strcmp(scheduling_approach, "S") == 0) {
        freeQueue(queues[0]);
        free(locks[0]);
    }
    else {
        for (int i = 0; i < num_of_processors; i++) {
            freeQueue(queues[i]);
            free(locks[i]);
        }
    }

    freeQueue(doneProcessesHead);
    free(doneProcessesLock);
    
    free(queues);
    free(tails);
    free(locks);

    return 0;
}



