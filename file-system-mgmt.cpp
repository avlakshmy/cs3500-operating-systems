#include <cstdio>
#include <iostream>
#include <cmath>
#include <map>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <time.h>

#define DISK_BLOCK_SIZE 128

/* the input parameters from the command line */
int D;                                /* diskfile size in KB */
char* disk_file;                      /* name of the diskfile */
int numBlocks;                        /* number of blocks in the disk file */
std::vector <bool> freeBitVector;     /* free space manager */
int freeInd;                          /* starting block number of the free bit vector on disk */

/* Reference: https://www.geeksforgeeks.org/c-program-find-size-file/ */
int getFileSize(const char* filename) {
  // opening the file in read mode
  FILE* fp = fopen(filename, "r");

  // checking if the file exists or not
  if (fp == NULL) {
    std::cout << "File not found, exiting...\n";
    return -1;
  }

  fseek(fp, 0L, SEEK_END);

  // calculating the size of the file
  long res = ftell(fp);

  // closing the file
  fclose(fp);

  return (int)res;
}

/* a class that represents a disk block */
class DiskBlock {
  public:
    int blockAddr[32];
};

/* a class that represents an inode block */
class Inode {
  public:
    int isFree;                                 // whether free or not
    uint8_t type;                               // type: whether it is a directory or a file
    char filename[16];                          // name of the file, if it is a file
    int readPtr;                                // read pointer for this file
    int writePtr;                               // weite pointer for this file
    char dirname[4];                            // name of the directory, if it is a directory
    int blockNumber;                            // block number for the directory
    int size;                                   // size of the file
    char date_created[28];                      // date/time when the file was created
    char date_modified[28];                     // date/time when the file was last modified
    int direct_block_addresses[3];              // addresses of the first three data blocks for this file
    int index_block;                            // address of the index block for this file
    int index_block_contents[32];               // the index block can contain the addresses of at max 32 data blocks

    /* prints the inode contents of the file */
    void printInode() {
      int i;
      std::cout << "\n";
      std::cout << "***************************\n";
      std::cout << "Filename: " << filename << "\n";
      std::cout << "Size: " << size << " bytes\n";
      std::cout << "Date Created: " << date_created;
      std::cout << "Date Last Modified: " << date_modified;
      std::cout << "Direct block values: ";
      for (i = 0; i < 3; i++) {
        std::cout << direct_block_addresses[i] << " ";
      }
      std::cout << "\n";
      std::cout << "Index block is stored in: " << index_block << "\n";
      std::cout << "Index block contents: ";
      printIndexBlockContents();
      std::cout << "\n";
      std::cout << "***************************\n\n";
    }

    /* prints the inode contents of the directory */
    void printInodeD() {
      int i;
      std::cout << "\n";
      std::cout << "***************************\n";
      std::cout << "Dirname: " << dirname << "\n";
      std::cout << "Date Created: " << date_created;
      std::cout << "Date Last Modified: " << date_modified;
      std::cout << "\n";
      std::cout << "***************************\n\n";
    }

    /* prints the valid index block contents */
    void printIndexBlockContents() {
      int i = 0;
      while (index_block_contents[i] != -1) {
        std::cout << index_block_contents[i] << " ";
        i++;
      }
      std::cout << "\n";
    }
};

int numInodeBlocks;
std::vector <Inode> inodes;
int ibIndex = 0;

int getNextFreeBlockIndex() {
  int i = numInodeBlocks;
  while (i < numBlocks) {
    if (freeBitVector[i]) {
      freeBitVector[i] = false;
      return i;
    }
    i++;
  }
  return -1;
}

