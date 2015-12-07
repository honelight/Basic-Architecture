#ifndef MEMSYS_H
#define MEMSYS_H

#include "types.h"
#include "cache.h"

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

typedef struct Memsys   Memsys;

struct Memsys {
  Cache *dcache;  


   // stats 
  uns64 stat_ifetch_access;
  uns64 stat_load_access;
  uns64 stat_store_access;
  uns64 stat_ifetch_delay;
  uns64 stat_load_delay;
  uns64 stat_store_delay;
};



///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

Memsys *memsys_new();
void    memsys_print_stats(Memsys *sys);

uns64   memsys_access(Memsys *sys, Addr addr, Access_Type type);
uns64   memsys_access_modeA(Memsys *sys, Addr lineaddr, Access_Type type);




///////////////////////////////////////////////////////////////////

#endif // MEMSYS_H
