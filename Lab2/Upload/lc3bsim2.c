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

//function that return the output value for selected register SR1
int getSR1()
{
    int SR1MUX_output=GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int SR1;

    if(SR1MUX_output==0)
    {
        SR1=Low16bits(CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR & 0x0FFF)>>9]);
    }
    else if(SR1MUX_output==1)
    {
        SR1=Low16bits(CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR & 0X01FF)>>6]);
    }
    return Low16bits(SR1);
}

void eval_micro_sequencer() {

  int condition_code=GetCOND(CURRENT_LATCHES.MICROINSTRUCTION);
  int j_code=GetJ(CURRENT_LATCHES.MICROINSTRUCTION);
  int ird_code=GetIRD(CURRENT_LATCHES.MICROINSTRUCTION);

  int ben=CURRENT_LATCHES.BEN;
  int r=CURRENT_LATCHES.READY;
  int opCode=(CURRENT_LATCHES.IR & 0xF000)>> 12; //get opcode as an int
  int IR11_code=(CURRENT_LATCHES.IR & 0x0800) >> 11; //get the bit 11 of the Instruction
  int condition_code_0=condition_code & 0x0001;      //higher bit of the condition code
  int condition_code_1= (condition_code & 0x0002)>>1; //lower bit of the condition code

  if(ird_code==0)//move to a next state
  {
      int output_bits;
      int ready_bit=(!condition_code_1 & condition_code_0 & r); //ready bit for the and gate at J[1]
      int branch_bit=(condition_code_1 & !condition_code_0 & ben); //branch bit for the and gate at J[2]
      int addr_mode_bit=(condition_code_1 & condition_code_0 & IR11_code); //IR[11] bit for the and gate at J[0]

      int next_state_code=(j_code & 0x0038) | ((((j_code & 0x0004)>>2) | branch_bit)<<2) |
      (((j_code & 0x0002)>>1 | ready_bit)<<1) | ((j_code & 0x0001)|addr_mode_bit); //or all the bits in J together to create the next state
                                                                                   //code
      NEXT_LATCHES.STATE_NUMBER=Low16bits(next_state_code);                        //define the next state code
      for(output_bits=0; output_bits<CONTROL_STORE_BITS;output_bits++)             //for loop copying the bits from the ucode file into latch
      {
          NEXT_LATCHES.MICROINSTRUCTION[output_bits]=Low16bits(CONTROL_STORE[next_state_code][output_bits]);
      }
      NEXT_LATCHES.STATE_NUMBER=Low16bits(next_state_code);
  }
  else//move to the next instruction
  {
      int output_bits;
      for(output_bits=0; output_bits<CONTROL_STORE_BITS;output_bits++)
      {
          NEXT_LATCHES.MICROINSTRUCTION[output_bits]=Low16bits(CONTROL_STORE[opCode][output_bits]); //copying bits indicated into latch
      }
      NEXT_LATCHES.STATE_NUMBER=Low16bits(opCode);
  }

  }


void cycle_memory() {

  /*
   * This function emulates memory and the WE logic.
   * Keep track of which cycle of MEMEN we are dealing with.
   * If fourth, we need to latch Ready bit at the end of
   * cycle to prepare microsequencer for the fifth cycle.
   */
   static int cycle_count=0;
   if(GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION))
   {
       cycle_count++;        //start with cycle count 1
       if(cycle_count==4)
       {
           NEXT_LATCHES.READY=1; //set the ready bit to 1 when the cycle hits the end of 4
       }
       else if(cycle_count==5)  //where the actua reading and wrting starts
       {
           //reset cycle count and the ready bit
           cycle_count=0;
           NEXT_LATCHES.READY=0;

           int read_or_write=GetR_W(CURRENT_LATCHES.MICROINSTRUCTION); //decide wether to read from memory or write to memory
           int dataSize=GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION); //decide either its byte or word to operate
           int mar_we_logic=CURRENT_LATCHES.MAR & 0x0001; // decide the mar_we_logic bits


           if(read_or_write)
           {
               if(dataSize)
               {
                   write_word(CURRENT_LATCHES.MAR, Low16bits(CURRENT_LATCHES.MDR)); //use write word function from lab1, write the value mdr
               }
               else
               {
		           if(mar_we_logic==0)
                   {
                       write_byte(CURRENT_LATCHES.MAR, Low16bits(CURRENT_LATCHES.MDR & 0xFF));//use write byte function from lab1
                   }
                   else
                   {
                       write_byte(CURRENT_LATCHES.MAR, (Low16bits(CURRENT_LATCHES.MDR & 0xFF00))>>8);//use write byte function from lab1
                   }
               }
           }
           else
           {
               if(dataSize)
               {
                   memory_value=Low16bits((read_word(CURRENT_LATCHES.MAR))&0xFFFF); //user read word function from lab1
               }
               else
               {
                   memory_value=Low16bits((read_byte(CURRENT_LATCHES.MAR))&0X00FF);
               }
           }
       }
   }
}



