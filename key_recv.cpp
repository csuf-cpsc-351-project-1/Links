#include <sys/ipc.h>
#include <signal.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <signal.h>
#include "msg.h"    /* For the message struct */
 
/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000
 
/* The ids for the shared memory segment and the message queue */
int shmid, msqid;
 
/* The pointer to the shared memory */
void* sharedMemPtr;
 
/* The name of the received file */
const char recvFileName[] = "recvfile.txt";
 
 
/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */
 
void recInit(int& shmid, int& msqid, void*& sharedMemPtr)
{
    /* DONE 
     TODO: 1. Create a file called keyfile.txt containing string "Hello world" (you may do
            so manually or from the code).
             2. Use ftok("keyfile.txt", 'a') in order to generate the key.
         3. Use the key in the TODO's below. Use the same key for the queue
            and the shared memory segment. This also serves to illustrate the difference
            between the key and the id used in message queues and shared memory. The id
            for any System V object (i.e. message queues, shared memory, and sempahores) 
            is unique system-wide among all System V objects. Two objects, on the other hand,
            may have the same key.
     */
 
  const char keyfile[] = "keyfile.txt";
   
  key_t key;
  key = ftok(keyfile, 'a');
 
  std::cout << "key is : " << key << "\n";
 
  /* DONE
  ** TODO: Allocate a piece of shared memory. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE. 
  */
 
  if ((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0644 | IPC_CREAT)) == -1) {
    perror("shmget");
    exit(1);
  }
 
    /* DONE 
  ** TODO: Attach to the shared memory 
  */
   
  sharedMemPtr = shmat(shmid, (void *)0, 0);
  if (sharedMemPtr == (char*)(-1)) {
    perror("shmat");
    exit(1);
  }
 
    /* DONE 
  ** TODO: Create a message queue 
  */
 
  if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
    perror("msgget");
    exit(1);
  }
   
  std::cout << "msqid is : " << msqid << "\n";
  std::cout << "shmid is : " << shmid << "\n";
 
}
 
/**
 * Perfoms the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
 
void recCleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
    /* DONE
  ** TODO: Detach from shared memory 
  */
     
  if ((shmdt(sharedMemPtr)) == 0)
    std::cout << "Detached pointer from shared memory successful.\n";
  else
  {
    std::cout << "Detach pointer from shared memory error: " << errno << "\n";
    exit(1);
  }
 
    /* DONE 
  ** TODO: Deallocate the shared memory chunk 
  */
     
  if ((shmctl(shmid, IPC_RMID, NULL)) == 0)
    std::cout << "Deallocation of shared memory chunk successful.\n";
  else
  {
    std::cout << "Deallocation of shmid error: " << errno << "\n";
    exit(1);
  }
 
    /* DONE
  ** TODO: Deallocate the message queue 
  */
   
  if ((msgctl(msqid, IPC_RMID, NULL)) == 0)
    std::cout << "Deallocation of message queue successful.\n";
  else
  {
    std::cout << "Deallocation of message queue error: " << errno << "\n";
    exit(1);
  }
}
 
/*
** The main loop
*/
 
void mainLoop(FILE *fp)
{
  std::cout << "Entering the main loop \n";
 
  /* Buffer for receiving message */
  message* recMsg = new message();
 
  /* Buffer for sending message */
  message* sndMsg = new message();
 
  /*  DONE 
  **  TODO: Receive the message and get the message size. The message will 
  ** contain regular information. The message will be of SENDER_DATA_TYPE
  ** (the macro SENDER_DATA_TYPE is defined in msg.h).  If the size field
  ** of the message is not 0, then we copy that many bytes from the shared
  ** memory region to the file. Otherwise, if 0, then we close the file and
  ** exit.
  **
  ** NOTE: the received file will always be saved into the file called
  ** "recvfile"
  */
 
  /* Keep receiving until the sender set the size to 0 indicating that there is
  ** no more data to send
  */
 
  /* Initialize msg size to 1 to get into the loop. Once we are inside the loop,
  ** receive a message from the queue and set the msgSize to its return value.
  ** If the message size is not 0, write the message from the shared memory to
  ** the file and send a confirmation to the receiver. Then go back to the
  ** beginning of the loop and repeat, if msgSize is ever 0, there will be
  ** no write and the loop exits.
  */
 
  int msgSize = 1;
  while(msgSize != 0)
  {
    std::cout << "I'm waiting to receive now.\n";
    if ((msgSize = msgrcv(msqid, recMsg, SHARED_MEMORY_CHUNK_SIZE,
                          SENDER_DATA_TYPE, 0)) < 0)
    {
      perror("msgrcv");
      exit(-1);
    }
    std::cout << "Received next chunk size: " << msgSize << "\n";
 
    /* If the sender is not telling us that we are done, then get to work */
    if (msgSize != 0)
    {
      /* Save the shared memory to file */
      std::cout << "I am going to write the file now\n";
      if ((fwrite(sharedMemPtr, sizeof(char), msgSize, fp)) == -1)
      {
        perror("fwrite");
        exit(-1);
      }
      std::cout << "I successfully wrote chunk to file. \n";
 
      /* DONE
      ** TODO: Tell the sender that we are ready for the next file chunk. 
            ** I.e. send a message of type RECV_DONE_TYPE (the value of size field
            ** does not matter in this case). 
            */
 
      sndMsg->mtype = RECV_DONE_TYPE;
      sndMsg->size = 0;
      if ((msgsnd(msqid, sndMsg, 0, 0)) < 0)
      {
        perror("msgsnd");
        exit(-1);
      }
    }
  }
  /* Deallocate buffers for sending & receiving messages */
  delete sndMsg;
  delete recMsg;
}
 
/**
 * Handles the exit signal
 * @param signal - the signal type
 */
void ctrlCSignal(int signal)
{
    /* Free system V resources */
    recCleanUp(shmid, msqid, sharedMemPtr);
}
 
int main(int argc, char** argv)
{
     
    /* TODO: Install a singnal handler (see signaldemo.cpp sample file).
     * In a case user presses Ctrl-c your program should delete message
     * queues and shared memory before exiting. You may add the cleaning functionality
     * in ctrlCSignal().
     */
    signal(SIGINT, ctrlCSignal);
 
    /* RecInitialize */
    recInit(shmid, msqid, sharedMemPtr);
   
  /* Open file for writing and error check */
  FILE* fp;
  fp = fopen(recvFileName, "w");
  if(!fp)
  {
   perror("fopen");
   exit(-1);
  }
 
    /* Go to the main loop */
  mainLoop(fp);
 
  /* Close file */
  if (fclose(fp) != 0)
  {
    perror("fclose");
    exit(-1);
  }
 
    /* DONE
  ** TODO: Detach from shared memory segment, and deallocate shared memory and 
  ** message queue (i.e. call cleanup) 
  */
  recCleanUp(shmid, msqid, sharedMemPtr);
    return 0;
}
