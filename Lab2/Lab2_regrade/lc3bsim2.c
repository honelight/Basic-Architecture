/***************************************************************/
/*                                                             */
/* LC-3b Simulator (Adapted from Prof. Yale Patt at UT Austin) */
/*                                                             */
/***************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
				      (x[J3] << 3) + (x[J2] << 2) +
				      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); }
int GetLSHF1(int *x)         { return(x[LSHF1]); }

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A
   There are two write enable signals, one for each byte. WE0 is used for
   the least significant byte of a word. WE1 is used for the most significant
   byte of a word. */

#define WORDS_IN_MEM    0x08000
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the bus */

typedef struct System_Latches_Struct{

int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

int READY;	/* ready bit */
  /* The ready bit is also latched as you dont want the memory system to assert it
     at a bad point in the cycle*/

int REGS[LC_3b_REGS]; /* register file. */

int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

int STATE_NUMBER; /* Current State Number - Provided for debugging */
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {

  eval_micro_sequencer();
  cycle_memory();
  eval_bus_drivers();
  drive_bus();
  latch_datapath_values();

  CURRENT_LATCHES = NEXT_LATCHES;

  CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {
    int i;

    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (CURRENT_LATCHES.PC == 0x0000) {
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {
    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
	cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {
    int address; /* this is a byte address */

    printf("\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {
    int k;

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%04x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%04x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%04x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%04x\n", BUS);
    printf("MDR          : 0x%04x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%04x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	printf("%d: 0x%04x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%04x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%04x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%04x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%04x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%04x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%04x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%04x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
	go();
	break;

    case 'M':
    case 'm':
	scanf("%i %i", &start, &stop);
	mdump(dumpsim_file, start, stop);
	break;

    case '?':
	help();
	break;
    case 'Q':
    case 'q':
	printf("Bye.\n");
	exit(0);

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
	    scanf("%d", &cycles);
	    run(cycles);
	}
	break;

    default:
	printf("Invalid Command\n");
	break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}

	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/************************************************************/
/*                                                          */
/* Procedure : init_memory                                  */
/*                                                          */
/* Purpose   : Zero out the memory array                    */
/*                                                          */
/************************************************************/
void init_memory() {
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}

	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files) {
    int i;
    init_control_store(ucode_filename);

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(program_filename);
	while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);

}

/***************************************************************/
/* --------- DO NOT MODIFY THE CODE ABOVE THIS LINE -----------*/
/***************************************************************/

/***************************************************************/
/* You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/

static int memory_value=0; //Here is the value output for the memory output
int PC_gate,ALU_gate,SHF_gate, MARMUX_gate,MDR_gate;

//import functions from lab 1
int read_word(int addr){
  assert( (addr & 0xFFFF0000) == 0 );
  return MEMORY[addr>>1][1]<<8 | MEMORY[addr>>1][0];
}

int read_byte(int addr){
  int bank=addr&1;
  assert( (addr & 0xFFFF0000) == 0 );
  return MEMORY[addr>>1][bank];
}

void write_byte(int addr, int value){
  int bank=addr&1;
  assert( (addr & 0xFFFF0000) == 0 );
  MEMORY[addr>>1][bank]= value & 0xFF;
}

void write_word(int addr, int value){
  assert( (addr & 0xFFFF0000) == 0 );
  MEMORY[addr>>1][1] = (value & 0x0000FF00) >> 8;
  MEMORY[addr>>1][0] = value & 0xFF;
}

int get_bits(int value, int start, int end){
  int result;
  assert(start >= end );
  result = value >> end;
  result = result % ( 1 << ( start - end + 1 ) );
  return result;
}

void setcc(int value){
  NEXT_LATCHES.N=0;
  NEXT_LATCHES.Z=0;
  NEXT_LATCHES.P=0;
  if ( value == 0 )      NEXT_LATCHES.Z=1;
  else if ( value & 0x8000 )  NEXT_LATCHES.N=1;
  else                   NEXT_LATCHES.P=1;
}

int LSHF(int value, int amount){
  return (value << amount) & 0xFFFF;
}

int RSHF(int value, int amount, int topbit ){
  int mask;
  mask = 1 << amount;
  mask -= 1;
  mask = mask << ( 16 -amount );

  return ((value >> amount) & ~mask) | ((topbit)?(mask):0); /* TBD */
}

int SEXT(int value, int topbit){
  int shift = sizeof(int)*8 - topbit;
  return (value << shift )>> shift;
}

int ZEXT(int value, int topbit){
return (value & ( (1<<(topbit+1))-1) );

}

int addr_result()
{
   int addr2mux_option=GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
   int addr1mux_option=GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);
   int lshf1_option=GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION);

   int addr1_output=0;
   int addr2_output=0;

   if(addr1mux_option==0)
   {
       addr1_output=CURRENT_LATCHES.PC;
   }
   else if(addr1mux_option==1)
   {
       addr1_output=CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR&0x01C0)>>6];
   }

   if (addr2mux_option==0)
   {
       addr2_output=0;
   }
   else if(addr2mux_option==1)
   {
       addr2_output=SEXT(CURRENT_LATCHES.IR,5);
   }
   else if(addr2mux_option==2)
   {
       addr2_output=SEXT(CURRENT_LATCHES.IR,9);
   }
   else if(addr2mux_option==3)
   {
       addr2_output=SEXT(CURRENT_LATCHES.IR,11);
   }

   if(lshf1_option==1)
   {
       addr2_output=Low16bits(addr2_output<<1);
   }
   return Low16bits(addr1_output+addr2_output);
}


