#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */

// added includes
// #include <sys/ipc.h>
#include <iostream>


/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void *sharedMemPtr;

/* The name of the received file */
const char recvFileName[] = "recvfile.txt";


/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{

	/* TODO: 1. Create a file called keyfile.txt containing string "Hello world" (you may do
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
	// create key
	key_t bathroomkey;

	// assign key
	bathroomkey = ftok(keyfile, 'a');

	// track the key for debugging
	std::cout << "The key is: " << bathroomkey << std::endl;


	/* TODO: Allocate a piece of shared memory. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE. */
	if ((shmid = shmget(bathroomkey, SHARED_MEMORY_CHUNK_SIZE, 0644 | IPC_CREAT)) == -1) {
		std::cout << "An error occurred while finding the shared memory id.\n";
		exit(-1);
	}
	/* TODO: Attach to the shared memory */
	sharedMemPtr = shmat(shmid, (void *)0, 0);
	if (sharedMemPtr == (char*)(-1)) {
		std::cout << "An error occurred while attempting to attach to the shared memory.\n";
		exit(-1);
	}
	/* TODO: Create a message queue */
	if ((msqid = msgget(bathroomkey, 0644 | IPC_CREAT)) == -1) {
		std::cout << "An error occurred while attaching to the message queue.\n";
		exit(-1);
	}

	// tracking for debugging purposes
	std::cout << "The message queue id is: " << msqid << std::endl;
	std::cout << "The shared memory id is: " << shmid << std::endl;

	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */

}

/**
 * The main loop
 */
void mainLoop()
{
	/* The size of the mesage */
	int msgSize = 1;

	/* Open the file for writing */
	FILE *fp = fopen(recvFileName, "w");

	/* Error checks */
	if (!fp)
	{
		perror("fopen");
		exit(-1);
	}

	/* TODO: Receive the message and get the message size. The message will
	 * contain regular information. The message will be of SENDER_DATA_TYPE
	 * (the macro SENDER_DATA_TYPE is defined in msg.h).  If the size field
	 * of the message is not 0, then we copy that many bytes from the shared
	 * memory region to the file. Otherwise, if 0, then we close the file and
	 * exit.
	 *
	 * NOTE: the received file will always be saved into the file called
	 * "recvfile"
	 */

	 /* Keep receiving until the sender set the size to 0, indicating that
	  * there is no more data to send
	  */
	message youvegotmail;

	// moved ready4more for persistence(?) Pretty sure while loop was resetting value to empty
	message ready4more;


	while (msgSize != 0)
	{
	  	if ((msgSize = msgrcv(msqid, &youvegotmail, SHARED_MEMORY_CHUNK_SIZE, SENDER_DATA_TYPE, 0)) < 0){
		perror("msgrcv");
		exit(-1);
	} //receive message from sender

		/* If the sender is not telling us that we are done, then get to work */
		if (msgSize != 0)
		{
			/* Save the shared memory to file */
			if (fwrite(sharedMemPtr, sizeof(char), msgSize, fp) == -1)
			{
				perror("fwrite");
				exit(-1);
			}

			/* TODO: Tell the sender that we are ready for the next file chunk.
			 * I.e. send a message of type RECV_DONE_TYPE (the value of size field
			 * does not matter in this case).
			 */
			ready4more.mtype = RECV_DONE_TYPE;
			ready4more.size = 0;


			if ((msgsnd(msqid, &ready4more, 0, 0)) < 0)
			{
				perror("msgsnd");
				exit(-1);
			}
		}
		/* We are done */
		else
		{
			/* Close the file */
			fclose(fp);
		}
	}
}



/**
 * Perfoms the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* TODO: Detach from shared memory */
	if ((shmdt(sharedMemPtr)) != 0) {
		std::cout << "Shared memory pointer detach failed.\n";
		exit(-1);
	}
	/* TODO: Deallocate the shared memory chunk */
	if ((shmctl(shmid, IPC_RMID, NULL)) != 0) {
		std::cout << "Shared memory deallocation failed.\n";
		exit(-1);
	}
	/* TODO: Deallocate the message queue */
	if ((msgctl(msqid, IPC_RMID, NULL)) != 0) {
		std::cout << "Message queue deallocation failed.\n";
		exit(-1);
	}
}

/**
 * Handles the exit signal
 * @param signal - the signal type
 */

void ctrlCSignal(int signal)
{
	/* Free system V resources */
	cleanUp(shmid, msqid, sharedMemPtr);
}

int main(int argc, char** argv)
{

	/* TODO: Install a singnal handler (see signaldemo.cpp sample file).
	 * In a case user presses Ctrl-c your program should delete message
	 * queues and shared memory before exiting. You may add the cleaning functionality
	 * in ctrlCSignal().
	 */
	signal(SIGINT, ctrlCSignal);

	/* Initialize */
	init(shmid, msqid, sharedMemPtr);

	/* Go to the main loop */
	mainLoop();

	/** TODO: Detach from shared memory segment, and deallocate shared memory and message queue (i.e. call cleanup) **/
	cleanUp(shmid, msqid, sharedMemPtr);

	return 0;
}