void createInodeEntryAndStoreFileOnDisk() {
  char temp[DISK_BLOCK_SIZE];
  int pos;
  FILE* fpd = fopen(disk_file, "r+");
  sprintf(temp, "{%s}", inodes[ibIndex].filename);
  sprintf((temp + strlen(temp)), "{%d}", inodes[ibIndex].size);
  sprintf((temp + strlen(temp)), "{%s}", inodes[ibIndex].date_created);
  sprintf((temp + strlen(temp)), "{%s}", inodes[ibIndex].date_modified);


  FILE* fp = fopen(inodes[ibIndex].filename, "r");
  if (fp != NULL) {
    int s = inodes[ibIndex].size;
    int fbi;
    int i = 0;
    char temp1[DISK_BLOCK_SIZE + 1];
    while (s > 0 && i < 3) {
      memset(temp1, 0, DISK_BLOCK_SIZE);
      fbi = getNextFreeBlockIndex();
      if (fbi == -1) {
        std::cout << "No free blocks available...\n";
        break;
      }
      inodes[ibIndex].direct_block_addresses[i] = fbi;
      i++;
      fread(temp1, 1, DISK_BLOCK_SIZE, fp);
      pos = fbi * DISK_BLOCK_SIZE;
      fseek(fpd, pos, SEEK_SET);
      fprintf(fpd, "%s", temp1);
      s -= DISK_BLOCK_SIZE;
    }
    if (s > 0) {
      // the data extends beyond 3 direct data blocks
      fbi = getNextFreeBlockIndex();
      if (fbi == -1) {
        std::cout << "No free blocks available...\n";
        return;
      }
      inodes[ibIndex].index_block = fbi;
      int k = 0;
      while(k < 32) {
        inodes[ibIndex].index_block_contents[k] = -1;
        k++;
      }
      int numBReq = (int)ceil((s+0.0)/128);
      k = 0;
      while (s > 0 && k < numBReq) {
        fbi = getNextFreeBlockIndex();
        if (fbi == -1) {
          std::cout << "No free blocks available...\n";
          break;
        }
        inodes[ibIndex].index_block_contents[k] = fbi;
        memset(temp1, 0, DISK_BLOCK_SIZE);
        fread(temp1, 1, DISK_BLOCK_SIZE, fp);
        pos = fbi * DISK_BLOCK_SIZE;
        fseek(fpd, pos, SEEK_SET);
        fprintf(fpd, "%s", temp1);
        s -= DISK_BLOCK_SIZE;
        k++;
      }
    }

    // this is the information about the direct block addresses
    sprintf((temp + strlen(temp)), "{%d,%d,%d}", inodes[ibIndex].direct_block_addresses[0], inodes[ibIndex].direct_block_addresses[1], inodes[ibIndex].direct_block_addresses[2]);

    // print the inode information in the corresponding inode block
    pos = ibIndex * DISK_BLOCK_SIZE;
    fseek(fpd, pos, SEEK_SET);
    fprintf(fpd, "%s", temp);

    ibIndex++;

    fclose(fp);
  }

  fclose(fpd);
}

void createInodeEntryAndStoreDirOnDisk() {
  char temp[DISK_BLOCK_SIZE];
  int pos;
  FILE* fpd = fopen(disk_file, "r+");
  sprintf(temp, "{%s}", inodes[ibIndex].dirname);
  sprintf(temp, "{%d}", inodes[ibIndex].blockNumber);
  sprintf((temp + strlen(temp)), "{%s}", inodes[ibIndex].date_created);
  sprintf((temp + strlen(temp)), "{%s}", inodes[ibIndex].date_modified);

  // print the inode information in the corresponding inode block
  pos = ibIndex * DISK_BLOCK_SIZE;
  fseek(fpd, pos, SEEK_SET);
  fprintf(fpd, "%s", temp);

  ibIndex++;

  fclose(fpd);
}

/* checks if the given directory name is valid */
int checkDirName(const char* dirname) {
  if (strlen(dirname) > 4) {
    return 0;
  }
  return 1;
}

/* checks if the given file name is valid */
int checkFileName(const char* filename) {
  if (strlen(filename) > 16) {
    return 0;
  }
  return 1;
}

/* if the filename is valid and the file is found, returns the inode number; else returns -1 */
int getInode(const char* filename) {
  int i = 0;
  while (i < numInodeBlocks) {
    if (strcmp(inodes[i].filename, filename) == 0 && inodes[i].isFree == 0) {
      return i;
    }
    else if (strcmp(inodes[i].dirname, filename) == 0 && inodes[i].isFree == 0) {
      return i;
    }
    i++;
  }
  return -1;
}

void openfile(const char* filename) {
  /*Semantics: If the file does not exist, creates the file with name specified by filename.
  Upon creation, a file has size of 0 bytes. The file is opened for both reading and writing.
  Set the read and write pointers for this file to 0.
  If the file already exists, open the file for reading and writing. Set the read and write
  pointers for this file to 0.
  Output: Echo the command to the screen and the result, as in:
  COMMAND: create /file1.c ; RESULT: Created /file1.c*/

  // first checking if the file already exists
  int i = getInode(filename);
  if (i == -1) {
    // if the file does not exist, we create a new file with 0 size
    time_t tim;
    inodes[ibIndex].type = 1;
    inodes[ibIndex].readPtr = 0;
    inodes[ibIndex].writePtr = 0;
    strcpy(inodes[ibIndex].filename, filename);
    inodes[ibIndex].size = 0;
    inodes[ibIndex].isFree = 0;
    time(&tim);
    strcpy(inodes[ibIndex].date_created, ctime(&tim));
    strcpy(inodes[ibIndex].date_modified, ctime(&tim));
    inodes[ibIndex].index_block = -1;
    int k = 0;
    while (k < 3) {
      inodes[ibIndex].direct_block_addresses[k] = -1;
      k++;
    }
    k = 0;
    while(k < 32) {
      inodes[ibIndex].index_block_contents[k] = -1;
      k++;
    }
    createInodeEntryAndStoreFileOnDisk();
  }
  else {
    inodes[i].readPtr = 0;
    inodes[i].writePtr = 0;
  }
  std::cout << "Opened file \"" << filename << "\"\n";
}