int PC_result, MARMUX_result, SHF_result, MDR_result,ALU_result;

void eval_bus_drivers() {

  /*
   * Datapath routine emulating operations before driving the bus.
   * Evaluate the input of tristate drivers
   *             Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */

   int SR1_out;
   int SR2_out;

   int gatePC=GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION); //get pc gate signal
   int gateMARMUX=GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION); //get MARMUX gate signal
   int gateMDR= GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION); //get MDR gate signal
   int gateSHF= GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION); //get SHF gate signal
   int gateALU= GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION); //get ALU gate signal

   if(gatePC)
   {
       PC_result=Low16bits(CURRENT_LATCHES.PC);                //load the PC value to BUS
   }
   if(gateMARMUX)
   {
       int MARMUX_option=GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION); //see if the MARMUX will load either from the ZEXT or the adder
       int adder1=GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);      //get adder1 value
       int adder2=GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);      //get adder2 value
       int get_LSHF1=GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION);      //get the shift amount

       int SR1_OUT, SR2_OUT;



       if(MARMUX_option==0) //load IR
       {
           MARMUX_result=CURRENT_LATCHES.IR&0x00FF; //get the lower 7 bits of IR
           if(MARMUX_result & 0x0080)
           {
               MARMUX_result=ZEXT(MARMUX_result, 7); //extend the result to 16 bits
           }
           MARMUX_result=Low16bits(MARMUX_result<<1);//shift left 1 bit
       }
       else if(MARMUX_option==1) //load from the adder
       {
           int adder1_result,adder2_result;
           if(adder1==0)
           {
               //choose for the PC as the adder1
               adder1_result=CURRENT_LATCHES.PC;
           }
           else if(adder1==1)
           {
                //choose SR1 as the adder1
               adder1_result=CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR & 0X01C0)>>6];
           }

           if(adder2==0)// add in 0 as the last bit
           {
               adder2_result=0;
           }
           else if(adder2==1)
           {
               //find the shift 6 bits from [5:0]
               adder2_result=SEXT(CURRENT_LATCHES.IR, 5);
           }
           else if(adder2==2)
           {
               //find the shift 6 bits from [8:0]
               adder2_result=SEXT(CURRENT_LATCHES.IR, 9);
           }
           else if(adder2==3)
           {
               //find the shift 6 bits from [11:0]
               adder2_result=SEXT(CURRENT_LATCHES.IR, 11);
           }
           adder1_result=Low16bits(adder1_result);
           adder2_result=Low16bits(adder2_result);

           if(get_LSHF1==1)
           {
                //shift adder2 by 1 left if the lshf1 is 1
               adder2_result=Low16bits(adder2_result<<1);
           }
           MARMUX_result=Low16bits(adder2_result+adder1_result); // add the two 16 bits result as the output
       }
   }
   if (gateMDR)
   {
       int word_size=GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION);
       if(word_size==0)
       {
           MDR_result=Low16bits(memory_value&0x00FF);
           if(MDR_result & 0x0080)
           {
               MDR_result=Low16bits(SEXT(MDR_result,7));
           }
       }
       else
       {
           MDR_result=Low16bits(memory_value);
       }
       MDR_result=Low16bits(MDR_result);
   }
   if(gateSHF)
   {
       int shift_type=(CURRENT_LATCHES.IR&0x003F)>>4; //take IR[5:4] as the shift type identifier
       int shift_amount=(CURRENT_LATCHES.IR&0x000F); //take IR[4:0] as the shift amount identifier
       int SR1=getSR1();                             //figure out which register is SR1
       if(shift_type==0)
       {
           SHF_result=Low16bits(SR1<<shift_amount); //do left shift
       }
       else if(shift_type==1)
       {
           SHF_result=RSHF(SR1, shift_amount, 0 ); //do right shift
       }
       else if(shift_type==3)
       {
           if(SR1 & 0x8000!=0x8000) //if the top bit of SR1 is not 1, do logical shift, else do arthimict shift
           {
               SHF_result=RSHF(SR1, shift_amount, 0 );
           }
           else
           {
               SHF_result=RSHF(SR1, shift_amount, 1 );
           }
       }
       SHF_result=Low16bits(SHF_result);
   }
   if(gateALU)
   {
       int ALUK=GetALUK(CURRENT_LATCHES.MICROINSTRUCTION); //get the ALUK value from the control
       int SR2MUX=(CURRENT_LATCHES.IR & 0x0020)>>5;        //get SR2MUX which is IR[6]
       int SR1=getSR1();
       int SR2_output;

       if(SR2MUX==0) //if SR2MUX is 0, load in IR[2:0] as indicating the register value
       {
           SR2_output=Low16bits(CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR & 0x0007)]);
       }
       else if(SR2MUX==1)
       {
           SR2_output=Low16bits(CURRENT_LATCHES.IR & 0x001F); //take IR[4:0]
           if(SR2_output & 0X0010)
           {
               SR2_output = Low16bits(SEXT(SR2_output, 5));
           }
       }

       if(ALUK==0) //perform add operation
       {
           ALU_result=SR1+SR2_output;
       }
       if(ALUK==1) //perform and operation
       {
           ALU_result=SR1&SR2_output;
       }
       if(ALUK==2) //perform xor operation
       {
           ALU_result=SR1^SR2_output;
       }
       if(ALUK==3) //directly load SR1
       {
           ALU_result=SR1;
       }
       ALU_result=Low16bits(ALU_result);
   }

}


