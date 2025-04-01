#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

// start defining things
struct sockaddr_in caddr;

// this function is for debugging purposes only
void print_block(uint8_t *buf){
    for (int i = 0; i<256; i++){
        if (i != 255){
            printf("%X, ", buf[i]);
        }
        else{
            printf("%X\n", buf[i]);
        }
    }
}

void print_packet(uint8_t *buf){
    for (int i = 0; i<261; i++){
        if (i != 260){
            printf("%X, ", buf[i]);
        }
        else{
            printf("%X\n", buf[i]);
        }
    }
}


/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
bool nread(int fd, int len, uint8_t *buf) {

    int read_bytes = 0;
    int to_read = len;

    while (read_bytes < to_read){
        ssize_t read_res = read(fd, buf+read_bytes, to_read - read_bytes);
        //printf("read_res: %li\n", read_res);
        if (read_res == -1){
            fprintf(stderr, "read() failed.\n");
            return false;
        }
        else if(read_res==0){
            continue;
        }
        else{
            read_bytes += read_res;
        }
    }

    return true;
    
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
bool nwrite(int fd, int len, uint8_t *buf) {

    int written = 0;
    int to_write = len;

    while (written < to_write){
        ssize_t res = write(fd, buf + written, to_write - written);
        if(res == 0){
            continue;
        }
        else if (res == -1){
            fprintf(stderr, "write() failed.\n");
            return false;
        }
        else{
            written += res;
        }
    }

    return true;
    
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
bool recv_packet(int fd, uint32_t *op, uint8_t *ret, uint8_t *block) {

    uint8_t ret_pack[HEADER_LEN+JBOD_BLOCK_SIZE];
    int len;

    len = HEADER_LEN;

    if (nread(fd, len, ret_pack)==false){
        fprintf(stderr, "nread failed.\n");
        return false;
    }

    memcpy(op, ret_pack, sizeof(uint32_t));
    *op = ntohl(*op);

    //printf("op: %i\n", *op);

    memcpy(ret, ret_pack + sizeof(uint32_t), sizeof(uint8_t));

    if (*ret == 0x02 || *ret == 0x03){
        len = JBOD_BLOCK_SIZE;
        if (nread(fd, len, ret_pack+HEADER_LEN)==false){       // added length to ret_pack here
            fprintf(stderr, "nread failed.\n");
            return false;
        }
        memcpy(block, ret_pack+HEADER_LEN, JBOD_BLOCK_SIZE);
        //print_block(block);
        //printf("\n");
    }

    return true;
    
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
bool send_packet(int sd, uint32_t op, uint8_t *block) {

    // pack the packet

    uint8_t info;   // info code starts as zero when sending message

    // check if block has payload and set 2nd lowest bit of 5th byte to 1
    if (block){
        info = 0x02;
    }
    else{
        info = 0x00;
    }

    uint8_t packet[HEADER_LEN + JBOD_BLOCK_SIZE]; //create a buffer

    int len;

    uint32_t network_op = htonl(op);

    memcpy(packet, &network_op, sizeof(uint32_t));

    memcpy(packet+sizeof(uint32_t), &info, sizeof(uint8_t));

    //packet[4] = info;

    if (block){
        memcpy(packet+HEADER_LEN, block, JBOD_BLOCK_SIZE);
        len = HEADER_LEN + JBOD_BLOCK_SIZE;
    }
    else{
        len = HEADER_LEN;
    }

    if (nwrite(sd, len, packet) == false){
        fprintf(stderr, "nwrite failed.\n");
        return false;
    }

    return true;
}

/* connect to server and set the global client variable to the socket */
bool jbod_connect(const char *ip, uint16_t port) {

  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(port);
  if (inet_aton(ip, &caddr.sin_addr)==0){
    return false;
  }
  // create a socket
  cli_sd = socket(PF_INET, SOCK_STREAM,0);

  // check if socket creation was successfull
  if (cli_sd == -1){
    return false;
  }

  // attempt to connect
  if (connect(cli_sd, (const struct sockaddr *)&caddr, sizeof(caddr)) == -1){
    return false;
  }

  return true;
}

// just close the connection
void jbod_disconnect(void) {
  if (cli_sd != -1){
    close(cli_sd);
    cli_sd = -1;
  }
}

int jbod_client_operation(uint32_t op, uint8_t *block) {

    if (send_packet(cli_sd,op,block) == false){
        printf("Send packet failed.");
        return -1;
    }

    uint32_t ret_op;
    uint8_t ret_info;
    uint8_t ret_block[JBOD_BLOCK_SIZE];

    if (!recv_packet(cli_sd, &ret_op, &ret_info, ret_block)) {
        fprintf(stderr, "Receive packet failed.\n");
        return -1;
    }

    // Check if the received opcode matches the sent one
    if (op != ret_op) {
        fprintf(stderr, "Received unexpected opcode. op = %i, ret = %i\n", op, ret_op);
        return -1;
    }
    
    if (ret_info == 0x02 || ret_info == 0x03){
        memcpy(block, ret_block, JBOD_BLOCK_SIZE);
    }
    
    return 0;

}









// reading

  //use while loop
  // use write() or read()
/*
    int read_bytes = 0;
    while (read_bytes < len) {
        ssize_t result = read(fd, buf + read_bytes, len - read_bytes);
        if (result == -1) {
            if (errno == EINTR) {
                continue;  // interrupted, try again
            } else {
                perror("read");
                return false;
            }
        } else if (result == 0) {
            fprintf(stderr, "Unexpected EOF\n");
            return false;
        } else {
            read_bytes += result;
        }
    }
    */

       /*
    int written_bytes = 0;
    while (written_bytes < len) {
        ssize_t result = write(fd, buf + written_bytes, len - written_bytes);
        if (result == -1) {
            if (errno == EINTR) {
                continue;  // interrupted, try again
            } else {
                perror("write");
                return false;
            }
        } else {
            written_bytes += result;
        }
    }
    */

       /*
  //call nread() or nwrite()
    if (!nread(fd, HEADER_LEN, (uint8_t *)op)) {
        fprintf(stderr, "Failed to read packet header\n");
        return false;
    }

    uint8_t info_code;
    if (!nread(fd, 1, &info_code)) {
        fprintf(stderr, "Failed to read info code\n");
        return false;
    }

    int payload_size = JBOD_BLOCK_SIZE;
    if (info_code & 0x01) {
        payload_size = 0;  // Payload does not exist
    }

    if (!nread(fd, payload_size, block)) {
        fprintf(stderr, "Failed to read payload\n");
        return false;
    }

    if (ret) {
        *ret = info_code;
    }
*/



    /*
  //call nwrite()
    if (!nwrite(sd, HEADER_LEN, (uint8_t *)&op)) {
        fprintf(stderr, "Failed to write packet header\n");
        return false;
    }

    // Info code (always 0 for the client sending requests)
    uint8_t info_code = 0x00;

    // Check if block payload exists
    if (block) {
        info_code |= 0x01;
    }

    if (!nwrite(sd, 1, &info_code)) {
        fprintf(stderr, "Failed to write info code\n");
        return false;
    }

    int payload_size = JBOD_BLOCK_SIZE;
    if (info_code & 0x01) {
        // Payload exists, write the block
        if (!nwrite(sd, payload_size, block)) {
            fprintf(stderr, "Failed to write payload\n");
            return false;
        }
    }
    */

     // NEW start

  /*

    uint32_t ret_op;
    uint8_t ret_info;
    uint8_t ret_block[JBOD_BLOCK_SIZE];

    if (!recv_packet(cli_sd, &ret_op, &ret_info, ret_block)) {
        fprintf(stderr, "Receive packet failed.\n");
        return -1;
    }

    // Check if the received opcode matches the sent one
    if (op != ret_op) {
        fprintf(stderr, "Received unexpected opcode.\n");
        return -1;
    }

    // Process the return info code if needed

    // Copy the block payload if needed
    if (block) {
        memcpy(block, ret_block, JBOD_BLOCK_SIZE);
    }

  // new end
  
    */