void createdir(const char* dirname) {
  int i = getInode(dirname);
  if (i != -1) {
    std::cout << "Directory already present...\n";
  }
  else {
    time_t tim;
    inodes[ibIndex].type = 0;
    inodes[ibIndex].readPtr = -1;
    inodes[ibIndex].writePtr = -1;
    inodes[ibIndex].isFree = 0;
    inodes[ibIndex].blockNumber = getNextFreeBlockIndex();
    strcpy(inodes[ibIndex].dirname, dirname);
    time(&tim);
    strcpy(inodes[ibIndex].date_created, ctime(&tim));
    strcpy(inodes[ibIndex].date_modified, ctime(&tim));
    createInodeEntryAndStoreDirOnDisk();
    std::cout << "Created directory \"" << dirname << "\"\n";
  }
}

void seekread(const char* filename, int pos) {
  /*Semantics: Move the read pointer for the specified file to byte number pos. Remember
  that the first byte of a file is numbered 0. If specified filename does not exist, print
  an error message and process next command in file.
  If pos points to a location beyond the last valid character, print an error message and
  process next command in file.
  Output: Echo the command to the screen and the result, as in:
  COMMAND: seekread /file1.c 35; RESULT: Read pointer set to byte 35
  COMMAND: seekread /dir1/file2c.c 35; RESULT: File does not exist
  COMMAND: seekread /dir1/file2a.c 100; RESULT: File length is exceeded.*/

  // first checking if the file exists
  int i = getInode(filename);

  // if it doesn't exist...
  if (i == -1) {
    std::cout << "File \"" << filename << "\" does not exist...\n";
  }
  else {
    if (pos >= inodes[i].size) {
      std::cout << "File length is exceeded...\n";
    }
    else {
      inodes[i].readPtr = pos;
      std::cout << "Read pointer for file \"" << filename << "\" set to byte " << inodes[i].readPtr << "\n";
    }
  }
}

void seekwrite(const char* filename, int pos) {
  /*Semantics: Move the write pointer for the specified file to byte number pos. Remember
  that the first byte of a file is numbered 0.
  If specified filename does not exist, print an error message and process next command in file.
  If pos points to a location beyond the last valid character, move the pointer to just past
  the last valid byte.
  Output: Echo the command to the screen and the result, as in:
  COMMAND: seekwrite /file1.c 35; RESULT: Write pointer set to byte 35
  COMMAND: seekwrite /dir1/file2c.c 35; RESULT: File does not exist
  COMMAND: seekwrite /dir1/file2a.c 100; RESULT: Write pointer set to byte 90
  In above example, assume that the file contained 90 bytes when command was issued.*/

  // first checking if the file exists
  int i = getInode(filename);

  // if it doesn't exist...
  if (i == -1) {
    std::cout << "File \"" << filename << "\" does not exist...\n";
  }
  else {
    if (pos > inodes[i].size) {
      inodes[i].writePtr = inodes[i].size;
    }
    else {
      inodes[i].writePtr = pos;
      std::cout << "Write pointer for file \"" << filename << "\" set to byte " << inodes[i].writePtr << "\n";
    }
  }
}

void seekwriteend(const char* filename) {
  /*Semantics: Move the write pointer for the specified file to past the last valid byte
  in the file. For instance, if the file has 10 bytes, then the write pointer will point
  to position 11.
  If specified filename does not exist, print an error message and process next command in file.
  Output: Echo the command to the screen and the result, as in:
  COMMAND: seekwriteend /file1.c; RESULT: Write pointer set to byte 11*/

  // first checking if the file exists
  int i = getInode(filename);

  // if it doesn't exist...
  if (i == -1) {
    std::cout << "File \"" << filename << "\" does not exist...\n";
  }
  else {
    inodes[i].writePtr = inodes[i].size;
    std::cout << "Write pointer for file \"" << filename << "\" set to byte " << inodes[i].writePtr << "\n";
  }
}

