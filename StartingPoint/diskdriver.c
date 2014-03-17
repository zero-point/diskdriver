/*
 *Operating Systems 3 - Assessed Exercise 1
 *
 *@author Irina Preda
 *@author 1102452p 
 *@date 05/03/2014
 *
 *A program that defines the interface to the DiskDriver module 
 *that mediates access to a disk device
 *
 */
#include "freesectordescriptorstore.h"
#include "freesectordescriptorstore_full.h"
#include "sectordescriptorcreator.h"
#include "diskdriver.h"
#include <pthread.h>
#include "BoundedBuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include "sectordescriptor.h"
#include "block.h"
#include "pid.h"
#include "voucher.h"
#include "diskdevice.h"

//data structures

FreeSectorDescriptorStore fsds;
DiskDevice disk_device;
BoundedBuffer bb_read;
BoundedBuffer bb_write;

//pthread_mutex_t mutex;
//pthread_cond_t cond;

struct voucher {

	SectorDescriptor sd;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int isDone; // 1 is successful, 0 otherwise
	int rw; // 1 is read, 0 otherwise

};

static void* thread_read(){

	while(1){
		struct voucher *vRead = (struct voucher *) blockingReadBB(bb_read); 
		int isDone = read_sector(disk_device,&(vRead->sd));
		pthread_mutex_lock(&(vRead->mutex));
		vRead->isDone = isDone;
		pthread_cond_signal(&(vRead->cond));
		pthread_mutex_unlock(&(vRead->mutex));
	}
	pthread_exit(NULL);
}

static void* thread_write(){

	while(1){
		struct voucher *vWrite = (struct voucher *) blockingReadBB(bb_write); 
		int isDone = write_sector(disk_device,&(vWrite->sd));
		pthread_mutex_lock(&(vWrite->mutex));
		if(vWrite->isDone == 0) pthread_cond_wait(&(vWrite->cond),&vWrite->mutex);
		vWrite->isDone = isDone;
		pthread_cond_signal(&(vWrite->cond));
		pthread_mutex_unlock(&(vWrite->mutex));
	}
	pthread_exit(NULL);
}

/*
 * called before any other methods to allow you to initialize data
 * structures and to start any internal threads.
 *
 * Arguments:
 *   dd: the DiskDevice that you must drive
 *   mem_start, mem_length: some memory for SectorDescriptors
 *   fsds_ptr: you hand back a FreeSectorDescriptorStore constructed
 *             from the memory provided in the two previous arguments
 */
void init_disk_driver(DiskDevice dd, void *mem_start, unsigned long mem_length,
		FreeSectorDescriptorStore *fsds_ptr){

	disk_device = dd;

	bb_read = createBB(2000);
	bb_write = createBB(2000);

	pthread_t thread_id_r, thread_id_w;

	// will need to use FreeSectorDescriptorStore facilities
	*fsds_ptr = create_fsds();

	// populate it with SectorDescriptors created from a memory range
	create_free_sector_descriptors(*fsds_ptr, mem_start, mem_length);

	fsds = *fsds_ptr;

	if(pthread_create(&thread_id_r,NULL,thread_read,NULL)){
		// an error occured
		perror("Error creating first thread\n"); 
		//exit(1); 
	} 

	if(pthread_create(&thread_id_w,NULL,thread_write,NULL)){
		// an error occured
		perror("Error creating second thread\n"); 
		//exit(1); 
	} 

}

/*
 * the following calls are used to write a sector to the disk
 * the nonblocking call must return promptly, returning 1 if successful at
 * queueing up the write, 0 if not (in case internal buffers are full)
 * the blocking call will usually return promptly, but there may be
 * a delay while it waits for space in your buffers.
 * neither call should delay until the sector is actually written to the disk
 * for a successful nonblocking call and for the blocking call, a voucher is
 * returned that is required to determine the success/failure of the write
 */
void blocking_write_sector(SectorDescriptor sd, Voucher *v){

	struct voucher *vWrite = (struct voucher*) malloc(sizeof(struct voucher));
	if(vWrite==NULL) {printf("Error"); return;} //failed allocating space for voucher
	vWrite->rw = 0;
	vWrite->isDone = 0;
	vWrite->sd = sd;
	pthread_mutex_init(&(vWrite->mutex), NULL);
	pthread_cond_init(&(vWrite->cond), NULL);
	*v = (Voucher) vWrite;
	blockingWriteBB(bb_write, *v); //write to the write buffer
}

int nonblocking_write_sector(SectorDescriptor sd, Voucher *v){

	struct voucher *vWrite = (struct voucher*) malloc(sizeof(struct voucher));
	if(vWrite==NULL) {printf("Error"); return -1;} //failed allocating space for voucher
	vWrite->rw = 0;
	vWrite->isDone = 0;
	vWrite->sd = sd;
	pthread_mutex_init(&(vWrite->mutex), NULL);
	pthread_cond_init(&(vWrite->cond), NULL);
	*v = (Voucher) vWrite;
	return nonblockingWriteBB(bb_write,*v); //write to the write buffer
}

/*
 * the following calls are used to initiate the read of a sector from the disk
 * the nonblocking call must return promptly, returning 1 if successful at
 * queueing up the read, 0 if not (in case internal buffers are full)
 * the blocking callwill usually return promptly, but there may be
 * a delay while it waits for space in your buffers.
 * neither call should delay until the sector is actually read from the disk
 * for successful nonblocking call and for the blocking call, a voucher is
 * returned that is required to collect the sector after the read completes.
 */
void blocking_read_sector(SectorDescriptor sd, Voucher *v){

	struct voucher *vRead = (struct voucher*) malloc(sizeof(struct voucher));
	if(vRead==NULL) {printf("Error"); return;} //failed allocating space for voucher
	vRead->rw = 1;
	vRead->isDone = 0;
	vRead->sd = sd;
	pthread_mutex_init(&(vRead->mutex), NULL);
	pthread_cond_init(&(vRead->cond), NULL);
	*v = (Voucher) vRead;
	blockingWriteBB(bb_read, *v); //write to the read buffer
}

int nonblocking_read_sector(SectorDescriptor sd, Voucher *v){

	struct voucher *vRead = (struct voucher*) malloc(sizeof(struct voucher));
	if(vRead==NULL) {printf("Error"); return -1;} //failed allocating space for voucher
	vRead->rw = 1;
	vRead->isDone = 0;
	vRead->sd = sd;
	pthread_mutex_init(&(vRead->mutex), NULL);
	pthread_cond_init(&(vRead->cond), NULL);
	*v = (Voucher) vRead;
	return nonblockingWriteBB(bb_read,*v); //write to the read buffer
}

/*
 * the following call is used to retrieve the status of the read or write
 * the return value is 1 if successful, 0 if not
 * the calling application is blocked until the read/write has completed
 * if a successful read, the associated SectorDescriptor is returned in sd
 */

int redeem_voucher(Voucher v, SectorDescriptor *sd){

	if (v == NULL){
		printf("Driver: null voucher redeemed!\n");
		return 0;
	}

	struct voucher *tempV = (struct voucher*) v;
	int isDone = 0;
	pthread_mutex_lock(&(tempV->mutex));

	if(tempV->rw == 1)
		*sd = tempV->sd;
	else
		blocking_put_sd(fsds, tempV->sd);

	isDone = tempV->isDone;
	//pthread_cond_destroy(&(tempV->cond));
	pthread_mutex_unlock(&(tempV->mutex));
	pthread_mutex_destroy(&(tempV->mutex));

	if(tempV->isDone == 1) return 1;
	else return 0;
}
