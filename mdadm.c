#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"
#include "jbod.h"
#include "mdadm.h"
#include "net.h"

int is_mounted = 0;
int is_written = 0;

//declarations for mounted/unmounted tracking
int mounted = 0;
int mount_suc = 1;
int unmount_suc = 1;

//declarations for write permission
int w_permission = -1;
int permission_suc = 1;
int revoke_suc = 1;

// this functon packs bytes for op
uint32_t pack_bytes(uint32_t a, uint32_t b, uint32_t c, uint32_t d){
    //set up local variables
    uint32_t retval = 0x0, disk, block, command, reserved;

    disk = a;
    block = (b) << 4;
    command = (c) << 12;
    reserved = (d) << 20; 

    retval = disk | block | command | reserved;
    return retval;
  }

int mdadm_mount(void) {
	//printf("inside mounted\n");
	// YOUR CODE GOES HERE
	if (mounted==0){
		uint32_t mount_op = pack_bytes(0,0,JBOD_MOUNT,0);
    	mount_suc = jbod_client_operation(mount_op, NULL);
		//printf("mount_suc: %i\n", mount_suc);   
    //check if mount succeeded
    if (mount_suc == 0){
      	mounted = 1;
      	return 1;
    }
  	}
  	return -1;
}

int mdadm_unmount(void) {
  
	// YOUR CODE GOES HERE
	//check if mounted
    if (mounted==1){
	uint32_t unmount_op = pack_bytes(0,0,JBOD_UNMOUNT,0);
    unmount_suc = jbod_client_operation(unmount_op, NULL);
    //check if unmount succeeded
    if (unmount_suc ==0){
      mounted=0;
      return 1;
    }
    }
    return -1;
}

int mdadm_write_permission(void){
 
	// YOUR CODE GOES HERE
	// check if permission revoked
	if (w_permission == -1){
		uint32_t writeperm_op = pack_bytes(0,0,JBOD_WRITE_PERMISSION,0);
		permission_suc = jbod_client_operation(writeperm_op, NULL);
	// check if permission succeeded
	if (permission_suc == 0){
		w_permission = 1;
		return 1;
	}
	}
	return -1;
}


int mdadm_revoke_write_permission(void){

	// YOUR CODE GOES HERE
	//check if mounted
    if (w_permission==1){
	uint32_t revokeperm_op = pack_bytes(0,0,JBOD_REVOKE_WRITE_PERMISSION,0);
    revoke_suc = jbod_client_operation(revokeperm_op, NULL);
    //check if unmount succeeded
    if (revoke_suc ==0){
      w_permission=-1;
      return 1;
    }
    }
    return -1;
}


int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
	// YOUR CODE GOES HERE
	// The following are cases for ivalid input
	if (mounted == 0){
		return -1;
	}
	if (start_addr + read_len > 1048576){
		return -1;
	}
	if (read_len > 0 && read_buf == NULL){
		return -1;
	}
	if (read_len>1024){
		return -1;
	}

	// create temp buffer
	uint8_t tempbuf[256];

	//create variables to track read and left bytes
	uint32_t read_bytes = 0;
	uint32_t left_bytes = read_len;

	int disknum;
	int blocknum;

	//call function to pack bytes from lecture(modified)
	uint32_t pack_bytes(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

	//declarations
	uint32_t offset = start_addr;
	uint8_t *start_address;
	uint32_t toread;

  //while loop to copy data into read_buf until read_len is satisfied
  	while (read_bytes < read_len){

		//calculate current disk and current block
		disknum = (start_addr+ read_bytes)/65536;
		blocknum = ((start_addr+ read_bytes)%65536)/256;

		//pack bytes
		uint32_t op = pack_bytes(disknum, blocknum, JBOD_SEEK_TO_DISK, 0);
		uint32_t op1 = pack_bytes(disknum, blocknum, JBOD_SEEK_TO_BLOCK, 0);
		uint32_t op2 = pack_bytes(disknum, blocknum, JBOD_READ_BLOCK, 0);


		if (cache_enabled()){
			int ret = cache_lookup(disknum, blocknum, tempbuf);
			if (ret == 1){
				}
					else{
					//seek disk
					jbod_client_operation(op, NULL);

					//seek block
					jbod_client_operation(op1, NULL);

					//read to temp buffer
					jbod_client_operation(op2, tempbuf);

					cache_insert(disknum,blocknum,tempbuf);
					}
		}
		else{
		//seek disk
		jbod_client_operation(op, NULL);

		//printf("Ok: %d", ok);

		//seek block
		jbod_client_operation(op1, NULL);

		//read to temp buffer
		jbod_client_operation(op2, tempbuf);
		}

		// want to set amount of bytes to read
		uint32_t left_in_block =(disknum * 65536)+(1 + blocknum) * 256 - offset;

		if (left_in_block >= left_bytes){
		toread = left_bytes;
		left_bytes = left_bytes-toread;
		}
		else if (left_in_block < left_bytes){
		toread = left_in_block;
		left_bytes = left_bytes - toread;
		}

		// the following is the start address for the first iteration
		if(read_bytes==0){
		start_address = &tempbuf[start_addr -(65536*disknum)-(blocknum*256)];
		}
		
		//copy to read buffer
		memcpy(read_buf, start_address, toread);

		//increase address where bytes are copied
		read_buf += toread;

		//increase address for where bytes are added from
		start_address = tempbuf;
		
		// update offset 
		offset += toread;

		//update amount of already read bytes
		read_bytes = read_bytes + toread;
	}  
  	return read_len;
}