void eval_micro_sequencer() {
   int condition_code=GetCOND(CURRENT_LATCHES.MICROINSTRUCTION);
   int condition_code_0=condition_code & 0x0001;
   int condition_code_1=(condition_code & 0X0002)>>1;
   int BEN=CURRENT_LATCHES.BEN;
   int R=CURRENT_LATCHES.READY;
   int opcode=get_bits(CURRENT_LATCHES.IR,15,12);
   int IR11=get_bits(CURRENT_LATCHES.IR,11,11);

   int j_code=GetJ(CURRENT_LATCHES.MICROINSTRUCTION);
   int J_2=(j_code&0x0004)>>2;
   int J_1=(j_code&0x0002)>>1;
   int J_0=j_code&0x0001;

   int ready_bit=(!condition_code_1 & condition_code_0 & R);
   int branch_bit=(condition_code_1 & !condition_code_0 & BEN);
   int addr_mode_bit=(condition_code_1 & condition_code_0 & IR11);
   int next_state_code=(j_code&0x0038)|((branch_bit|J_2)<<2)|((ready_bit|J_1)<<1)|((addr_mode_bit|J_0));


   if(GetIRD(CURRENT_LATCHES.MICROINSTRUCTION)==0)
   {
      int output_bits=0;
      NEXT_LATCHES.STATE_NUMBER=Low16bits(next_state_code);
      for(output_bits=0; output_bits<CONTROL_STORE_BITS;output_bits++)
      {
          NEXT_LATCHES.MICROINSTRUCTION[output_bits]=Low16bits(CONTROL_STORE[next_state_code][output_bits]);
      }
      NEXT_LATCHES.STATE_NUMBER=Low16bits(next_state_code);
   }
   else
   {
       int output_bits=0;
       NEXT_LATCHES.STATE_NUMBER=Low16bits(opcode);
       for(output_bits=0; output_bits<CONTROL_STORE_BITS;output_bits++)
       {
          NEXT_LATCHES.MICROINSTRUCTION[output_bits]=Low16bits(CONTROL_STORE[opcode][output_bits]);
       }
   }
}




void cycle_memory() {
   static int cycle_count=0;
   if(GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       cycle_count++;
       if(cycle_count==4)
       {
           NEXT_LATCHES.READY=1;
       }
       else if(cycle_count==5)
       {
           cycle_count=0;
           NEXT_LATCHES.READY=0;
           int data_size=GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION);
           int r_w=GetR_W(CURRENT_LATCHES.MICROINSTRUCTION);
           int mar_0=get_bits(CURRENT_LATCHES.MAR,0,0);
           int write_value=CURRENT_LATCHES.MDR;
           int high_byte=Low16bits(write_value&0xFF00)>>8;
           int low_byte=Low16bits(write_value&0x00FF);

           if(r_w==0)
           {
               if(data_size==0)
               {
                   memory_value=read_byte(CURRENT_LATCHES.MAR)&0x00FF;
               }
               else if(data_size==1)
               {
                   memory_value=read_word(CURRENT_LATCHES.MAR);
               }
           }
           else if(r_w==1)
           {
               if(data_size==0)
               {
                   if(mar_0==0)
                   {
                       write_byte(CURRENT_LATCHES.MAR, low_byte);
                   }
                   else if(mar_0==1)
                   {
                       write_byte(CURRENT_LATCHES.MAR, high_byte);
                   }
               }
               else if(data_size==1)
               {
                   write_word(CURRENT_LATCHES.MAR,write_value);
               }
           }
       }
   }

}