/* function that returns the next data block number corresponding to a give file */
int getNextDB(int db, int inodeIndex) {
  if (db == inodes[inodeIndex].direct_block_addresses[0]) {
    return inodes[inodeIndex].direct_block_addresses[1];
  }
  else if (db == inodes[inodeIndex].direct_block_addresses[1]) {
    return inodes[inodeIndex].direct_block_addresses[2];
  }
  else if (db == inodes[inodeIndex].direct_block_addresses[2]) {
    return inodes[inodeIndex].direct_block_addresses[3];
  }
  else if (db == inodes[inodeIndex].direct_block_addresses[3]) {
    return inodes[inodeIndex].index_block_contents[0];
  }
  else {
    int i = 0;
    while (i < 32) {
      if (db == inodes[inodeIndex].index_block_contents[i]) {
        return inodes[inodeIndex].index_block_contents[i+1];
      }
      i++;
    }
  }
}

char* readfile(const char* filename, int length, int flag) {
  /*Semantics: Print, to screen, the characters starting from readpointer to
  Minimum(filelength - 1, readpointer+length-1).
  If file does not exist, print an error message and proceed to next message in command.
  Output: Echo the command to the screen and the result, as below. Assume that readpointer
  is 10:
  COMMAND: read /file1.c 10; RESULT:
  *************************
  Filename: /file1.c; Bytes read: 10 - 19
  String stored: klmnopqrst
  *************************
  As another example, assume that readpointer is 5 and file length is 10 bytes:
  COMMAND: read /dir1/file2a.c 10; RESULT:
  *************************
  Filename: /dir1/file2a.c; Bytes read: 5 - 9
  String stored: fghij
  **************************/

  int i = getInode(filename);
  if (i == -1) {
    std::cout << "File \"" << filename << "\" does not exist...\n";
    return NULL;
  }
  else {
    if (inodes[i].readPtr >= inodes[i].size) {
      std::cout << "Read pointer for file \"" << filename << "\" is at the end of file. Reposition the read pointer to read...\n";
      return NULL;
    }
    FILE *fpd;
    fpd = fopen(disk_file, "r");
    int db;
    int pos;
    int start = inodes[i].readPtr;
    int end = inodes[i].readPtr + length - 1;
    if (end > inodes[i].size) {
      end = inodes[i].size - 1;
    }
    length = end - start + 1;

    if (inodes[i].readPtr < DISK_BLOCK_SIZE) {
      db = inodes[i].direct_block_addresses[0];
      pos = db*DISK_BLOCK_SIZE + inodes[i].readPtr;
    }
    else if (inodes[i].readPtr >= DISK_BLOCK_SIZE && inodes[i].readPtr < 2*DISK_BLOCK_SIZE) {
      db = inodes[i].direct_block_addresses[1];
      pos = (db - 1)*DISK_BLOCK_SIZE + inodes[i].readPtr;
    }
    else if (inodes[i].readPtr >= 2*DISK_BLOCK_SIZE && inodes[i].readPtr < 3*DISK_BLOCK_SIZE) {
      db = inodes[i].direct_block_addresses[2];
      pos = (db - 2)*DISK_BLOCK_SIZE + inodes[i].readPtr;
    }
    else {
      int te = inodes[i].readPtr - 3*DISK_BLOCK_SIZE + 1;
      int quo = te/DISK_BLOCK_SIZE;
      db = inodes[i].index_block_contents[quo];
      pos = (db - 3 - quo)*DISK_BLOCK_SIZE + inodes[i].readPtr;
    }

    fseek(fpd, pos, SEEK_SET);

    char temp[length];
    memset(temp, 0, length);

    if (length + inodes[i].readPtr <= DISK_BLOCK_SIZE) {
      fread(temp, 1, length, fpd);
    }
    else {
      // the file actually extends beyond current data block

      // first read the portion in current data block
      char temp1[DISK_BLOCK_SIZE - inodes[i].readPtr + 1];
      memset(temp1, 0, DISK_BLOCK_SIZE - inodes[i].readPtr);
      fread(temp1, 1, DISK_BLOCK_SIZE - inodes[i].readPtr, fpd);
      strncpy(temp, temp1, DISK_BLOCK_SIZE - inodes[i].readPtr);

      int len = length - DISK_BLOCK_SIZE + inodes[i].readPtr; // balance length to be read
      while (len > 0) {
        db = getNextDB(db, i);
        pos = DISK_BLOCK_SIZE * db;
        fseek(fpd, pos, SEEK_SET);
        if (len < DISK_BLOCK_SIZE) {
          char temp2[len + 1];
          memset(temp2, 0, len);
          fread(temp2, 1, len, fpd);
          strncpy((temp+strlen(temp)), temp2, len);
          break;
        }
        else {
          char temp3[DISK_BLOCK_SIZE + 1];
          memset(temp3, 0, DISK_BLOCK_SIZE);
          fread(temp3, 1, DISK_BLOCK_SIZE, fpd);
          strncpy((temp+strlen(temp)), temp3, DISK_BLOCK_SIZE);
          len -= DISK_BLOCK_SIZE;
        }
      }
    }

    temp[strlen(temp)-1] = '\0';
    inodes[i].readPtr += length;
    std::cout << "Filename: \"" << filename << "\"; Bytes read: " << start << " - " << end << "\n";
    if (flag == 1) {
      std::cout << "String read: " << temp << "\n";
    }

    fclose(fpd);
    char* t;
    t = (char*) malloc(sizeof(char) * strlen(temp));
    strcpy(t, temp);
    return t;
  }
}

