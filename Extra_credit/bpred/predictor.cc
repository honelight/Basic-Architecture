#include <assert.h>
#include "predictor.h"


#define PHT_CTR_MAX  3
#define PHT_CTR_INIT 2

// These are hard coded for this assignment

#define HIST_LEN        16
#define TABLE_ENTRIES   (1<<16)

extern UINT32 PRED_TYPE;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

PREDICTOR::PREDICTOR(void){

  // Init for Last Time Predictor
  lastTimeTable = new UINT32[TABLE_ENTRIES];
  for(UINT32 ii=0; ii< TABLE_ENTRIES; ii++){
    lastTimeTable[ii]=NOT_TAKEN;
  }


  // Init for TwoBit Counter
  twoBitCounterTable = new UINT32[TABLE_ENTRIES];
  for(UINT32 ii=0; ii< TABLE_ENTRIES; ii++){
    twoBitCounterTable[ii]=0;
  }


  // Init for Two Level Predictor
  historyLength    = HIST_LEN;
  GHR              = 0;
  numPhtEntries    = (1<< HIST_LEN);

  PHT = new UINT32[numPhtEntries];

  for(UINT32 ii=0; ii< numPhtEntries; ii++){
    PHT[ii]=PHT_CTR_INIT; 
  }
  
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool   PREDICTOR::GetPrediction(UINT32 PC){

  switch(PRED_TYPE){

  case PRED_TYPE_NEVERTAKEN: 
    return NOT_TAKEN;

  case PRED_TYPE_ALWAYSTAKEN: 
    return TAKEN;

  case PRED_TYPE_LAST_TIME:
     return GetPredictionLastTimePred(PC);

  case PRED_TYPE_TWOBIT_COUNTER:
    return GetPredictionTwoBitCounterPred(PC);

  case PRED_TYPE_TWOLEVEL_PRED:
    return GetPredictionTwoLevelPred(PC);

  default: printf("Undefined Predictor Type\n");
           exit(-1);
  }
  
}


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void  PREDICTOR::UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir){

  switch(PRED_TYPE){

  case PRED_TYPE_NEVERTAKEN: 
    return; 

  case PRED_TYPE_ALWAYSTAKEN: 
    return; 

  case PRED_TYPE_LAST_TIME:
    UpdateLastTimePred(PC, resolveDir, predDir);
    return;

  case PRED_TYPE_TWOBIT_COUNTER:
    UpdateTwoBitCounterPred(PC, resolveDir, predDir);
    return;

  case PRED_TYPE_TWOLEVEL_PRED:
    UpdateTwoLevelPred(PC, resolveDir, predDir);
    return;

  default: printf("Undefined Predictor Type\n");
           exit(-1);
  }

}

/////////////////////////////////////////////////////////////
//  LAST TIME PREDICTOR IS ALREADY IMPLEMENTED
/////////////////////////////////////////////////////////////

bool   PREDICTOR::GetPredictionLastTimePred(UINT32 PC){

  UINT32 tableIndex   = (PC) % TABLE_ENTRIES; // Need to index the table
  UINT32 tableEntry= lastTimeTable[tableIndex];
  
  if(tableEntry == TAKEN){
    return TAKEN; 
  }else{
    return NOT_TAKEN; 
  }

}

/////////////////////////////////////////////////////////////
//  LAST TIME PREDICTOR IS ALREADY IMPLEMENTED
/////////////////////////////////////////////////////////////

void  PREDICTOR::UpdateLastTimePred(UINT32 PC, bool resolveDir, bool predDir){

  UINT32 tableIndex   = (PC) % TABLE_ENTRIES; // Need to index the table

  if(resolveDir==TAKEN){
    lastTimeTable[tableIndex]=TAKEN;  // store the resolved direction
  }else{
    lastTimeTable[tableIndex]=NOT_TAKEN;  // store the resolved direction
  }

}


//////////////////////////////////////////////////////////////////////
/////////////// DO NOT MODIFY THE CODE ABOVE THIS LINE ///////////////
//////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////
// YOU NEED TO IMPLEMENT TWO BIT COUNTER PREDICTOR
/////////////////////////////////////////////////////////////

static int states;

bool   PREDICTOR::GetPredictionTwoBitCounterPred(UINT32 PC){

	UINT32 tableIndex = (PC) % TABLE_ENTRIES; // Need to index the table
	UINT32 tableEntry = twoBitCounterTable[tableIndex];
	states = tableEntry;
	if (tableEntry == 2||tableEntry==3){
		return TAKEN;
	}
	else{
		return NOT_TAKEN;
	}
}

/////////////////////////////////////////////////////////////
// YOU NEED TO IMPLEMENT TWO BIT COUNTER PREDICTOR UPDATE
/////////////////////////////////////////////////////////////


void  PREDICTOR::UpdateTwoBitCounterPred(UINT32 PC, bool resolveDir, bool predDir){
	UINT32 tableIndex = (PC) % TABLE_ENTRIES; // Need to index the table

	if (resolveDir == TAKEN){
		if (states == 3)
		{
			twoBitCounterTable[tableIndex] = states;  // store the resolved direction
		}
		else
		{
			states++;
			twoBitCounterTable[tableIndex] = states;
		}
	}
	else{
		if (states == 0)
		{
			twoBitCounterTable[tableIndex] = states;  // store the resolved direction
		}
		else
		{
			states--;
			twoBitCounterTable[tableIndex] = states;
		}
	}

}



/////////////////////////////////////////////////////////////
// YOU NEED TO IMPLEMENT TWO LEVEL PREDICTOR
/////////////////////////////////////////////////////////////

static int two_level_states;
static unsigned short history_register=0;
bool   PREDICTOR::GetPredictionTwoLevelPred(UINT32 PC){
	UINT32 tableIndex = history_register; // Need to index the table
	UINT32 tableEntry = PHT[tableIndex];
	two_level_states = tableEntry;
	if (tableEntry == 2 || tableEntry == 3){
		return TAKEN;
	}
	else{
		return NOT_TAKEN;
	}
}

/////////////////////////////////////////////////////////////
// YOU NEED TO IMPLEMENT TWO LEVEL PREDICTOR UPDATE
/////////////////////////////////////////////////////////////


void  PREDICTOR::UpdateTwoLevelPred(UINT32 PC, bool resolveDir, bool predDir){

	UINT32 tableIndex = history_register; // Need to index the table
	history_register = history_register << 1;
	if (resolveDir == TAKEN){
		if (two_level_states == 3)
		{
			PHT[tableIndex] = two_level_states;  // store the resolved direction
			history_register++;
		}
		else
		{
			two_level_states++;
			PHT[tableIndex] = two_level_states;
			history_register++;
		}
	}
	else{
		if (two_level_states == 0)
		{
			PHT[tableIndex] = two_level_states;  // store the resolved direction
		}
		else
		{
			two_level_states--;
			PHT[tableIndex] = two_level_states;
		}
	}

}
