#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <queue>
#include <sys/time.h>
#include <unistd.h>

class Process {
  public:
    int id;                     /* the process ID */
    int initQueueLevel;         /* the level of the first queue the process was assigned to */
    int currQueueLevel;         /* the level of the queue the process is currently in */
    int burstTime;              /* the duration of the CPU burst for the process */
    double arrivalTime;         /* the time at which the process first joined a queue */
    double currArrivalTime;     /* the time at which the process joined the current queue */
    double finishTime;          /* the time at which the process finished its CPU burst execution */
};

/* comparator used in the declaration of the priority queue (max heap by default) for the SJF queues */
class Compare {
  public:
    bool operator() (Process p1, Process p2) {
      if (p1.burstTime > p2.burstTime) {
        return true;
      }
      else {
        return false;
      }
    }
};

/* the multi-level feedback queues */
std::queue <Process> q1;
std::priority_queue <Process, std::vector<Process>, Compare> q2;
std::priority_queue <Process, std::vector<Process>, Compare> q3;
std::queue <Process> q4;

/* global variables to store the start time and end time of the program */
double st, et;

/* the input parameters from the command line */
char* inputFileName;      /* the file containing the input parameters for the different processes */
char* outputFileName;     /* the file into which the output logs should be printed */
int tq;                   /* the time quantum for the Round Robin scheduling algorithm */
long threshold;           /* the threshold time beyond which a process waiting in a lower queue
                             can be upgraded to the immediate higher queue */

/* to print the elements of a queue */
void printQueue(std::queue <Process> q) {
  while (!q.empty()) {
    std::cout << q.front().id << " " << q.front().initQueueLevel << " " << q.front().burstTime << "\n";
    q.pop();
  }
  std::cout << "\n";
}

/* to print the elements of a priority queue */
void printPQueue(std::priority_queue <Process, std::vector<Process>, Compare> q) {
  while (!q.empty()) {
    std::cout << q.top().id << " " << q.top().initQueueLevel << " " << q.top().burstTime << "\n";
    q.pop();
  }
  std::cout << "\n";
}

/* to read the information about different processes from the input file, and store them
   in the corresponding queues */
void addProcessesToQueue() {
  FILE* fp;
  fp = fopen(inputFileName, "r");

  /* checking for any error in opening the file */
  if (fp == NULL) {
    printf("Error in opening file %s\n", inputFileName);
    exit(1);
  }

  char delim[] = " \t";
  char line[100];
  /* reading from file */
  while (fgets(line, 100, fp) != NULL) {
    int arr[3];
    int i = 0;
    /* using strtok to store the different fields of the process in an array */
    char *ptr = strtok(line, delim);
    while (ptr != NULL) {
      arr[i] = atoi(ptr);
      i++;
      ptr = strtok(NULL, delim);
    }
    /* creating a Process object using the parameters stored in the array */
    Process p;
    p.id = arr[0];
    p.initQueueLevel = arr[1];
    p.currQueueLevel = arr[1];
    p.burstTime = arr[2];
    p.arrivalTime = 0;
    p.currArrivalTime = 0;
    /* adding the Process to the corresponding queue */
    switch (arr[1]) {
      case 1:
            q1.push(p);
            break;
      case 2:
            q2.push(p);
            break;
      case 3:
            q3.push(p);
            break;
      case 4:
            q4.push(p);
            break;
      default:
            std::cout << "Invalid\n";
            exit(0);
    }
  }
  fclose(fp);
}

/* to check whether any of the processes at the heads of the queues 2,3,4 have been waiting
   for more time than the threshold time, since their arrival into their respective queues */
void checkThreshold() {
  Process p;
  double tempT;
  struct timeval temp_time;
  gettimeofday(&temp_time, NULL);
  tempT = (temp_time.tv_sec * 1000000 + temp_time.tv_usec +0.0)/1000;

  if (!q2.empty()) {
    p = q2.top();
    if (tempT - p.currArrivalTime - st > threshold) {
      q2.pop();
      p.currArrivalTime = tempT;
      p.currQueueLevel = 1;
      q1.push(p);
    }
  }

  if (!q3.empty()) {
    p = q3.top();
    if (tempT - p.currArrivalTime - st > threshold) {
      q3.pop();
      p.currArrivalTime = tempT;
      p.currQueueLevel = 2;
      q2.push(p);
    }
  }

  if (!q4.empty()) {
    p = q4.front();
    if (tempT - p.currArrivalTime - st > threshold) {
      q4.pop();
      p.currArrivalTime = tempT;
      p.currQueueLevel = 3;
      q3.push(p);
    }
  }
}