void writefile(const char* filename, const char* str1) {
  /*Semantics: Write to file, the string in locations starting from writepointer to
  writepointer+length-1; where length is the length of the string (in bytes). The file
  length will be updated if the write goes past the previous write byte. The string will
  contain only readable characters from the set {[a − z], [A − Z], [0 − 9], #, ?, $, !, &}
  and will contain no spaces.
  If file does not exist, print an error message and proceed to next message in command.
  Output: Echo the command to the screen and the result, as below. Assume that writepointer is 10:
  COMMAND: write /file1.c ABCDEFGHIJKLMNO; RESULT: File written and length is: 25 bytes*/

  char* str;
  str = (char*) malloc(strlen(str1) * sizeof(char));
  strcpy(str, str1);
  int i = getInode(filename);
  if (i == -1) {
    std::cout << "File \"" << filename << "\" does not exist...\n";
  }
  else {
    FILE *fpd;
    fpd = fopen(disk_file, "r+");
    int db;
    int pos;
    int length = strlen(str);
    int length1 = length;

    if (inodes[i].size == 0) {
      if (inodes[i].writePtr < DISK_BLOCK_SIZE) {
        db = getNextFreeBlockIndex();
        inodes[i].direct_block_addresses[0] = db;
        pos = db*DISK_BLOCK_SIZE + inodes[i].writePtr;
      }
      else if (inodes[i].writePtr >= DISK_BLOCK_SIZE && inodes[i].writePtr < 2*DISK_BLOCK_SIZE) {
        db = getNextFreeBlockIndex();
        inodes[i].direct_block_addresses[1] = db;
        pos = (db - 1)*DISK_BLOCK_SIZE + inodes[i].writePtr;
      }
      else if (inodes[i].writePtr >= 2*DISK_BLOCK_SIZE && inodes[i].writePtr < 3*DISK_BLOCK_SIZE) {
        db = getNextFreeBlockIndex();
        inodes[i].direct_block_addresses[2] = db;
        pos = (db - 2)*DISK_BLOCK_SIZE + inodes[i].writePtr;
      }
      else {
        inodes[i].index_block = getNextFreeBlockIndex();
        int te = inodes[i].writePtr - 3*DISK_BLOCK_SIZE + 1;
        int quo = te/DISK_BLOCK_SIZE;
        int y = 0;
        while (y < quo) {
          inodes[i].index_block_contents[y] = getNextFreeBlockIndex();
          y++;
        }
        db = inodes[i].index_block_contents[quo];
        pos = (db - 3 - quo)*DISK_BLOCK_SIZE + inodes[i].writePtr;
      }

      fseek(fpd, pos, SEEK_SET);

      if (length + inodes[i].writePtr <= DISK_BLOCK_SIZE) {
        fprintf(fpd, "%s", str);
      }
      else {
        // the file will extend beyond the current data block
        // first write the portion that will fit into the current data block
        char temp1[DISK_BLOCK_SIZE - inodes[i].writePtr];
        strncpy(temp1, str, DISK_BLOCK_SIZE - inodes[i].writePtr);
        char* next = str + DISK_BLOCK_SIZE - inodes[i].writePtr;
        fprintf(fpd, "%s", temp1);

        int len = length - DISK_BLOCK_SIZE + inodes[i].writePtr; // balance length to be read
        while (len > 0) {
          db = getNextDB(db, i);
          pos = DISK_BLOCK_SIZE * db;
          fseek(fpd, pos, SEEK_SET);
          if (len < DISK_BLOCK_SIZE) {
            char temp2[len];
            strncpy(temp2, next, len);
            next = next + len;
            fprintf(fpd, "%s", temp2);
            break;
          }
          else {
            char temp3[DISK_BLOCK_SIZE];
            strncpy(temp3, next, DISK_BLOCK_SIZE);
            next = next + len;
            fprintf(fpd, "%s", temp3);
            len -= DISK_BLOCK_SIZE;
          }
        }
      }
    }
    else {
      if (inodes[i].writePtr < DISK_BLOCK_SIZE) {
        db = inodes[i].direct_block_addresses[0];
        pos = db*DISK_BLOCK_SIZE + inodes[i].writePtr;
      }
      else if (inodes[i].writePtr >= DISK_BLOCK_SIZE && inodes[i].writePtr < 2*DISK_BLOCK_SIZE) {
        db = inodes[i].direct_block_addresses[1];
        pos = (db - 1)*DISK_BLOCK_SIZE + inodes[i].writePtr;
      }
      else if (inodes[i].writePtr >= 2*DISK_BLOCK_SIZE && inodes[i].writePtr < 3*DISK_BLOCK_SIZE) {
        db = inodes[i].direct_block_addresses[2];
        pos = (db - 2)*DISK_BLOCK_SIZE + inodes[i].writePtr;
      }
      else {
        int te = inodes[i].writePtr - 3*DISK_BLOCK_SIZE + 1;
        int quo = te/DISK_BLOCK_SIZE;
        db = inodes[i].index_block_contents[quo];
        pos = (db - 3 - quo)*DISK_BLOCK_SIZE + inodes[i].writePtr;
      }

      fseek(fpd, pos, SEEK_SET);

      if (length + inodes[i].writePtr <= DISK_BLOCK_SIZE) {
        fprintf(fpd, "%s", str);
      }
      else {
        // the file will extend beyond the current data block
        // first write the portion that will fit into the current data block
        char temp1[DISK_BLOCK_SIZE - inodes[i].writePtr];
        strncpy(temp1, str, DISK_BLOCK_SIZE - inodes[i].writePtr);
        char* next = str + DISK_BLOCK_SIZE - inodes[i].writePtr;
        fprintf(fpd, "%s", temp1);

        int len = length - DISK_BLOCK_SIZE + inodes[i].writePtr; // balance length to be read
        while (len > 0) {
          db = getNextDB(db, i);
          pos = DISK_BLOCK_SIZE * db;
          fseek(fpd, pos, SEEK_SET);
          if (len < DISK_BLOCK_SIZE) {
            char temp2[len];
            strncpy(temp2, next, len);
            next = next + len;
            fprintf(fpd, "%s", temp2);
            break;
          }
          else {
            char temp3[DISK_BLOCK_SIZE];
            strncpy(temp3, next, DISK_BLOCK_SIZE);
            next = next + len;
            fprintf(fpd, "%s", temp3);
            len -= DISK_BLOCK_SIZE;
          }
        }
      }
    }

    if (inodes[i].writePtr + length1 >= inodes[i].size) {
      inodes[i].size = inodes[i].writePtr + length1;
      inodes[i].writePtr = inodes[i].size - 1;
    }
    else {
      inodes[i].writePtr += length1;
    }

    char tempInode[DISK_BLOCK_SIZE];
    sprintf(tempInode, "{%s}", inodes[i].filename);
    sprintf((tempInode + strlen(tempInode)), "{%d}", inodes[i].size);
    sprintf((tempInode + strlen(tempInode)), "{%s}", inodes[i].date_created);
    sprintf((tempInode + strlen(tempInode)), "{%s}", inodes[i].date_modified);
    sprintf((tempInode + strlen(tempInode)), "{%d,%d,%d}", inodes[i].direct_block_addresses[0], inodes[i].direct_block_addresses[1], inodes[i].direct_block_addresses[2]);

    // print the inode information in the corresponding inode block
    pos = i * DISK_BLOCK_SIZE;
    fseek(fpd, pos, SEEK_SET);
    fprintf(fpd, "%s", tempInode);

    std::cout << "File \"" << filename << "\" written; Length of file: " << inodes[i].size << "\n";
    fclose(fpd);
  }
}

