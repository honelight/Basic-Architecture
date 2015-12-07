
#include <assert.h>
#include "tracer.h"

/////////////////////////////////////////
/////////////////////////////////////////

CBP_TRACER::CBP_TRACER(char *traceFileName){
  char  cmdString[1024];
  
  sprintf(cmdString,"gunzip -c %s", traceFileName);

  if ((traceFile = popen(cmdString, "r")) == NULL){
   printf("Unable to open the trace file. Dying\n");
   exit(-1);
  }

  numInst=0;
  numCondBranch=0;

}

/////////////////////////////////////////
/////////////////////////////////////////

bool  CBP_TRACER::GetNextRecord(CBP_TRACE_RECORD *rec){

  fread (&rec->PC, 4, 1, traceFile);
  fread (&rec->branchTarget, 4, 1, traceFile);
  fread (&rec->opType, 1, 1, traceFile);
  fread (&rec->branchTaken, 1, 1, traceFile);

  if(feof(traceFile)){
    return FAILURE; 
  }

  // sanity check
  assert(rec->opType < OPTYPE_MAX);

  // update trace stats and heartbeat
  numInst++;
  CheckHeartBeat();

  if(rec->opType == OPTYPE_BRANCH_COND){
    numCondBranch++;
  }

  return SUCCESS; 
}

/////////////////////////////////////////
/////////////////////////////////////////

void CBP_TRACER::CheckHeartBeat(){
  UINT64 dotInterval=1000000;
  UINT64 lineInterval=30*dotInterval;

  if(numInst-lastHeartBeat >= dotInterval){
    printf("."); 
    fflush(stdout);

    lastHeartBeat=numInst;

    if(numInst % lineInterval == 0){
      printf("\n");
      fflush(stdout);
    }

  }

}
/////////////////////////////////////////
/////////////////////////////////////////
