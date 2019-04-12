#include <sys/ipc.h>
#include <signal.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include "msg.h"    /* For the message struct */
 
/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000
 
/* The ids for the shared memory segment and the message queue */
int shmid, msqid;
 
/* The pointer to the shared memory */
void* sharedMemPtr;
 
/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 */
 
void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
    /* DONE
     TODO: 
        1. Create a file called keyfile.txt containing string "Hello world" (you may do
            so manually or from the code).
        2. Use ftok("keyfile.txt", 'a') in order to generate the key.
        3. Use the key in the TODO's below. Use the same key for the queue
            and the shared memory segment. This also serves to illustrate the difference
            between the key and the id used in message queues and shared memory. The id
            for any System V objest (i.e. message queues, shared memory, and sempahores) 
            is unique system-wide among all SYstem V objects. Two objects, on the other hand,
            may have the same key.
     */
   
  const char keyfile[] = "keyfile.txt";
 
  key_t key;
  if ((key = ftok(keyfile, 'a')) == -1) {
    perror("ftok");
    exit(1);
  }
 
  std::cout << "key is: " << key << "\n";
 
    /* DONE
     TODO: Get the id of the shared memory segment. T
     he size of the segment must be SHARED_MEMORY_CHUNK_SIZE */
 
  if ((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0644 | IPC_CREAT)) == -1) {
    perror("shmget");
    exit(1);
  }
 
    /* DONE 
     TODO: Attach to the shared memory */
 
  sharedMemPtr = shmat(shmid, (void *)0, 0);
  if (sharedMemPtr == (char*)(-1)) {
    perror("shmat");
    exit(1);
  }
 
    /* DONE 
     TODO: Attach to the message queue */
 
  if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
    perror("msgget");
    exit(1);
  }
 
  std::cout << "my msqid: " << msqid << "\n";
  std::cout << "my shmid: " << shmid << "\n";
   
}
 
/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
 
void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
    /* DONE
     TODO: Detach from shared memory */
 
  if ((shmdt(sharedMemPtr)) == 0)
    std::cout << "Detached pointer from shared memory successful.\n";
  else
  {
    std::cout << "Detach pointer from shared memory error: " << errno << "\n";
    exit(-1);
  }
}
 
/**
 * The main send function
 * @param fileName - the name of the file
 */
void send(const char* fileName)
{
    /* Open the file for reading */
    FILE* fp = fopen(fileName, "r");
 
    /* A buffer to store message we will send to the receiver. */
    message* sndMsg = new message();
     
    /* A buffer to store message received from the receiver. */
    message* recMsg = new message();
 
    /* Was the file open? */
    if(!fp)
    {
        perror("fopen");
        exit(-1);
    }
    /* Read the whole file */
   
    while (true)
  {
        /* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory. 
         * fread will return how many bytes it has actually read (since the last chunk may be less
         * than SHARED_MEMORY_CHUNK_SIZE).
         */
    if ((sndMsg->size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
    {
      perror("fread");
      exit(-1);
    }
 
    /* TODO: Send a message to the receiver telling him that the data is ready 
         * (message of type SENDER_DATA_TYPE) 
         */
     
    sndMsg->mtype = SENDER_DATA_TYPE;
    if ((msgsnd(msqid, sndMsg, sndMsg->size, 0)) < 0)
    {
      perror("msgsnd");
      exit(-1);
    }
    std::cout << "I've sent a message of chunk size: " << sndMsg->size << "\n";
     
    /* DONE
     * TODO: Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us
         * that he finished saving the memory chunk. 
         */
 
    std::cout << "I'm waiting to receive confirmation now.\n";
    if ((msgrcv(msqid, recMsg, 0, RECV_DONE_TYPE, 0)) < 0)
    {
      perror("msgrcv");
      exit(-1);
    }
    std::cout << "I received the confirmation message.\n";
    if (feof(fp))
      break;
  }
 
  /* DONE
   * TODO: once we are out of the above loop, we have finished sending the file.
   * Lets tell the receiver that we have nothing more to send. We will do this by
   * sending a message of type SENDER_DATA_TYPE with size field set to 0.   
   */
 
  sndMsg->mtype = SENDER_DATA_TYPE;
  sndMsg->size = 0;
  if ((msgsnd(msqid, sndMsg, 0, 0)) < 0)
  {
    perror("msgsnd");
    exit(-1);
  }
  std::cout << "I've sent my closing message.\n";
 
    /* Close the file */
    if (fclose(fp) != 0)
  {
    perror("fclose");
    exit(-1);
  }
  /* Deallocate buffers for sending & receiving messages */
  delete recMsg;
  delete sndMsg;
}
 
 
int main(int argc, char** argv)
{
     
    /* Check the command line arguments */
    if(argc < 2)
    {
        fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
        exit(-1);
    }
 
    /* Connect to shared memory and the message queue */
    init(shmid, msqid, sharedMemPtr);
 
    /* Send the file */
    send(argv[1]);
     
    /* Cleanup */
    cleanUp(shmid, msqid, sharedMemPtr);
         
    return 0;
}