void drive_bus() {

  /*
   * Datapath routine for driving the bus from one of the 5 possible
   * tristate drivers.
   */
   int gatePC=GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION);
   int gateMARMUX=GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION);
   int gateMDR= GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION);
   int gateSHF= GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION);
   int gateALU= GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION);
   if(gatePC)
   {
       BUS=PC_result;
   }
   else if(gateMARMUX)
   {
       BUS=MARMUX_result;
   }
   else if(gateMDR)
   {
       BUS=MDR_result;
   }
   else if(gateSHF)
   {
       BUS=SHF_result;
   }
   else if(gateALU)
   {
       BUS=ALU_result;
   }
   else
   {
       BUS=0; //default situation, there is no information on the bus
   }

}


void latch_datapath_values() {

  /*
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the bus; therefore, this routine has to come
   * after drive_bus.
   */

    int BEN_LD    = GetLD_BEN(CURRENT_LATCHES.MICROINSTRUCTION);
    int CC_LD     = GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION);
    int IR_LD     = GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION);
    int MAR_LD    = GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION);
    int MDR_LD    = GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION);
    int PC_LD     = GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION);
    int REG_LD    = GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION);
    int MIO_EN=GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION);
    int word_size=GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION);
    int MAR_byte=CURRENT_LATCHES.MAR&0x0001;
    int PC_MUX=GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int DR_MUX=GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION);


    if(BEN_LD)
    {
        int N_value=(CURRENT_LATCHES.IR>>11)&CURRENT_LATCHES.N;
        int Z_value=(CURRENT_LATCHES.IR>>10)&CURRENT_LATCHES.Z;
        int P_value=(CURRENT_LATCHES.IR>>9)&CURRENT_LATCHES.P;

        NEXT_LATCHES.BEN=(N_value|Z_value|P_value);
    }

    int first_bit=(BUS&0x8000)>>15; //set cc value
    if((CC_LD)&&(first_bit))
    {
        NEXT_LATCHES.N=1;
        NEXT_LATCHES.Z=0;
        NEXT_LATCHES.P=0;
    }
    else if((CC_LD)&&(BUS==0))
    {
        NEXT_LATCHES.Z=1;
        NEXT_LATCHES.P=0;
        NEXT_LATCHES.N=0;
    }
    else if(CC_LD)
    {
        NEXT_LATCHES.P=1;
        NEXT_LATCHES.N=0;
        NEXT_LATCHES.Z=0;
    }

    if(IR_LD)
    {
        NEXT_LATCHES.IR=Low16bits(BUS); //load IR from bus
    }

    if(MAR_LD) {
        NEXT_LATCHES.MAR = Low16bits(BUS); //load MAR from bus
    }
    if((MDR_LD)&&(MIO_EN==0)&&(word_size==0)&&(MAR_byte==0)) //load MDR consider the WE logic value and the MAR[0]
    {
        NEXT_LATCHES.MDR=Low16bits(BUS&0x00FF);
    }
    else if((MDR_LD)&&(MIO_EN==0)&&(word_size==1))
    {
        NEXT_LATCHES.MDR=Low16bits(BUS);
    }
    else if((MDR_LD)&&(MIO_EN==0)&&(word_size==0)&&(MAR_byte==1))
    {
        NEXT_LATCHES.MDR=Low16bits(BUS&0x00FF)<<8;
    }
    else if((MDR_LD)&&(MIO_EN))
    {
        NEXT_LATCHES.MDR=Low16bits(memory_value);
    }


    if((PC_LD)&&(PC_MUX==0))
    {
        NEXT_LATCHES.PC=Low16bits(CURRENT_LATCHES.PC+2); //load default PC value which is PC+2
    }
    else if((PC_LD)&&(PC_MUX==1))
    {
        NEXT_LATCHES.PC=Low16bits(BUS); //load PC value from BUS
    }
    else if((PC_LD)&&(PC_MUX==2))    //load PC value from the Adder
    {
        int ADDR1MUX_value=GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION); //Choose which value to load into PC depends on the ADDR1MUX
        int adder1_value=0;
        if(ADDR1MUX_value==0)
        {
            adder1_value=CURRENT_LATCHES.PC;
        }
        else if(adder1_value==1)
        {
            adder1_value=CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR & 0x01C0)>>6];
        }
        adder1_value=Low16bits(adder1_value);

        int ADDr2MUX_value=GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
        int adder2_value=0;
        if(ADDr2MUX_value==0)
        {
            adder2_value=0;
        }
        else if(ADDr2MUX_value==1)
        {
            adder2_value=CURRENT_LATCHES.IR & 0X003f;
            if(adder2_value & 0x0020)
            {
                adder2_value=SEXT(CURRENT_LATCHES.IR, 5);
            }
        }
        else if(ADDr2MUX_value==2)
        {
            adder2_value=CURRENT_LATCHES.IR & 0X01FF;
            if(adder2_value & 0x0100)
            {
                adder2_value=SEXT(CURRENT_LATCHES.IR, 8);
            }
        }
        else if(ADDr2MUX_value==3)
        {
            adder2_value=CURRENT_LATCHES.IR & 0X07FF;
            if(adder2_value & 0x0400)
            {
                adder2_value=SEXT(CURRENT_LATCHES.IR, 10);
            }
        }
        adder2_value=Low16bits(adder2_value);


        int LSHF1_value=adder2_value;
        int LSHF1_MUX=GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION);
        if(LSHF1_MUX)
        {
            LSHF1_value=Low16bits(adder2_value<<1);
        }
        NEXT_LATCHES.PC=Low16bits(LSHF1_value+adder1_value);
    }

    if(REG_LD)
    {
        if(DR_MUX==0)
        {
            NEXT_LATCHES.REGS[((CURRENT_LATCHES.IR & 0x0FFF)>>9)]=Low16bits(BUS);
        }
        else if(DR_MUX==1)
        {
            NEXT_LATCHES.REGS[7]=Low16bits(BUS);
        }
    }



    if((PC_LD)&&(PC_MUX==0))
    {
        NEXT_LATCHES.PC=Low16bits(CURRENT_LATCHES.PC+2);
    }
    else if((PC_LD)&&(PC_MUX==1))
    {
        NEXT_LATCHES.PC=Low16bits(BUS);
    }
    else if((PC_LD)&&(PC_MUX==2))
    {
        int ADDR1MUX_value=GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);
        int adder1_value;
        if(ADDR1MUX_value==0)
        {
            adder1_value=CURRENT_LATCHES.PC;
        }
        else if(ADDR1MUX_value==1)
        {
            adder1_value=CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR & 0x01C0)>>6];
        }
        adder1_value=Low16bits(adder1_value);

        int ADDr2MUX_value=GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
        int adder2_value;
        if(ADDr2MUX_value==0)
        {
            adder2_value=0;
        }
        else if(ADDr2MUX_value==1)
        {
            adder2_value=CURRENT_LATCHES.IR & 0X003f;
            if(adder2_value & 0x0020)
            {
                adder2_value=SEXT(CURRENT_LATCHES.IR, 5);
            }
        }
        else if(ADDr2MUX_value==2)
        {
            adder2_value=CURRENT_LATCHES.IR & 0X01FF;
            if(adder2_value & 0x0100)
            {
                adder2_value=SEXT(CURRENT_LATCHES.IR, 8);
            }
        }
        else if(ADDr2MUX_value==3)
        {
            adder2_value=CURRENT_LATCHES.IR & 0X07FF;
            if(adder2_value & 0x0400)
            {
                adder2_value=SEXT(CURRENT_LATCHES.IR, 10);
            }
        }
        adder2_value=Low16bits(adder2_value);


        int LSHF1_value=adder2_value;
        int LSHF1_MUX=GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION);
        if(LSHF1_MUX)
        {
            LSHF1_value=Low16bits(adder2_value<<1);
        }
        NEXT_LATCHES.PC=Low16bits(LSHF1_value+adder1_value);
    }
}
