#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "cache.h"



extern uns64 cycle_count; // You can use this as timestamp for LRU

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE INIT FUNCTION -----------
////////////////////////////////////////////////////////////////////

Cache  *cache_new(uns64 size, uns64 assoc, uns64 linesize, uns64 repl_policy){

   Cache *c = (Cache *) calloc (1, sizeof (Cache));
   c->num_ways = assoc;
   c->repl_policy = repl_policy;

   if(c->num_ways > MAX_WAYS){
     printf("Change MAX_WAYS in cache.h to support %llu ways\n", c->num_ways);
     exit(-1);
   }

   // determine num sets, and init the cache
   c->num_sets = size/(linesize*assoc);
   c->sets  = (Cache_Set *) calloc (c->num_sets, sizeof(Cache_Set));

   return c;
}

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE PRINT STATS FUNCTION -----------
////////////////////////////////////////////////////////////////////

void    cache_print_stats    (Cache *c, char *header){
  double read_mr =0;
  double write_mr =0;

  if(c->stat_read_access){
    read_mr=(double)(c->stat_read_miss)/(double)(c->stat_read_access);
  }

  if(c->stat_write_access){
    write_mr=(double)(c->stat_write_miss)/(double)(c->stat_write_access);
  }

  printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
  printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
  printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
  printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
  printf("\n%s_READ_MISSPERC  \t\t : %10.3f", header, 100*read_mr);
  printf("\n%s_WRITE_MISSPERC \t\t : %10.3f", header, 100*write_mr);
  printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);

  printf("\n");
}



////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Return HIT if access hits in the cache, MISS otherwise
// Also if mark_dirty is TRUE, then mark the resident line as dirty
// Update appropriate stats
////////////////////////////////////////////////////////////////////

Flag    cache_access(Cache *c, Addr lineaddr, uns mark_dirty){
  Flag outcome=MISS;
  int way_count=c->num_ways;
  int location_count=c->num_sets;
  int bits_count=log2(location_count);
  //printf("%d\n",lineaddr);

  //printf("%d %d\n",way_count, location_count);

  int last_bits=lineaddr&(location_count-1);
  Addr input_tag=lineaddr>>bits_count;
  int i;
  //printf("%d\n", last_bits);
  if(mark_dirty==TRUE)
  {
      c->stat_write_access++;
  }
  else if(mark_dirty==FALSE)
  {
      c->stat_read_access++;
  }
  Cache_Set test_set=c->sets[last_bits];

  for(i=0;i<way_count;i++)
  {
      Addr test_address=test_set.line[i].tag;
      if(test_address==input_tag)
      {
          //printf("%d %d\n",input_tag,test_address);
          outcome=HIT;
          c->sets[last_bits].line[i].last_access_time=cycle_count;
          if(mark_dirty==TRUE)
          {
              c->sets[last_bits].line[i].dirty=mark_dirty;
          }
          break;
      }
      else if(c->sets[last_bits].line[i].valid==0)
      {
          outcome=MISS;
          break;
      }

  }

  if(outcome==MISS)
  {
      if(mark_dirty==TRUE)
      {
          c->stat_write_miss++;
      }
      else if(mark_dirty==FALSE)
      {
          c->stat_read_miss++;
      }
  }

  return outcome;
}

////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Install the line: determine victim using repl policy (LRU/RAND)
// copy victim into last_evicted_line for tracking writebacks
////////////////////////////////////////////////////////////////////

void    cache_install(Cache *c, Addr lineaddr, uns mark_dirty){

  // Your Code Goes Here
  // Note: You can use cycle_count as timestamp for LRU

  int replace=1;
  int way_count=c->num_ways;
  int location_count=c->num_sets;
  int bits_count=log2(location_count);
  //printf("%d",location_count);
  int last_bits=lineaddr&(location_count-1);
  Addr input_tag=lineaddr>>bits_count;
  int i;
  for(i=0;i<way_count;i++)
  {
      if(c->sets[last_bits].line[i].valid==0)
      {
          c->sets[last_bits].line[i].tag=input_tag;
          c->sets[last_bits].line[i].valid=TRUE;
          c->sets[last_bits].line[i].dirty=mark_dirty;
          c->sets[last_bits].line[i].last_access_time=cycle_count;
          replace=0;
          break;
      }
  }
  if(replace==1 && c->repl_policy==0)
  {
      int min_cycle_number=0;
      unsigned int min_cycle_block=c->sets[last_bits].line[0].last_access_time;
      for(i=0;i<way_count;i++)
      {
          if(c->sets[last_bits].line[i].last_access_time<min_cycle_block)
          {
              min_cycle_number=i;
              min_cycle_block=c->sets[last_bits].line[i].last_access_time;
          }
      }
      if(c->sets[last_bits].line[min_cycle_number].dirty==TRUE)
      {
          c->stat_dirty_evicts++;
      }
	  c->last_evicted_line = c->sets[last_bits].line[min_cycle_number];
      c->sets[last_bits].line[min_cycle_number].tag=input_tag;
      c->sets[last_bits].line[min_cycle_number].valid=TRUE;
      c->sets[last_bits].line[min_cycle_number].dirty=mark_dirty;
      c->sets[last_bits].line[min_cycle_number].last_access_time=cycle_count;
      //printf("%d %d\n",last_bits, min_cycle_number);
  }
  else if(replace==1 && c->repl_policy==1)
  {
      int rand_value=rand()%way_count;
      if(c->sets[last_bits].line[rand_value].dirty==TRUE)
      {
          c->stat_dirty_evicts++;
      }
	  c->last_evicted_line = c->sets[last_bits].line[rand_value];
      c->sets[last_bits].line[rand_value].tag=input_tag;
      c->sets[last_bits].line[rand_value].valid=TRUE;
      c->sets[last_bits].line[rand_value].dirty=mark_dirty;
      c->sets[last_bits].line[rand_value].last_access_time=cycle_count;

  }
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


