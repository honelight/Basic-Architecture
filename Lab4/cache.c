#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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
  int count=0;
  int replace=1;
  int line_size=sizeof(Cache_Line);
  int set_size=sizeof(Cache_Set)/sizeof(Cache_Line);
  int location_count=c->num_ways;
  int read_or_write=0;

  //printf("%d",location_count);

  int last_six_bit=lineaddr&0x3F;
  Addr input_tag=lineaddr>>6;
  int i;
  //printf("%d\n", line_size);
  if(mark_dirty==TRUE)
  {
      c->stat_write_access++;
  }
  else if(mark_dirty==FALSE)
  {
      c->stat_read_access++;
  }

  for(i=0;i<location_count;i++)
  {
      Cache_Set test_set=c->sets[i];
      Addr test_address=test_set.line[last_six_bit].tag;
      //printf("%d %d \n", test_address, input_tag);
      if(test_address==input_tag)
      {
          outcome=HIT;
          c->sets[i].line[last_six_bit].last_access_time=cycle_count;
          if(mark_dirty==TRUE)
          {
              c->sets[i].line[last_six_bit].dirty=mark_dirty;
          }
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

  int count=0;
  int replace=1;
  int line_size=sizeof(Cache_Line);
  int set_size=sizeof(Cache_Set)/sizeof(Cache_Line);
  //int location_count=sizeof(c->sets);
  int location_count=c->num_ways;

  int last_six_bit=lineaddr&0x3F;
  Addr input_tag=lineaddr>>6;
  int i;
  for(i=0;i<location_count;i++)
  {
      Cache_Set test_set=c->sets[i];
      Addr test_address=test_set.line[last_six_bit].tag;
      if(test_address==0)
      {
          c->sets[i].line[last_six_bit].tag=input_tag;
          c->sets[i].line[last_six_bit].valid=TRUE;
          c->sets[i].line[last_six_bit].dirty=mark_dirty;
          c->sets[i].line[last_six_bit].last_access_time=cycle_count;
          replace=0;
          break;
      }
  }
  if(replace==1)
  {
      int min_cycle_number=0;
      unsigned int min_cycle_block=c->sets[0].line[last_six_bit].last_access_time;
      for(i=0;i<location_count;i++)
      {
          if(c->sets[i].line[last_six_bit].last_access_time<min_cycle_block)
          {
              min_cycle_number=i;
              min_cycle_block=c->sets[i].line[last_six_bit].last_access_time;
          }
      }
      if(c->sets[min_cycle_number].line[last_six_bit].dirty==TRUE)
      {
          c->stat_dirty_evicts++;
      }
      c->last_evicted_line=c->sets[min_cycle_number].line[last_six_bit];
      c->sets[min_cycle_number].line[last_six_bit].tag=input_tag;
      c->sets[min_cycle_number].line[last_six_bit].valid=TRUE;
      c->sets[min_cycle_number].line[last_six_bit].dirty=mark_dirty;
      c->sets[min_cycle_number].line[last_six_bit].last_access_time=cycle_count;
  }

}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