int mdadm_write(uint32_t start_addr, uint32_t write_len, const uint8_t *write_buf) {

	// YOUR CODE GOES HERE
	// The following are cases for ivalid input
	if (mounted == 0){
		return -1;
	}
	//printf("exits after second\n");
	if (start_addr + write_len > 1048576){
		return -1;
	}
	//printf("exits after third\n");
	if (write_len > 0 && write_buf == NULL){
		return -1;
	}
	//printf("exits after fourth\n");
	if (write_len>1024){
		return -1;
	}
	//printf("exits after fifth\n");
	if (w_permission == -1){
		return -1;
	}

	//printf("after sisth\n");

	uint8_t temp_buf[256];

	uint32_t written = 0;
	uint32_t towrite_bytes = write_len;

	int disk_num;
	int block_num;

	uint32_t offset = start_addr;
	uint8_t *start_address;
	uint32_t towrite;

	while (written < write_len){
		//printf("hello\n");
		disk_num = (start_addr + written)/65536;
		block_num = ((start_addr + written)%65536)/256;

		//printf("2\n");
		int found = -1;
		uint32_t seek_disk = pack_bytes(disk_num, 0, JBOD_SEEK_TO_DISK, 0);
		uint32_t seek_block = pack_bytes(0, block_num, JBOD_SEEK_TO_BLOCK, 0);
		uint32_t reading = pack_bytes(0, 0, JBOD_READ_BLOCK, 0);

		//printf("3\n");

		if (cache_enabled()){
			if (cache_lookup(disk_num, block_num, temp_buf)== 1){		
				found = 1;
			}
			else{
				//printf("4: inside cache\n");
				found = 0;
				jbod_client_operation(seek_disk, NULL);

				jbod_client_operation(seek_block, NULL);

				jbod_client_operation(reading, temp_buf);
			}
		}
		else{
		//printf("4: outside cache\n");
		printf("sent this\n");

		jbod_client_operation(seek_disk, NULL);

		jbod_client_operation(seek_block, NULL);

		jbod_client_operation(reading, temp_buf);

		}

		//printf("5\n");

		// want to set amount of bytes to write
		uint32_t left_in_block1 =(disk_num * 65536)+(1 + block_num) * 256 - offset;

		if (left_in_block1 >= towrite_bytes){
		towrite = towrite_bytes;
		towrite_bytes = towrite_bytes-towrite;
		}
		else if (left_in_block1 < towrite_bytes){
		towrite = left_in_block1;
		towrite_bytes = towrite_bytes - towrite;
		}

		if(written==0){
		start_address = &temp_buf[start_addr -(65536*disk_num)-(block_num*256)];
		}

		if (disk_num == 0 && block_num == 0){
			start_address = &temp_buf[start_addr];
		}

		memcpy(start_address, write_buf, towrite);
		
		if (found==1){
			cache_update(disk_num, block_num, temp_buf);
		}

		uint32_t op = pack_bytes(disk_num, block_num, JBOD_SEEK_TO_DISK, 0);
		uint32_t op1 = pack_bytes(disk_num, block_num, JBOD_SEEK_TO_BLOCK, 0);
		uint32_t op2 = pack_bytes(disk_num, block_num, JBOD_WRITE_BLOCK, 0);

		//printf("%d", op);
		//printf("%d", op1);
		//printf("%d", op2);

		//seek disk
		jbod_client_operation(op, NULL);

		//seek block
		jbod_client_operation(op1, NULL);

		//write from
		jbod_client_operation(op2, temp_buf);

		//increase address where bytes are copied from
		write_buf += towrite;

		//increase address for where bytes are added to
		start_address = temp_buf;
		
		// update offset 
		offset += towrite;

		//update amount of already written bytes
		written = written + towrite;

		//counter++;

		}

	return write_len;
}
