#include <sys/shm.h>
#include <sys/msg.h>
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
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{

	// create key
	key_t bathroomkey;

	// assign key
	if ((bathroomkey = ftok("keyfile.txt", 'a')) == -1) {
		std::cout << "An error occurred while creating the key in sender.\n";
		exit(-1);
	}

	// track the key for debugging purposes
	std::cout << "The key is: " << bathroomkey << std::endl;

	/*
	NOTE:	Use the key in the TODO's below. Use the same key for the queue
	and the shared memory segment. This also serves to illustrate the difference
	between the key and the id used in message queues and shared memory. The id
	for any System V object (i.e. message queues, shared memory, and sempahores)
	is unique system-wide among all System V objects. Two objects, on the other hand,
	may have the same key.
	 */

	// find the shared memory id
	if ((shmid = shmget(bathroomkey, SHARED_MEMORY_CHUNK_SIZE, 0644 | IPC_CREAT)) == -1) {
		std::cout << "An error occurred while finding the shared memory id.\n";
		exit(-1);
	}

	// attach to the shared memory
	sharedMemPtr = shmat(shmid, (void *)NULL, 0);
	if (sharedMemPtr == (char*)(-1)) {
		std::cout << "An error occurred while attempting to attach to the shared memory.\n";
		exit(-1);
	}

	// attach to message queue
	if ((msqid = msgget(bathroomkey, 0644 | IPC_CREAT)) == -1) {
		std::cout << "An error occurred while attaching to the message queue.\n";
		exit(-1);
	}

	std::cout << "The message queue id is: " << msqid << std::endl;
	std::cout << "The shared memory id is: " << shmid << std::endl;

}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* TODO: Detach from shared memory */
	if (shmdt(sharedMemPtr) == 0) {
		std::cout << "Successfully detached shared memory pointer.\n";
	}
	else {
		std::cout << "An error occurred while attempting to detach the shared memory pointer.\n";
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
	message sendMsg;

	/* A buffer to store message received from the receiver. */
	message recvMsg;

	/* Was the file open? */
	if (!fp)
	{
		std::cout << "The file was not open.\n";
		exit(-1);
	}

	/* Read the whole file */
	while (!feof(fp))
	{
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory.
		 * fread will return how many bytes it has actually read (since the last chunk may be less
		 * than SHARED_MEMORY_CHUNK_SIZE).
		 */
		if ((sendMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
		{
			std::cout << "An error occurred when reading from file.\n";
			exit(-1);
		}


		/* TODO: Send a message to the receiver telling him that the data is ready
		 * (message of type SENDER_DATA_TYPE)
		 */

		sendMsg.mtype = SENDER_DATA_TYPE;

		if ((msgsnd(msqid, &sendMsg, sizeof(message), 0)) < 0) {
			std::cout << "The message was unable to send.\n";
			exit(-1);
		}

		std::cout << "The message sent has a chunk size of: " << sendMsg.size << std::endl;


		/* TODO: Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us
		 * that he finished saving the memory chunk.

		 */

		std::cout << "Waiting for response.\n";

		if ((msgrcv(msqid, &recvMsg, sizeof(message), RECV_DONE_TYPE, 0)) < 0) {
			std::cout << "No response from receiver. Exiting...\n";
			exit(-1);
		}

		std::cout << "Response received.\n";

	}


	/** TODO: once we are out of the above loop, we have finished sending the file.
	  * Lets tell the receiver that we have nothing more to send. We will do this by
	  * sending a message of type SENDER_DATA_TYPE with size field set to 0.
	  */
	sendMsg.mtype = SENDER_DATA_TYPE;
	sendMsg.size = 0;
	if ((msgsnd(msqid, &sendMsg, 0, 0)) < 0) {
		std::cout << "Failed to send the final message to the receiver.\n";
		exit(-1);
	}

	/* Close the file */
	if (fclose(fp) != 0) {
		std::cout << "Error occurred when attempting to close the file.\n";
		exit(-1);
	}

}


int main(int argc, char** argv)
{

	/* Check the command line arguments */
	if (argc < 2)
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