void eval_bus_drivers() {



   if(GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       PC_gate=Low16bits(CURRENT_LATCHES.PC);
   }
   if(GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       int SR1_number=0;
       int SR1_value;
       int SR2_number,SR2_value,SR2_option;
       int SR1_option=GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);
       if(SR1_option==0)
       {
           SR1_number=get_bits(CURRENT_LATCHES.IR,11,9);
       }
       else if (SR1_option==1)
       {
           SR1_number=get_bits(CURRENT_LATCHES.IR,8,6);
       }
       SR1_value=CURRENT_LATCHES.REGS[Low16bits(SR1_number)];

       SR2_option=get_bits(CURRENT_LATCHES.IR,5,5);
       if(SR2_option==0)
       {
           SR2_number=get_bits(CURRENT_LATCHES.IR,2,0);
           SR2_value=CURRENT_LATCHES.REGS[Low16bits(SR2_number)];
       }
       else if(SR2_option==1)
       {
           SR2_value=get_bits(CURRENT_LATCHES.IR,4,0);
           if(get_bits(SR2_value,4,4))
           {
               SR2_value=SEXT(SR2_value,5);
           }
       }

       int ALUK=GetALUK(CURRENT_LATCHES.MICROINSTRUCTION);
       if(ALUK==0)
       {
           ALU_gate=SR1_value+SR2_value;
       }
       else if(ALUK==1)
       {
           ALU_gate=SR1_value&SR2_value;
       }
       else if(ALUK==2)
       {
           ALU_gate=SR1_value^SR2_value;
       }
       else if(ALUK==3)
       {
           ALU_gate=SR1_value;
       }
       ALU_gate=Low16bits(ALU_gate);
   }
   if(GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       int shift_type=get_bits(CURRENT_LATCHES.IR,5,5);
       int shift_direction=get_bits(CURRENT_LATCHES.IR,4,4);
       int shift_amoumt=get_bits(CURRENT_LATCHES.IR,3,0);

       int SR1_number=0;
       int SR1_value;
       int SR2_number,SR2_value,SR2_option;
       int SR1_option=GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);
       if(SR1_option==0)
       {
           SR1_number=get_bits(CURRENT_LATCHES.IR,11,9);
       }
       else if (SR1_option==1)
       {
           SR1_number=get_bits(CURRENT_LATCHES.IR,8,6);
       }
       SR1_value=CURRENT_LATCHES.REGS[Low16bits(SR1_number)];

       if(shift_direction==0)
       {
           SHF_gate=LSHF(SR1_value,shift_amoumt);
       }
       else if(shift_direction==1)
       {
           if(shift_type==0)
           {
               SHF_gate=RSHF(SR1_value,shift_amoumt,0);
           }
           else if(shift_type==1)
           {
               if(get_bits(SR1_value,15,15)==0)
               {
                   SHF_gate=RSHF(SR1_value,shift_amoumt,0);
               }
               else
               {
                   SHF_gate=RSHF(SR1_value,shift_amoumt,1);
               }
           }
       }
       SHF_gate=Low16bits(SHF_gate);
   }

   if(GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       int output1;
       int output2;
       output1=ZEXT(get_bits(CURRENT_LATCHES.IR,7,0),7);
       output1=Low16bits(output1<<1);

       int MARMUX_option=GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION);


       output2=addr_result();

       if(MARMUX_option==0)
       {
           MARMUX_gate=output1;
       }
       else if(MARMUX_option==1)
       {
           MARMUX_gate=output2;
       }
   }

   if(GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       int data_size=GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION);

       if(data_size==0)
       {
           MDR_gate=Low16bits(get_bits(memory_value,7,0));
           if(get_bits(MDR_gate,7,7))
           {
               MDR_gate=SEXT(MDR_gate,7);
           }
       }
       else if(data_size==1)
       {
           MDR_gate=Low16bits(memory_value);
       }
       MDR_gate=Low16bits(MDR_gate);
   }
}

void drive_bus() {

   if(GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION)==1)
    BUS=PC_gate;
   else if(GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION)==1)
    BUS=ALU_gate;
   else if(GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION)==1)
    BUS=MARMUX_gate;
   else if(GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION)==1)
    BUS=MDR_gate;
   else if(GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION)==1)
    BUS=SHF_gate;
   else
    BUS=0;

}




void latch_datapath_values() {

   if(GetLD_BEN(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       NEXT_LATCHES.BEN=((CURRENT_LATCHES.IR>>11)&CURRENT_LATCHES.N)|
                        ((CURRENT_LATCHES.IR>>10)&CURRENT_LATCHES.Z)|
                        ((CURRENT_LATCHES.IR>>9)&CURRENT_LATCHES.P);
   }
   if(GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       setcc(BUS);
   }
   if(GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       NEXT_LATCHES.IR=BUS;
   }
   if(GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       NEXT_LATCHES.MAR=BUS;
   }
   if(GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0)
       {
            NEXT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR,11,9)]=BUS;
       }
       else
       {
           NEXT_LATCHES.REGS[7]=BUS;
       }
   }

   if(GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       if(GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)==1)
       {
           NEXT_LATCHES.MDR=memory_value;
       }
       else
       {
           int data_size=GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION);
           int mar_0=CURRENT_LATCHES.MAR&0x0001;

           if(data_size==1)
           {
               NEXT_LATCHES.MDR=BUS;
           }
           else
           {
               if(mar_0==0)
               {
                   NEXT_LATCHES.MDR=get_bits(BUS,7,0);
               }
               else
               {
                   NEXT_LATCHES.MDR=(get_bits(BUS,7,0))<<8;
               }
           }
       }
   }

   if(GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION)==1)
   {
       if(GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0)
       {
           NEXT_LATCHES.PC=Low16bits(CURRENT_LATCHES.PC+2);
       }
       else if(GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION)==1)
       {
           NEXT_LATCHES.PC=BUS;
       }
       else if(GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION)==2)
       {
           NEXT_LATCHES.PC=addr_result();
       }
   }
}