void deletefile(const char* filename) {
  /*Semantics: Delete the file, if it exists. If it does not exist, print an error message
  and proceed to next command in the file.
  Output: Echo the command to the screen and the result of the deletion, as in:
  COMMAND: delete /file1.c; RESULT: /file1.c deleted
  COMMAND: delete /file39.c; RESULT: /file39.c does not exist*/

  int i = getInode(filename);
  if (i == -1) {
    std::cout << "File \"" << filename << "\" does not exist, cannot be deleted...\n";
  }
  else {
    inodes[i].isFree = true;
    std::cout << "File \"" << filename << "\" is deleted\n";
  }
}

void deletedir(const char* dirname) {
  int i = getInode(dirname);
  if (i == -1) {
    std::cout << "Directory \"" << dirname << "\" does not exist, cannot be deleted...\n";
  }
  else {
    inodes[i].isFree = true;
    std::cout << "Directory \"" << dirname << "\" is deleted\n";
  }
}

void ls(const char* dirname) {
  std::cout << "Sorry, not implemented....\n";
}

void run(const char* name) {
  FILE* fp;
  std::string str1, tok, t;
  seekread(name, 0);
  char* filecontents;
  char* filec;
  char delim[] = "\n";
  filecontents = (char*) malloc(sizeof(char)*getFileSize(name));
  strcpy(filecontents, readfile(name, getFileSize(name), 0));
  fp = fopen(name, "r");
  char cmd[100];
  //while (fgets(cmd, 100, fp) != NULL) {
  filec = strtok(filecontents, delim);
  while (filec != NULL) {
    std::string fileString(filec);
    std::stringstream s1(fileString);
    s1 >> tok;
    printf("\nCommand = %s\n", fileString.c_str());
    // open command
    if (strcmp(tok.c_str(), "open") == 0) {
      s1 >> tok;
      // if the given file name is valid
      if (checkFileName(tok.c_str())) {
        openfile(tok.c_str());
      }
      // if an invalid file name is given
      else {
        std::cout << "Invalid file name specified...\n";
      }
    }

    else if (strcmp(tok.c_str(), "createdir") == 0) {
      s1 >> tok;
      // if the given directory name is valid
      if (checkDirName(tok.c_str())) {
        createdir(tok.c_str());
      }
      // if an invalid directory name is given
      else {
        std::cout << "Invalid directory name specified...\n";
      }
    }

    else if (strcmp(tok.c_str(), "seekread") == 0) {
      s1 >> tok; // file name
      s1 >> t;
      int tempPos = atoi(t.c_str()); // position
      seekread(tok.c_str(), tempPos);
    }

    else if (strcmp(tok.c_str(), "seekwrite") == 0) {
      s1 >> tok; // file name
      s1 >> t;
      int tempPos = atoi(t.c_str()); // position
      seekwrite(tok.c_str(), tempPos);
    }

    else if (strcmp(tok.c_str(), "seekwriteend") == 0) {
      s1 >> tok;
      seekwriteend(tok.c_str());
    }

    else if (strcmp(tok.c_str(), "read") == 0) {
      s1 >> tok;
      s1 >> t;
      int tempLen = atoi(t.c_str());
      readfile(tok.c_str(), tempLen, 1);
    }

    else if (strcmp(tok.c_str(), "write") == 0) {
      s1 >> tok;
      s1 >> t;
      writefile(tok.c_str(), t.c_str());
    }

    else if (strcmp(tok.c_str(), "delete") == 0) {
      s1 >> tok;
      deletefile(tok.c_str());
    }

    else if (strcmp(tok.c_str(), "deletedir") == 0) {
      s1 >> tok;
      deletedir(tok.c_str());
    }

    else if (strcmp(tok.c_str(), "ls") == 0) {
      s1 >> tok;
      ls(tok.c_str());
    }

    filec = strtok(NULL, delim);
  }

  fclose(fp);
}