/* the driver code, to take inputs from the user, and simulate a process scheduler's functionality */
int main(int argc, char* argv[]) {
  /* taking the command line arguments from the user */
  if (argc != 9) {
    std::cout << "Incorrect number of command line arguments, expected 9, received " << argc << "\n";
    exit(0);
  }

  int i = 1;
  char opt;
  while (i <= 7) {
    opt = argv[i][1];
    switch (opt) {
      case 'Q':
        tq = atoi(argv[i+1]);
        if (tq < 10 || tq > 20) {
          std::cout << "Expected time quantum value to be in the range 10 - 20, but received " << tq << "\n";
          exit(0);
        }
        break;

      case 'T':
        threshold = atoi(argv[i+1]);
        if (threshold < 100 || threshold > 50000) {
          std::cout << "Expected threshold value to be in the range 100 - 50000, but received " << threshold << "\n";
          exit(0);
        }
        break;

      case 'F':
        inputFileName = (char*) malloc(100*sizeof(char));
        strcpy(inputFileName, argv[i+1]);
        break;

      case 'P':
        outputFileName = (char*) malloc(100*sizeof(char));
        strcpy(outputFileName, argv[i+1]);
        break;

      default:
        std::cout << "Incorrect option -" << opt << "\n";
        exit(0);
    }

    i = i + 2;
  }

  /* reading the process information from the input file, and storing them in the respective queues */
  addProcessesToQueue();

  /* opening the output logs file */
  FILE* fp;
  fp = fopen(outputFileName, "a+");

  Process p;

  /* the total number of processes that need to be scheduled/run */
  int num_proc = q1.size() + q2.size() + q3.size() + q4.size();

  /* to store the sum of TATs, to compute mean TAT at the end */
  double sum_tat = 0.0;

  /* a variable to keep track of the total number of processes currently waiting to be scheduled/run */
  int flag = num_proc;

  struct timeval start_time, temp_time, end_time;

  /* taking note of the start time of the program */
  gettimeofday(&start_time, NULL);
  st = (start_time.tv_sec * 1000000 + start_time.tv_usec + 0.0)/1000;

  /* the process scheduler runs as long as there are processes left to be scheduled/run
     on the CPU, and each time a process finishes its CPU burst */

  /* while there are still processes left to be scheduled */
  while (flag > 0) {
    /* while there are processes in queue 1 (Round Robin) left to be scheduled */
    while (!q1.empty()) {
      p = q1.front();
      q1.pop();
      /* if the time quantum for RR is less than the burst time (left) for the process... */
      if (tq < p.burstTime) {
        /* ...the process "runs on the CPU" for time quantum duration */
        usleep(tq*1000);
        /* the process is pushed back onto the tail of the queue, with modified leftover burst time */
        p.burstTime -= tq;
        q1.push(p);
        /* each time a process finishes its CPU burst, the scheduler checks if any processes
           have been waiting for too long */
        checkThreshold();
      }
      /* if the burst time (left) for the process is less than the time quantum for RR... */
      else {
        /* ...the process "runs on the CPU" for burst duration */
        usleep(p.burstTime*1000);
        /* now the process has finished its CPU burst */
        gettimeofday(&temp_time, NULL);
        p.finishTime = (temp_time.tv_sec * 1000000 + temp_time.tv_usec +0.0)/1000;
        fprintf(fp, "ID: %-5d; Orig. Level: %-5d; Final Level: %-5d; Comp. Time(ms): %-7.2lf; TAT(ms): %-7.2lf\n", p.id, p.initQueueLevel, p.currQueueLevel, p.finishTime, (p.finishTime - st));
        fprintf(stdout, "ID: %-5d; Orig. Level: %-5d; Final Level: %-5d; Comp. Time(ms): %-7.2lf; TAT(ms): %-7.2lf\n", p.id, p.initQueueLevel, p.currQueueLevel, p.finishTime, (p.finishTime - st));
        sum_tat += (p.finishTime - st);
        /* each time a process finishes its CPU burst, the scheduler checks if any processes
           have been waiting for too long */
        checkThreshold();
      }
    }

    /* while there are no processes left in queue 1, but there are processes in queue 2
       (Shortest Job First) left to be scheduled */
    while (q1.empty() && !q2.empty()) {
      p = q2.top();
      q2.pop();
      /* the priority queue implementation ensures that the process with the next shortest
         burst time is now at the head of the queue; so, this process gets scheduled */
      usleep(p.burstTime*1000);
      /* now the process has finished its CPU burst */
      gettimeofday(&temp_time, NULL);
      p.finishTime = (temp_time.tv_sec * 1000000 + temp_time.tv_usec +0.0)/1000;
      fprintf(fp, "ID: %-5d; Orig. Level: %-5d; Final Level: %-5d; Comp. Time(ms): %-7.2lf; TAT(ms): %-7.2lf\n", p.id, p.initQueueLevel, p.currQueueLevel, p.finishTime, (p.finishTime - st));
      fprintf(stdout, "ID: %-5d; Orig. Level: %-5d; Final Level: %-5d; Comp. Time(ms): %-7.2lf; TAT(ms): %-7.2lf\n", p.id, p.initQueueLevel, p.currQueueLevel, p.finishTime, (p.finishTime - st));
      sum_tat += (p.finishTime - st);
      /* each time a process finishes its CPU burst, the scheduler checks if any processes
         have been waiting for too long */
      checkThreshold();
    }

    /* while there are no processes left in queue 1 and queue 2, but there are processes
       in queue 3 (Shortest Job First) left to be scheduled */
    while (q1.empty() && q2.empty() && !q3.empty()) {
      p = q3.top();
      q3.pop();
      /* the priority queue implementation ensures that the process with the next shortest
         burst time is now at the head of the queue; so, this process gets scheduled */
      usleep(p.burstTime*1000);
      /* now the process has finished its CPU burst */
      gettimeofday(&temp_time, NULL);
      p.finishTime = (temp_time.tv_sec * 1000000 + temp_time.tv_usec +0.0)/1000;
      fprintf(fp, "ID: %-5d; Orig. Level: %-5d; Final Level: %-5d; Comp. Time(ms): %-7.2lf; TAT(ms): %-7.2lf\n", p.id, p.initQueueLevel, p.currQueueLevel, p.finishTime, (p.finishTime - st));
      fprintf(stdout, "ID: %-5d; Orig. Level: %-5d; Final Level: %-5d; Comp. Time(ms): %-7.2lf; TAT(ms): %-7.2lf\n", p.id, p.initQueueLevel, p.currQueueLevel, p.finishTime, (p.finishTime - st));
      sum_tat += (p.finishTime - st);
      /* each time a process finishes its CPU burst, the scheduler checks if any processes
         have been waiting for too long */
      checkThreshold();
    }

    /* while there are no processes left in queues 1, 2 and 3, but there are processes
       in queue 4 (First Come First Served) left to be scheduled */
    while (q1.empty() && q2.empty() && q3.empty() && !q4.empty()) {
      p = q4.front();
      q4.pop();
      /* the process that arrived first is at the head of the queue; so, as per the FCFS
         scheme, this process gets scheduled */
      usleep(p.burstTime*1000);
      /* now the process has finished its CPU burst */
      gettimeofday(&temp_time, NULL);
      p.finishTime = (temp_time.tv_sec * 1000000 + temp_time.tv_usec +0.0)/1000;
      fprintf(fp, "ID: %-5d; Orig. Level: %-5d; Final Level: %-5d; Comp. Time(ms): %-7.2lf; TAT(ms): %-7.2lf\n", p.id, p.initQueueLevel, p.currQueueLevel, p.finishTime, (p.finishTime - st));
      fprintf(stdout, "ID: %-5d; Orig. Level: %-5d; Final Level: %-5d; Comp. Time(ms): %-7.2lf; TAT(ms): %-7.2lf\n", p.id, p.initQueueLevel, p.currQueueLevel, p.finishTime, (p.finishTime - st));
      sum_tat += (p.finishTime - st);
      /* each time a process finishes its CPU burst, the scheduler checks if any processes
         have been waiting for too long */
      checkThreshold();
    }

    /* updating the number of processes still left to be scheduled/run */
    flag = q1.size() + q2.size() + q3.size() + q4.size();
  }

  /* taking note of the end time of the program */
  gettimeofday(&end_time, NULL);
  et = (end_time.tv_sec * 1000000 + end_time.tv_usec + 0.0)/1000;

  /* calculating the mean turnaround time and throughput */
  fprintf(fp, "Mean Turnaround Time: %-5.2lf (ms); Throughput: %-5.2lf (processes/sec)\n", (sum_tat/num_proc), (num_proc*1000.0)/(et-st));
  fprintf(stdout, "Mean Turnaround Time: %-5.2lf (ms); Throughput: %-5.2lf (processes/sec)\n", (sum_tat/num_proc), (num_proc*1000.0)/(et-st));

  fprintf(fp, "\n\n");
  fclose(fp);

  return 0;
}
