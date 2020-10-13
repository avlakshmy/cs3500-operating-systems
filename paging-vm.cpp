#include <cstdio>
#include <iostream>
#include <cmath>
#include <map>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <queue>
#include <sys/time.h>
#include <unistd.h>
#include <chrono>
#include <ctime>

#define MAX_SIZE 1000      /* assuming the max number of pages that a process may occupy is 1000 */
#define MEM_LIMIT 16777216 /* assuming 16 MB is the max main/virtual memory size */
#define MAX_PROC 1000      /* assuming that a maximum of 1000 processes can be run in one session */

/* the input parameters from the command line */
int M;    /* main memory size in KB */
int V;    /* virtual memory size in KB  */
int P;    /* page size in bytes */

int MMF;  /* number of main memory frames */
int VMF;  /* number of virtual memory frames */

int globalPIDctr = 0;   /* global counter to keep track of the PIDs */

int lastRunPID[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; /* keeps track of the last 10 processes that were run */
int lastRunPIDind = 9;

uint8_t mainMemory[MEM_LIMIT]; /* array that stores the value in each byte in main memory */

uint8_t virtualMemory[MEM_LIMIT]; /* array that stores the value in each byte in virtual memory */

/* a class that represents a page table entry */
class PageTableEntry {
  public:
    int MMFNumber;       /* main memory frame number corresponding to this page */
    int VMFNumber;       /* virtual memory frame number corresponding to this page */
};

/* bit vector to maintain free frames in main memory */
char freeFrames[MAX_SIZE];

/* counter to keep track of the total number of free frames in main memory */
int numFreeFrames;

/* bit vector to maintain free frames in virtual memory */
char vmFreeFrames[MAX_SIZE];

/* counter to keep track of the total number of free frames in virtual memory */
int numVMFreeFrames;

/* a class that represents an executable */
class Executable {
  public:
    char* fileName;                            /* name of the file corresponding to the executable */
    int size;                                  /* size of the executable */
    int pid;                                   /* process ID assigned to the executable */
    int base;                                  /* base address in main memory where it is loaded */
    int numPages;                              /* number of pages for this executable */
    int isInMain;                              /* flag that tells us whether this process is in main memory or not */
    int isInVirtual;                           /* flag that tells us whether this process is in virtual memory or not */
    std::map <int, uint8_t> memoryMap;         /* memory map for the variables and values in this executable */
    std::vector <PageTableEntry> pageTable;    /* page table for this executable/process */

    /* a function that initialises all the parameters for this process when it is loaded into main memory / virtual memory*/
    void initialise(const char* fn, int pti[], int procId, int isM, int isV) {
      // the executable is now loaded into main memory / virtual memory
      isInMain = isM;
      isInVirtual = isV;

      // setting the process ID for this executable
      pid = procId;

      // setting the file name corresponding to this executable
      fileName = (char*) malloc(50*sizeof(char));
      strcpy(fileName, fn);

      // setting the base address in main memory where this executable has been loaded
      base = pti[0]*P;

      // reading the file corresponding to this executable to extract size information
      FILE* fp;
      fp = fopen(fileName, "r");
      int x;
      fscanf(fp, "%d\n", &x);
      size = x*1024;
      fclose(fp);

      // setting the number of pages for this executable, calculated from the size
      numPages = (int) ceil((size + 0.0)/P);

      // initialising the page table for this process, and the global frame table
      pageTable.resize(numPages);
      int i;
      if (isInMain == 1) {
        // if this process is loaded into main memory...
        i = 0;
        while (i < numPages) {
          // ... we have to initialise the page table with the main memory frame number
          pageTable[i].MMFNumber = pti[i];

          // and also update the main memory free frames bit vector
          freeFrames[pti[i]] = '0';

          i++;
        }
        numFreeFrames -= numPages;
      }
      else if (isInVirtual == 1) {
        // if this process is loaded into virtual memory...
        i = 0;
        while (i < numPages) {
          // ... we have to initialise the page table with the virtual memory frame number
          pageTable[i].VMFNumber = pti[i];

          // and also update the virtual memory free frames bit vector
          vmFreeFrames[pti[i]] = '0';

          i++;
        }
        numVMFreeFrames -= numPages;
      }
    }

    /* checks whether a given logical address is valid for the executable */
    int checkAddress(int addr) {
      return (addr <= size);
    }

    /* function to execute the add instruction for the executable */
    int add(int x, int y, int z) {
      uint8_t v1, v2, sum;
      // if x is a valid address...
      if (checkAddress(x)) {
        // take its value,...
        v1 = memoryMap[x];
        // and if y is also a valid address...
        if (checkAddress(y)) {
          // take its value too...
          v2 = memoryMap[y];
          // compute their sum
          sum = v1+v2;
          // if z is a valid address...
          if (checkAddress(z)) {
            // store the sum in the corresponding main memory byte, as well as in the memory map for the process
            memoryMap[z] = sum;
            mainMemory[base+z] = sum;
            std::cout << "Command: add " << x << ", " << y << ", " << z << "; ";
            std::cout << "Result: Value in addr " << x << " = " << (int) v1 << ", addr " << y << " = " << (int) v2 << ", addr " << z << " = " << (int) sum << "\n";
          }
          else {
            std::cout << "Invalid Memory Address " << z << " specified for process id " << pid << "\n";
            return 0;
          }
        }
        else {
          std::cout << "Invalid Memory Address " << y << " specified for process id " << pid << "\n";
          return 0;
        }
      }
      else {
        std::cout << "Invalid Memory Address " << x << " specified for process id " << pid << "\n";
        return 0;
      }
      return 1;
    }

    /* function to execute the sub instruction for the executable */
    int sub(int x, int y, int z) {
      uint8_t v1, v2, diff;
      // if x is a valid address...
      if (checkAddress(x)) {
        // take its value,...
        v1 = memoryMap[x];
        // and if y is also a valid address...
        if (checkAddress(y)) {
          // take its value too...
          v2 = memoryMap[y];
          // compute their difference
          diff = v1-v2;
          // if z is a valid address...
          if (checkAddress(z)) {
            // store the difference in the corresponding main memory byte, as well as in the memory map for the process
            memoryMap[z] = diff;
            mainMemory[base+z] = diff;
            std::cout << "Command: diff " << x << ", " << y << ", " << z << "; ";
            std::cout << "Result: Value in addr " << x << " = " << (int) v1 << ", addr " << y << " = " << (int) v2 << ", addr " << z << " = " << (int) diff << "\n";
          }
          else {
            std::cout << "Invalid Memory Address " << z << " specified for process id " << pid << "\n";
            return 0;
          }
        }
        else {
          std::cout << "Invalid Memory Address " << y << " specified for process id " << pid << "\n";
          return 0;
        }
      }
      else {
        std::cout << "Invalid Memory Address " << x << " specified for process id " << pid << "\n";
        return 0;
      }
      return 1;
    }

    /* function to execute the print instruction for the executable */
    int print(int x) {
      if (checkAddress(x)) {
        std::cout << "Command: print " << x << "; ";
        //check what to do if the address key doesn't exist
        //rigth now it is adding the key to map with the default value 0
        std::cout << "Result: Value in addr " << x << " = " << (int) memoryMap[x] << "\n";
      }
      else {
        std::cout << "Invalid Memory Address " << x << " specified for process id " << pid << "\n";
        return 0;
      }
      return 1;
    }

    /* function to execute the load instruction for the executable */
    int load(uint8_t a, int y) {
      // if y is a valid address...
      if (checkAddress(y)) {
        // store the value a in that address in main memory as well as the memory map for the process
        memoryMap[y] = a;
        mainMemory[base+y] = a;
        std::cout << "Command: load " << (int) a << ", " << y << "; ";
        std::cout << "Result: Value of " << (int) a << " is now stored in addr " << y << "\n";
      }
      else {
        std::cout << "Invalid Memory Address " << y << " specified for process id " << pid << "\n";
        return 0;
      }
      return 1;
    }

    /* function to execute the run instruction for the executable */
    void run() {
      // open the file corresponding to this executable
      FILE* fp;
      fp = fopen(fileName, "r");
      int temp;
      int isValid;
      char buffer[100];
      char* context;
      char* remainder;

      // first, scan the size value from the file
      fscanf(fp, "%d\n", &temp);

      std::cout << "\n";
      // then, proceed to read the instructions from the file line by line
      while (fgets(buffer, 100, fp)) {
        char* op;
        const char* delimiter = " ,";
        op = strtok_r (buffer, delimiter, &context);
        remainder = context;
        int v1, v2, v3;
        uint8_t v;
        char *s1, *s2, *s3;
        char* ctx;

        // for the add instruction
        if (strcmp(op, "add") == 0) {
          // extract the 3 parameters...
          s1 = strtok_r(remainder, delimiter, &ctx);
          s2 = strtok_r(NULL, delimiter, &ctx);
          s3 = ctx;
          v1 = atoi(s1);
          v2 = atoi(s2);
          v3 = atoi(s3);

          // ... and then call the add function
          isValid = add(v1, v2, v3);

          // if there is an invalid address specified
          if (isValid == 0) {
            break;
          }
        }

        // for the sub instruction
        else if (strcmp(op, "sub") == 0) {
          // extract the 3 parameters...
          s1 = strtok_r(remainder, delimiter, &ctx);
          s2 = strtok_r(NULL, delimiter, &ctx);
          s3 = ctx;
          v1 = atoi(s1);
          v2 = atoi(s2);
          v3 = atoi(s3);

          // ... and then call the sub function
          isValid = sub(v1, v2, v3);

          // if there is an invalid address specified
          if (isValid == 0) {
            break;
          }
        }

        // for the print instruction
        else if (strcmp(op, "print") == 0) {
          // extract the logical address parameter...
          v1 = atoi(remainder);

          // ... and then call the print function
          isValid = print(v1);

          // if an invalid address is specified
          if (isValid == 0) {
            break;
          }
        }

        // for the load instruction
        else if (strcmp(op, "load") == 0) {
          // extract the 2 parameters...
          s1 = strtok_r(remainder, delimiter, &ctx);
          s2 = ctx;
          v1 = atoi(s1);
          v = (uint8_t) v1; // convert the value to be loaded into an 8-bit unsigned integer
          v2 = atoi(s2);

          // ... and then call the load function
          isValid = load(v1, v2);

          // if an invalid address is specified
          if (isValid == 0) {
            break;
          }
        }
      }
      std::cout << "\n";
    }

    /* function to print the page table entries for the executable */
    void printPageTable(FILE* fp) {
      int i = 0;
      while (i < pageTable.size()) {
        fprintf(fp, "%5d %5d\n", i, pageTable[i].MMFNumber);
        i++;
      }
    }

    /* function to deallocate all main memory space assigned to the executable */
    void deallocateMem() {
      int i = 0;
      while (i < pageTable.size()) {
        // update the main memory free frames bit vector
        freeFrames[pageTable[i].MMFNumber] = '1';
        i++;
      }
      numFreeFrames += pageTable.size();
    }

    /* function to deallocate all virtual memory space assigned to the executable */
    void deallocateVirtualMem() {
      int i = 0;
      while (i < pageTable.size()) {
        // update the virtual memory free frames bit vector
        vmFreeFrames[pageTable[i].VMFNumber] = '1';
        i++;
      }
      numVMFreeFrames += pageTable.size();
    }
};

/* an array of executables */
Executable exec[MAX_PROC];

/* function that checks if string a is a subsequence of string b; if yes, it
   returns 1, and with the corresponding indices being stored in the array s

   Reference:
   https://www.geeksforgeeks.org/given-two-strings-find-first-string-subsequence-second/ */

int checkFreeFrames(const char a[], const char b[], int numFree, int s[]) {
  int i, j, k;
  i = j = k = 0;
  int m, n;
  m = strlen(a);
  n = strlen(b);

  // if the required number of frames is greater than the number of available free frames
  if (m > numFree) {
    return 0;
  }

  // traversing string b to get the indices of the free frames
  for (i = 0; i < n && j < m; i++) {
    if (a[j] == b[i]) {
      j++;
      s[k] = i;
      k++;
    }
  }

  return 1;
}

/* function that tries to load a given set of executable files into memory */
void load (std::vector <std::string> fileArr) {
  int i = 0;
  int n = fileArr.size();

  // trying to load the executables one by one in order
  for (i = 0; i < n; i++) {
    Executable e;
    FILE* fptemp;
    fptemp = fopen(fileArr[i].c_str(), "r");

    // firstly, checking if a valid file has been specified
    if (fptemp == NULL) {
      std::cout << fileArr[i].c_str() << " could not be loaded - file does not exist\n";
      continue;
    }
    else {
      int x;
      // scanning the size of the executable from the first line of the file
      fscanf(fptemp, "%d\n", &x);

      // calculating the number of pages that will be required to store the executable
      int s = (int) ceil(((x*1024) + 0.0)/P);
      fclose(fptemp);

      // array that will store the frame numbers where this process's pages can be accommodated
      int pti[s];

      // temporary array to help find the required number of free frames
      char str[s];
      int i1;
      for (i1 = 0; i1 < s; i1++) {
        str[i1] = '1';
      }
      str[i1] = '\0';

      // finding if there are adequate number of free frames in main memory to accommodate this process
      int tem = checkFreeFrames(str, freeFrames, numFreeFrames, pti);
      if (tem == 0) {
        // if not, checking the same in virtual memory
        str[i1] = '\0';
        tem = checkFreeFrames(str, vmFreeFrames, numVMFreeFrames, pti);
        if (tem == 0) {
          // if adequate space is not there in virtual memory as well
          std::cout << fileArr[i].c_str() << " could not be loaded - memory is full, or available memory is not of adequate size\n";
        }
        else {
          // if adequate space is there in virtual memory

          // generating a new process ID
          globalPIDctr++;

          // loading the process in virtual memory and initialising all its parameters
          e.initialise(fileArr[i].c_str(), pti, globalPIDctr, 0, 1);

          // storing the process in the list of processes
          exec[e.pid] = e;

          std::cout << e.fileName << " is loaded into virtual memory and is assigned process id: " << e.pid << "\n";
        }
      }
      else {
        // if adequate space is there in main memory

        // generating a new process ID
        globalPIDctr++;

        // loading the process in main memory and initialising all its parameters
        e.initialise(fileArr[i].c_str(), pti, globalPIDctr, 1, 0);

        // storing the process in the list of processes
        exec[e.pid] = e;

        std::cout << e.fileName << " is loaded into main memory and is assigned process id: " << e.pid << "\n";
      }
    }
  }
}

/* function that swaps out a specified process from main memory into virtual memory */
int swapout(int pid) {
  int s = exec[pid].numPages;
  // if the process id is valid, and it is in main memory
  if (pid >= 1 && pid <= globalPIDctr) {
    if (exec[pid].isInMain == 1 && exec[pid].isInVirtual == 1) {
      exec[pid].isInMain = 0;
      int i2;
      i2 = 0;
      while (i2 < s) {
        // update the main memory frames bit vector to reflect that those frames are now free
        freeFrames[exec[pid].pageTable[i2].MMFNumber] = '1';
        i2++;
      }
      numFreeFrames += s;
      std::cout << "\nProcess with pid " << pid << " is swapped out of main memory (but is already in virtual memory)\n";
    }
    else if (exec[pid].isInMain == 1 && exec[pid].isInVirtual == 0) {
      // we try to find a set of free frames in virtual memory to shift this process into
      int pti[s];
      char str[s];
      int i1;
      for (i1 = 0; i1 < s; i1++) {
        str[i1] = '1';
      }
      str[i1] = '\0';
      int tem = checkFreeFrames(str, vmFreeFrames, numVMFreeFrames, pti);
      if (tem == 0) {
        std::cout << "\nProcess with pid " << pid << " could not be swapped out - virtual memory is full, or available space is not adequate\n";
      }
      else {
        exec[pid].isInMain = 0;
        exec[pid].isInVirtual = 1;

        int i2;
        i2 = 0;
        while (i2 < s) {
          // update the main memory frames bit vector to reflect that those frames are now free
          freeFrames[exec[pid].pageTable[i2].MMFNumber] = '1';
          i2++;
        }
        numFreeFrames += s;

        i2 = 0;
        while (i2 < s) {
          exec[pid].pageTable[i2].VMFNumber = pti[i2];
          vmFreeFrames[pti[i2]] = '0';
          i2++;
        }
        numVMFreeFrames -= s;

        std::cout << "\nProcess with pid " << pid << " is swapped out to virtual memory\n";
        return 1;
      }
    }
    else {
      std::cout << "\nInvalid pid " << pid << "; please input a valid instruction.\n";
      return 0;
    }
  }
  else {
    std::cout << "\nInvalid pid " << pid << "; please input a valid instruction.\n";
    return 0;
  }
}

/* helper function for swapin */
void aux_swapin(int pid) {
  int s = exec[pid].numPages;
  int pti[s];
  char str[s];
  int i1;
  for (i1 = 0; i1 < s; i1++) {
    str[i1] = '1';
  }
  str[i1] = '\0';

  // now this process is in main memory
  exec[pid].isInMain = 1;

  // getting the frame numbers in main memory to store this process
  checkFreeFrames(str, freeFrames, numFreeFrames, pti);

  // updating the page table for the process and also the main memory free frames bit vector
  int i2 = 0;
  while (i2 < s) {
    exec[pid].pageTable[i2].MMFNumber = pti[i2];
    freeFrames[pti[i2]] = '0';
    i2++;
  }
  numFreeFrames -= s;

  std::cout << "\nProcess with pid " << pid << " is swapped in to main memory\n";
}

/* function that swaps in a specified process from virtual memory into main memory */
int swapin(int pid) {
  // if the process id is valid, and it is in virtual memory
  if (pid >= 1 && pid <= globalPIDctr && exec[pid].isInVirtual == 1 && exec[pid].isInMain == 0) {
    int s = exec[pid].numPages;

    // we try to find a set of s free frames in main memory to load this process into
    if (numFreeFrames < s) {
      // the number of free main memory frames is insufficient to accommodate this process
      if (lastRunPID[9] != -1) {
        // at least one process was run
        int j1 = lastRunPIDind;
        int j2 = (j1 + 1)%10;
        // go through all the processes which have been run, latest first
        while ((lastRunPID[j2] != -1) && (j2 != j1)) {
          // if there is enough space to swap out this process into virtual memory
          if ((exec[lastRunPID[j2]].isInMain == 1) && (numVMFreeFrames >= exec[lastRunPID[j2]].numPages)) {
            // swap it out
            swapout(lastRunPID[j2]);

            // now if the new process can be accommodated in main memory, we stop here...
            if (numFreeFrames >= exec[pid].numPages) {
              break;
            }
          }
          // ... otherwise, we continue to look at the next latest run process
          j2 = (j2 + 1)%10;
        }

        // if it wasn't possible to swap in the process still...
        if (j2 == j1 || (lastRunPID[j2] == -1)) {
          int j3 = 1;
          // we try to identify one last process in main memory whose swapping out can lead to
          // the current process to be swapped into main memory
          while (j3 <= globalPIDctr) {
            if ((exec[j3].isInMain == 1) && ((numFreeFrames + exec[j3].numPages) >= exec[pid].numPages) && (numVMFreeFrames >= exec[j3].numPages)) {
              break;
            }
            j3++;
          }
          // if we find such a process, we swap it out, and swap in the current process
          if (j3 <= globalPIDctr) {
            swapout(j3);
            aux_swapin(pid);
            return 1;
          }
          // if even that is not possible
          else {
            std::cout << "\nProcess with pid " << pid << " cannot be swapped in to main memory - main memory is full, or available space is not adequate\n";
            return 0;
          }
        }
        else {
          // now the process can be swapped in to main memory
          aux_swapin(pid);
          return 1;
        }
      }
      else {
        // no processes were run at all
        int j2 = 1;
        // we try to identify one process in main memory whose swapping out can lead to
        // the current process to be swapped into main memory
        while (j2 <= globalPIDctr) {
          if ((exec[j2].isInMain == 1) && ((numFreeFrames + exec[j2].numPages) >= exec[pid].numPages) && (numVMFreeFrames >= exec[j2].numPages)) {
            break;
          }
          j2++;
        }
        // if we find such a process, we swap it out, and swap in the current process
        if (j2 <= globalPIDctr) {
          swapout(j2);
          aux_swapin(pid);
          return 1;
        }
        // if that is not possible
        else {
          std::cout << "\nProcess with pid " << pid << " cannot be swapped in to main memory - main memory is full, or available space is not adequate\n";
          return 0;
        }
      }
    }
    else {
      // now we can swap in the current process into main memory
      aux_swapin(pid);
      return 1;
    }
  }
  else {
    if (pid > globalPIDctr || exec[pid].isInVirtual == 0) {
      std::cout << "\nInvalid pid " << pid << "; please input a valid instruction.\n";
      return 0;
    }
    if (exec[pid].isInMain == 1) {
      std::cout << "\nProcess with pid " << pid << " is already in main memory.\n";
      return 0;
    }
  }
}

/* driver code that manages the entire paging and virtual memory scheme */
int main(int argc, char* argv[]) {
  // taking the command line arguments from the user
  if (argc != 7) {
    std::cout << "Incorrect number of command line arguments, expected 7, received " << argc << "\n";
    exit(0);
  }

  int i = 1;
  char opt;
  while (i <= 5) {
    opt = argv[i][1];
    switch (opt) {
      case 'M':
        M = atoi(argv[i+1]);
        break;

      case 'V':
        V = atoi(argv[i+1]);
        break;

      case 'P':
        P = atoi(argv[i+1]);
        break;

      default:
        std::cout << "Incorrect option -" << opt << "\n";
        exit(0);
    }

    i = i + 2;
  }

  // calculating the number of memory frames, virtual memory frames
  MMF = (M*1024)/P;
  VMF = (V*1024)/P;

  int j;

  // initialising the main memory free frames bit vector to all free
  for (j = 0; j < MMF; j++) {
    freeFrames[j] = '1';
  }
  freeFrames[j] = '\0';
  numFreeFrames = MMF;

  // initialising the virtual memory free frames bit vector to all free
  for (j = 0; j < VMF; j++) {
    vmFreeFrames[j] = '1';
  }
  vmFreeFrames[j] = '\0';
  numVMFreeFrames = VMF;

  //executable command interpreter
  std::string fileString, str1, tok, t;
  std::cout << "\n<Command Please> ";
  while (std::getline(std::cin, fileString)) {
    std::stringstream s1(fileString);
    s1 >> tok;

    // exit command
    if (strcmp(tok.c_str(), "exit") == 0) {
      std::cout << "\nDeallocating all memory...\n";
      int i = 1;
      while (i <= globalPIDctr) {
        // deallocating all main memory allotted to any of the processes
        if (exec[i].isInMain == 1) {
          exec[i].deallocateMem();
          exec[i].isInMain = 0;
        }
        // deallocating all virtual memory allotted to any of the processes
        if (exec[i].isInVirtual == 1) {
          exec[i].deallocateVirtualMem();
          exec[i].isInVirtual = 0;
        }
        i++;
      }
      std::cout << "Exiting...\n";
      break;
    }

    // load command
    else if (strcmp(tok.c_str(), "load") == 0) {
      std::vector <std::string> temp;
      // creating an array of the file names to be loaded
      while (s1 >> tok) {
        temp.push_back(tok);
      }
      std::cout << "\n";
      load(temp);
    }

    // run command
    else if (strcmp(tok.c_str(), "run") == 0) {
      s1 >> tok;
      int pid = std::stoi(tok);
      if (pid >= 1 && pid <= globalPIDctr) {
        // if the process is in main memory, we can directly run the process
        if (exec[pid].isInMain == 1) {
          exec[pid].run();
        }
        // if the process is in virtual memory, we need to swap in the process to main memory
        else if (exec[pid].isInVirtual == 1) {
          int swin = swapin(pid);
          // only if the swap in was successful, we can run the process
          if (swin == 1) {
            exec[pid].run();
          }
        }

        else {
          std::cout << "\nInvalid pid " << pid << "; please input a valid instruction.\n";
        }
        // updating the latest run processes list
        lastRunPID[lastRunPIDind] = pid;
        lastRunPIDind = (lastRunPIDind - 1)%10;
      }
      else {
        std::cout << "\nInvalid pid " << pid << "; please input a valid instruction.\n";
      }
    }

    // kill command
    else if (strcmp(tok.c_str(), "kill") == 0) {
      s1 >> tok;
      int pid = std::stoi(tok);
      if (pid >= 1 && pid <= globalPIDctr) {
        // deallocating any main memory space allotted to the process
        if (exec[pid].isInMain == 1) {
          exec[pid].deallocateMem();
          exec[pid].isInMain = 0;
        }
        // deallocating any virtual memory space allotted to the process
        if (exec[pid].isInVirtual == 1) {
          exec[pid].deallocateVirtualMem();
          exec[pid].isInVirtual = 0;
        }
        std::cout << "\nKilled process with pid " << pid << "\n";
      }
      else {
        std::cout << "\nInvalid pid " << pid << "; please input a valid instruction.\n";
      }
    }

    // print command
    else if (strcmp(tok.c_str(), "print") == 0) {
      s1 >> tok;
      int memloc = std::stoi(tok);
      s1 >> tok;
      int len = std::stoi(tok);
      // checking if the requested memory addresses are valid physical addresses
      if (memloc >= 0 && ((memloc+len) < M*1024)) {
        int i = 0;
        std::cout << "\n";
        while (i < len) {
          std::cout << "Value of " << memloc+i << " is " << mainMemory[memloc+i] << "\n";
          i++;
        }
        std::cout << "\n";
      }
      else {
        std::cout << "\nInvalid memory access: aborting...\n";
      }
    }

    // listpr command
    else if (strcmp(tok.c_str(), "listpr") == 0) {
      int i = 1;
      std::cout << "\nProcesses in Main Memory:\n";
      while (i <= globalPIDctr) {
        // printing the identifier values of all the processes in main memory
        if (exec[i].isInMain == 1) {
          std::cout << "pid " << i << "\n";
          std::map <int, uint8_t> :: iterator itr;
          for (itr = exec[i].memoryMap.begin(); itr != exec[i].memoryMap.end(); itr++) {
            std::cout << itr->first << " " << (int) itr->second << "\n";
          }
          std::cout << "\n";
        }
        i++;
      }
      i = 1;
      std::cout << "\nProcesses in Virtual Memory:\n";
      while (i <= globalPIDctr) {
        // printing the identifier values of all the processes solely in virtual memory
        // (to avoid reprinting of identifier values of swapped-in processes among the above)
        if (exec[i].isInMain == 0 && exec[i].isInVirtual) {
          std::cout << "pid " << i << "\n";
          std::map <int, uint8_t> :: iterator itr;
          for (itr = exec[i].memoryMap.begin(); itr != exec[i].memoryMap.end(); itr++) {
            std::cout << itr->first << " " << (int) itr->second << "\n";
          }
          std::cout << "\n";
        }
        i++;
      }
      std::cout << "\n";
    }

    // pte command
    else if (strcmp(tok.c_str(), "pte") == 0) {
      s1 >> tok;
      int pid = std::stoi(tok);
      if (pid >= 1 && pid <= globalPIDctr) {
        if (exec[pid].isInMain == 1) {
          s1 >> tok;
          char* fn;
          fn = (char*) malloc(50*sizeof(char));
          strcpy(fn, tok.c_str());
          FILE* fpn;
          fpn = fopen(fn, "a+");
          auto timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
          fprintf(fpn, "%s\n", ctime(&timenow));
          fprintf(fpn, "%5s %5s\n", "Page", "Main Memory Frame");
          int k = 0;
          // printing the page table entries for this process
          while (k < exec[pid].pageTable.size()) {
            fprintf(fpn, "%5d %5d\n", k, exec[pid].pageTable[k].MMFNumber);
            k++;
          }
          fclose(fpn);
        }
        else if (exec[pid].isInMain == 0 && exec[pid].isInVirtual == 1) {
          std::cout << "\nProcess with pid " << pid << " is in virtual memory, hence not printing page table.\n";
        }
      }
      else {
        std::cout << "\nInvalid pid " << pid << "; please input a valid instruction.\n";
      }
    }

    // pteall command
    else if (strcmp(tok.c_str(), "pteall") == 0) {
      s1 >> tok;
      char* fn;
      fn = (char*) malloc(50*sizeof(char));
      strcpy(fn, tok.c_str());
      FILE* fpn;
      fpn = fopen(fn, "a+");
      auto timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      fprintf(fpn, "%s\n", ctime(&timenow));
      int i3 = 1;
      while (i3 <= globalPIDctr) {
        if (exec[i3].isInMain == 1) {
          fprintf(fpn, "Process ID %d\n", i3);
          fprintf(fpn, "%5s %5s\n", "Page", "Frame");
          int k = 0;
          // printing the page table entries for this process
          while (k < exec[i3].pageTable.size()) {
            fprintf(fpn, "%5d %5d\n", k, exec[i3].pageTable[k].MMFNumber);
            k++;
          }
          fprintf(fpn, "\n");
        }
        i3++;
      }
      fclose(fpn);
    }

    // swap out command
    else if (strcmp(tok.c_str(), "swapout") == 0) {
      s1 >> tok;
      int pid = std::stoi(tok);
      swapout(pid);
    }

    // swap in command
    else if (strcmp(tok.c_str(), "swapin") == 0) {
      s1 >> tok;
      int pid = std::stoi(tok);
      swapin(pid);
    }

    // default case
    else {
      std::cout << "Invalid command... Please enter a valid command\n";
    }

    std::cout << "\n<Command Please> ";
  }
  return 0;
}