/* driver code that manages the entire file system */
int main(int argc, char* argv[]) {
  // taking the command line arguments from the user
  if (argc != 4) {
    std::cout << "Incorrect number of command line arguments, expected 4, received " << argc << "\n";
    exit(0);
  }

  disk_file = (char*) malloc(16*sizeof(char));
  strcpy(disk_file, argv[1]);

  if (argv[2][1] == 'D') {
    D = atoi(argv[3]);
    if (D < 5) {
      std::cout << "Expected value of disk size D > 5 kB, but received " << D << "\n";
      exit(0);
    }
  }
  else {
    std::cout << "Incorrect option specified, expected -D, received -" << argv[2][1] << "\n";
    exit(0);
  }

  // calculating the number of disk blocks
  numBlocks = D*8;
  freeBitVector.resize(numBlocks);

  int i;

  for (i = 0; i < numBlocks; i++) {
    freeBitVector[i] = true;
  }
  for (i = 0; i < numInodeBlocks; i++) {
    inodes[i].isFree = true;
  }

  // allocating first 20 of the disk blocks to store the inode information
  numInodeBlocks = 20;
  inodes.resize(numInodeBlocks);

  FILE* fp;
  // first checking if the disk file already exists
  fp = fopen(disk_file, "r");

  // if it doesn't exist...
  if (fp == NULL) {
    // ... we create it with the specified size, and open it in read-write mode
    int x = (D * 1024) - 1;
    fp = fopen(disk_file, "w+");
    fseek(fp, x, SEEK_SET);
    fputc('\0', fp);
    fclose(fp);
  }
  else {
    fclose(fp);
  }

  time_t tim;
  char* f[3];
  for (i = 0; i < 3; i++) {
    f[i] = (char*) malloc(16*sizeof(char));
  }

  strcpy(f[0], "file");
  strcpy(f[1], "file1");
  strcpy(f[2], "file2");
  for (i = 0; i < 3; i++) {
    inodes[ibIndex].type = 1;
    strcpy(inodes[ibIndex].filename, f[i]);
    inodes[ibIndex].size = getFileSize(inodes[ibIndex].filename);
    inodes[ibIndex].isFree = 0;
    time(&tim);
    strcpy(inodes[ibIndex].date_created, ctime(&tim));
    strcpy(inodes[ibIndex].date_modified, ctime(&tim));
    inodes[ibIndex].index_block = -1;
    int k = 0;
    while (k < 3) {
      inodes[ibIndex].direct_block_addresses[k] = -1;
      k++;
    }
    k = 0;
    while(k < 32) {
      inodes[ibIndex].index_block_contents[k] = -1;
      k++;
    }
    createInodeEntryAndStoreFileOnDisk();
    inodes[ibIndex-1].printInode();
  }

  // ... we get rid of the current file pointer and reopen the file in read-write mode
  fp = fopen(disk_file, "r+");

  // moving to the first block after the inode blocks
  int pos = DISK_BLOCK_SIZE * numInodeBlocks;
  fseek(fp, pos, SEEK_SET);

  // executable command interpreter
  std::string fileString, str1, tok, t;
  std::cout << "\n<Command Please> ";
  while (std::getline(std::cin, fileString)) {
    std::stringstream s1(fileString);
    s1 >> tok;

    // load command
    if (strcmp(tok.c_str(), "load") == 0) {
      s1 >> tok;
      // if the given file name is valid
      if (checkFileName(tok.c_str())) {
        int i = getInode(tok.c_str());
        if (i == -1) {
          time_t tim;
          inodes[ibIndex].type = 1;
          strcpy(inodes[ibIndex].filename, tok.c_str());
          inodes[ibIndex].size = getFileSize(inodes[ibIndex].filename);
          time(&tim);
          strcpy(inodes[ibIndex].date_created, ctime(&tim));
          strcpy(inodes[ibIndex].date_modified, ctime(&tim));
          inodes[ibIndex].isFree = 0;
          inodes[ibIndex].index_block = -1;
          int k = 0;
          while (k < 3) {
            inodes[ibIndex].direct_block_addresses[k] = -1;
            k++;
          }
          k = 0;
          while(k < 32) {
            inodes[ibIndex].index_block_contents[k] = -1;
            k++;
          }
          createInodeEntryAndStoreFileOnDisk();
        }
        run(tok.c_str());
      }
      // if an invalid file name is given
      else {
        std::cout << "Invalid file name specified...\n";
      }
    }

    // printinode command
    else if (strcmp(tok.c_str(), "printinode") == 0) {
      s1 >> tok;
      // if the given file name is valid
      if (checkFileName(tok.c_str())) {
        // first we find the corresponding inode block
        int i = getInode(tok.c_str());
        // if found, we print the inode block's contents
        if (i != -1) {
          if (inodes[i].type == 1) {
            inodes[i].printInode();
          }
          else {
            inodes[i].printInodeD();
          }
        }
        // if the file is not found
        else {
          std::cout << "File not found...\n";
        }
      }
      // if an invalid file name is given
      else {
        std::cout << "Invalid file name specified...\n";
      }
    }

    // exit command
    else if (strcmp(tok.c_str(), "exit") == 0) {
      break;
    }

    // default case
    else {
      std::cout << "Invalid command... Please enter a valid command\n";
    }

    std::cout << "\n<Command Please> ";
  }

  return 0;
}
