#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "cache.h"
#include "jbod.h"

//Uncomment the below code before implementing cache functioncs.
static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

static int created_cache = 0;

static int empty = -1;

static int MRU_index;

int cache_create(int num_entries) {
    if (num_entries < 2 || num_entries >4096){
      return -1;
    }
    if (created_cache == 1){
      return -1;
    }
    num_queries = 0;
    num_hits = 0;
    //for (int i = 0; i<)
    cache = calloc(num_entries, sizeof(cache_entry_t)); // the sizeof part might be a longshot
    
    if (cache != NULL){
      cache_size = num_entries;
      created_cache = 1;
      return 1;
    }
    return -1;
}

int cache_destroy(void) {
  if (created_cache == 0){
    return -1;
  }
  free(cache);
  cache = NULL;
  cache_size = 0;
  created_cache = 0;
  empty = -1;
  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  
  if (created_cache == 0){
    return -1;
  }
  /*
  if (num_queries == 0){
    return -1;
  }
  */
 
  if (empty < 0){
    return -1;
  }
  
  //num_queries = 10;

  int success = 0;
  if (buf == NULL){
    return -1;
  }
  num_queries++;

  
  for (int i = 0; i<cache_size; i++){
      if (cache[i].disk_num == disk_num && cache[i].block_num == block_num){
        memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);
        success = 1;
        clock++;
        cache[i].clock_accesses = clock;
        num_hits++;
        //printf("numhits: %d\n", num_hits);
        //MRU_index = i;
      }
  }

  if (success){
    return 1;
  }
  return -1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
    for (int i = 0; i<cache_size; i++){
    
    clock++;

    //entry is valid case

    if (cache[i].valid == true){

      if (cache[i].disk_num == disk_num && cache[i].block_num == block_num) {
      // Entry already exists, return failure
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
      cache[i].clock_accesses = clock;
    }
}
  }
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  
  // check for invalid parameters
  //printf("hello");
  empty = 0;

  if (created_cache == 0){
  return -1;
  }
  if (disk_num < 0 || disk_num > JBOD_NUM_DISKS){
    return -1;
  }
    if (block_num < 0 || block_num > JBOD_NUM_BLOCKS_PER_DISK){
    return -1;
  }
  if (buf == NULL){
    return -1;
  }

  for (int i = 0; i<cache_size; i++){
    
    clock++;

    //entry is valid case

    if (cache[i].valid == true){

      if (cache[i].disk_num == disk_num && cache[i].block_num == block_num) {
      // Entry already exists, return failure
      return -1;
    }

      //Review this: updates mru according to clok accesses
      //cache[i].clock_accesses = clock;

  if (cache[i].clock_accesses > cache[MRU_index].clock_accesses){
  MRU_index = i;
}
    }
    else{
        empty = 0;
        cache[i].valid = true;
        cache[i].disk_num = disk_num;
        cache[i].block_num = block_num;
        memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
        cache[i].clock_accesses = clock;
        return 1;
    }
  }

  cache[MRU_index].disk_num = disk_num;
  cache[MRU_index].block_num = block_num;
  memcpy(cache[MRU_index].block, buf, JBOD_BLOCK_SIZE);
  cache[MRU_index].clock_accesses = clock;


return 1;
}

bool cache_enabled(void) {
  if (created_cache == 1){
    return true;
  }
  return false;
}

void cache_print_hit_rate(void) {
	fprintf(stderr, "num_hits: %d, num_queries: %d\n", num_hits, num_queries);
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}

int cache_resize(int new_num_entries) {
  while(new_num_entries<cache_size){
    
  }
  cache = realloc(cache, new_num_entries);
  return 1;
}
