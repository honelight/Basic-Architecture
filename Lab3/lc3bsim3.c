
/***************************************************************/
/*                                                             */
/*   LC-3b Simulator - Lab 3                                   */
/*                                                             */
/*   ECE 3056A                                                 */
/*   This is a modified version of the LC-3b Pipeline Design   */
/*   from Prof. Yale Patt's course at UT Austin                */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
void FETCH_stage();
void DE_stage();
void AGEX_stage();
void MEM_stage();
void SR_stage();
/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define TRUE  1
#define FALSE 0

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
/* control signals from the control store */
enum CS_BITS {
  SR1_NEEDED,
  SR2_NEEDED,
  DRMUX,

  ADDR1MUX,
  ADDR2MUX1, ADDR2MUX0,
  LSHF1,
  ADDRESSMUX,
  SR2MUX,
  ALUK1, ALUK0,
  ALU_RESULTMUX,

  BR_OP,
  UNCOND_OP,
  TRAP_OP,
  BR_STALL,

  DCACHE_EN,
  DCACHE_RW,
  DATA_SIZE,

  DR_VALUEMUX1, DR_VALUEMUX0,
  LD_REG,
  LD_CC,
  NUM_CONTROL_STORE_BITS

} CS_BITS;


enum AGEX_CS_BITS {

  AGEX_ADDR1MUX,
  AGEX_ADDR2MUX1, AGEX_ADDR2MUX0,
  AGEX_LSHF1,
  AGEX_ADDRESSMUX,
  AGEX_SR2MUX,
  AGEX_ALUK1, AGEX_ALUK0,
  AGEX_ALU_RESULTMUX,

  AGEX_BR_OP,
  AGEX_UNCOND_OP,
  AGEX_TRAP_OP,
  AGEX_BR_STALL,
  AGEX_DCACHE_EN,
  AGEX_DCACHE_RW,
  AGEX_DATA_SIZE,

  AGEX_DR_VALUEMUX1, AGEX_DR_VALUEMUX0,
  AGEX_LD_REG,
  AGEX_LD_CC,
  NUM_AGEX_CS_BITS
} AGEX_CS_BITS;

enum MEM_CS_BITS {
  MEM_BR_OP,
  MEM_UNCOND_OP,
  MEM_TRAP_OP,
  MEM_BR_STALL,
  MEM_DCACHE_EN,
  MEM_DCACHE_RW,
  MEM_DATA_SIZE,

  MEM_DR_VALUEMUX1, MEM_DR_VALUEMUX0,
  MEM_LD_REG,
  MEM_LD_CC,
  NUM_MEM_CS_BITS
} MEM_CS_BITS;

enum SR_CS_BITS {
  SR_DR_VALUEMUX1, SR_DR_VALUEMUX0,
  SR_LD_REG,
  SR_LD_CC,
  NUM_SR_CS_BITS
} SR_CS_BITS;


/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int Get_SR1_NEEDED(int *x)     { return (x[SR1_NEEDED]); }
int Get_SR2_NEEDED(int *x)     { return (x[SR2_NEEDED]); }
int Get_DRMUX(int *x)          { return (x[DRMUX]);}
int Get_DE_BR_OP(int *x)       { return (x[BR_OP]); }
int Get_ADDR1MUX(int *x)       { return (x[AGEX_ADDR1MUX]); }
int Get_ADDR2MUX(int *x)       { return ((x[AGEX_ADDR2MUX1] << 1) + x[AGEX_ADDR2MUX0]); }
int Get_LSHF1(int *x)          { return (x[AGEX_LSHF1]); }
int Get_ADDRESSMUX(int *x)     { return (x[AGEX_ADDRESSMUX]); }
int Get_SR2MUX(int *x)          { return (x[AGEX_SR2MUX]); }
int Get_ALUK(int *x)           { return ((x[AGEX_ALUK1] << 1) + x[AGEX_ALUK0]); }
int Get_ALU_RESULTMUX(int *x)  { return (x[AGEX_ALU_RESULTMUX]); }
int Get_BR_OP(int *x)          { return (x[MEM_BR_OP]); }
int Get_UNCOND_OP(int *x)      { return (x[MEM_UNCOND_OP]); }
int Get_TRAP_OP(int *x)        { return (x[MEM_TRAP_OP]); }
int Get_DCACHE_EN(int *x)      { return (x[MEM_DCACHE_EN]); }
int Get_DCACHE_RW(int *x)      { return (x[MEM_DCACHE_RW]); }
int Get_DATA_SIZE(int *x)      { return (x[MEM_DATA_SIZE]); }
int Get_DR_VALUEMUX1(int *x)   { return ((x[SR_DR_VALUEMUX1] << 1 ) + x[SR_DR_VALUEMUX0]); }
int Get_AGEX_LD_REG(int *x)    { return (x[AGEX_LD_REG]); }
int Get_AGEX_LD_CC(int *x)     { return (x[AGEX_LD_CC]); }
int Get_MEM_LD_REG(int *x)     { return (x[MEM_LD_REG]); }
int Get_MEM_LD_CC(int *x)      { return (x[MEM_LD_CC]); }
int Get_SR_LD_REG(int *x)      { return (x[SR_LD_REG]); }
int Get_SR_LD_CC(int *x)       { return (x[SR_LD_CC]); }
int Get_DE_BR_STALL(int *x)    { return (x[BR_STALL]); }
int Get_AGEX_BR_STALL(int *x)  { return (x[AGEX_BR_STALL]); }
int Get_MEM_BR_STALL(int *x)   { return (x[MEM_BR_STALL]); }



/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][NUM_CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A
   There are two write enable signals, one for each byte. WE0 is used for
   the least significant byte of a word. WE1 is used for the most significant
   byte of a word. */

#define WORDS_IN_MEM    0x08000
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/
/* The LC-3b register file.                                      */
/***************************************************************/
#define LC3b_REGS 8
int REGS[LC3b_REGS];
/***************************************************************/
/* architectural state */
/***************************************************************/
int  PC,  	/* program counter */
     N,		/* n condition bit */
     Z = 1,	/* z condition bit */
     P;		/* p condition bit */
/***************************************************************/
/* LC-3b State info.                                             */
/***************************************************************/

typedef struct PipeState_Entry_Struct{

  /* DE latches */
int DE_NPC,
    DE_IR,
    DE_V,
    /* AGEX lateches */
    AGEX_NPC,
    AGEX_SR1,
    AGEX_SR2,
    AGEX_CC,
    AGEX_IR,
    AGEX_DRID,
    AGEX_V,
    AGEX_CS[NUM_AGEX_CS_BITS],
    /* MEM latches */
    MEM_NPC,
    MEM_ALU_RESULT,
    MEM_ADDRESS,
    MEM_CC,
    MEM_IR,
    MEM_DRID,
    MEM_V,
    MEM_CS[NUM_MEM_CS_BITS],
    /* SR latches */
    SR_NPC,
    SR_DATA,
    SR_ALU_RESULT,
    SR_ADDRESS,
    SR_IR,
    SR_DRID,
    SR_V,
    SR_CS[NUM_SR_CS_BITS];

} PipeState_Entry;

/* data structure for latch */
PipeState_Entry PS, NEW_PS;

/* simulator signal */
int RUN_BIT;

/* Internal stall signals */
int   dep_stall,
      v_de_br_stall,
      v_agex_br_stall,
      v_mem_br_stall,
      mem_stall,
      icache_r;

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
    printf("rdump            -  dump the architectural state    \n");
    printf("idump            -  dump the internal state         \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

void print_CS(int *CS, int num)
{
  int ii ;
  for ( ii = 0 ; ii < num; ii++) {
    printf("%d",CS[ii]);
  }
  printf("\n");
}
/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {
  NEW_PS = PS;
  SR_stage();
  MEM_stage();
  AGEX_stage();
  DE_stage();
  FETCH_stage();
  PS = NEW_PS;
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
	if (PC == 0x0000) {
	    cycle();
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
    if ((RUN_BIT == FALSE) || (PC == 0x0000)) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }
    printf("Simulating...\n\n");
    /* initialization */
    while (PC != 0x0000)
      cycle();
    cycle();
      RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a region of memory to the output file.     */
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
/* Purpose   : Dump current architectural state  to the       */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {
    int k;

    printf("\nCurrent architectural state :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count : %d\n", CYCLE_COUNT);
    printf("PC          : 0x%04x\n", PC);
    printf("CCs: N = %d  Z = %d  P = %d\n", N, Z, P);
    printf("Registers:\n");
    for (k = 0; k < LC3b_REGS; k++)
	printf("%d: 0x%04x\n", k, (REGS[k] & 0xFFFF));
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent architectural state :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC          : 0x%04x\n", PC);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", N, Z, P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%04x\n", k, (REGS[k] & 0xFFFF));
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : idump                                           */
/*                                                             */
/* Purpose   : Dump current internal state to the              */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void idump(FILE * dumpsim_file) {
    int k;

    printf("\nCurrent architectural state :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count     : %d\n", CYCLE_COUNT);
    printf("PC              : 0x%04x\n", PC);
    printf("CCs: N = %d  Z = %d  P = %d\n", N, Z, P);
    printf("Registers:\n");
    for (k = 0; k < LC3b_REGS; k++)
	printf("%d: 0x%04x\n", k, (REGS[k] & 0xFFFF));
    printf("\n");

    printf("------------- Stall Signals -------------\n");
    printf("ICACHE_R        :  %d\n", icache_r);
    printf("DEP_STALL       :  %d\n", dep_stall);
    printf("V_DE_BR_STALL   :  %d\n", v_de_br_stall);
    printf("V_AGEX_BR_STALL :  %d\n", v_agex_br_stall);
    printf("MEM_STALL       :  %d\n", mem_stall);
    printf("V_MEM_BR_STALL  :  %d\n", v_mem_br_stall);
    printf("\n");

    printf("------------- DE   Latches --------------\n");
    printf("DE_NPC          :  0x%04x\n", PS.DE_NPC );
    printf("DE_IR           :  0x%04x\n", PS.DE_IR );
    printf("DE_V            :  %d\n", PS.DE_V);
    printf("\n");

    printf("------------- AGEX Latches --------------\n");
    printf("AGEX_NPC        :  0x%04x\n", PS.AGEX_NPC );
    printf("AGEX_SR1        :  0x%04x\n", PS.AGEX_SR1 );
    printf("AGEX_SR2        :  0x%04x\n", PS.AGEX_SR2 );
    printf("AGEX_CC         :  %d\n", PS.AGEX_CC );
    printf("AGEX_IR         :  0x%04x\n", PS.AGEX_IR );
    printf("AGEX_DRID       :  %d\n", PS.AGEX_DRID);
    printf("AGEX_CS         :  ");
    for ( k = 0 ; k < NUM_AGEX_CS_BITS; k++) {
      printf("%d",PS.AGEX_CS[k]);
    }
    printf("\n");
    printf("AGEX_V          :  %d\n", PS.AGEX_V);
    printf("\n");

    printf("------------- MEM  Latches --------------\n");
    printf("MEM_NPC         :  0x%04x\n", PS.MEM_NPC );
    printf("MEM_ALU_RESULT  :  0x%04x\n", PS.MEM_ALU_RESULT );
    printf("MEM_ADDRESS     :  0x%04x\n", PS.MEM_ADDRESS );
    printf("MEM_CC          :  %d\n", PS.MEM_CC );
    printf("MEM_IR          :  0x%04x\n", PS.MEM_IR );
    printf("MEM_DRID        :  %d\n", PS.MEM_DRID);
    printf("MEM_CS          :  ");
    for ( k = 0 ; k < NUM_MEM_CS_BITS; k++) {
      printf("%d",PS.MEM_CS[k]);
    }
    printf("\n");
    printf("MEM_V           :  %d\n", PS.MEM_V);
    printf("\n");

    printf("------------- SR   Latches --------------\n");
    printf("SR_NPC          :  0x%04x\n", PS.SR_NPC );
    printf("SR_DATA         :  0x%04x\n", PS.SR_DATA );
    printf("SR_ALU_RESULT   :  0x%04x\n", PS.SR_ALU_RESULT );
    printf("SR_ADDRESS      :  0x%04x\n", PS.SR_ADDRESS );
    printf("SR_IR           :  0x%04x\n", PS.SR_IR );
    printf("SR_DRID         :  %d\n", PS.SR_DRID);
    printf("SR_CS           :  ");
    for ( k = 0 ; k < NUM_SR_CS_BITS; k++) {
      printf("%d",PS.SR_CS[k]);
    }
    printf("\n");
    printf("SR_V            :  %d\n", PS.SR_V);

    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file,"\nCurrent architectural state :\n");
    fprintf(dumpsim_file,"-------------------------------------\n");
    fprintf(dumpsim_file,"Cycle Count     : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file,"PC              : 0x%04x\n", PC);
    fprintf(dumpsim_file,"CCs: N = %d  Z = %d  P = %d\n", N, Z, P);
    fprintf(dumpsim_file,"Registers:\n");
    for (k = 0; k < LC3b_REGS; k++)
      fprintf(dumpsim_file,"%d: 0x%04x\n", k, (REGS[k] & 0xFFFF));
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- Stall Signals -------------\n");
    fprintf(dumpsim_file,"ICACHE_R        :  %d\n", icache_r);
    fprintf(dumpsim_file,"DEP_STALL       :  %d\n", dep_stall);
    fprintf(dumpsim_file,"V_DE_BR_STALL   :  %d\n", v_de_br_stall);
    fprintf(dumpsim_file,"V_AGEX_BR_STALL :  %d\n", v_agex_br_stall);
    fprintf(dumpsim_file,"MEM_STALL       :  %d\n", mem_stall);
    fprintf(dumpsim_file,"V_MEM_BR_STALL  :  %d\n", v_mem_br_stall);
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- DE   Latches --------------\n");
    fprintf(dumpsim_file,"DE_NPC          :  0x%04x\n", PS.DE_NPC );
    fprintf(dumpsim_file,"DE_IR           :  0x%04x\n", PS.DE_IR );
    fprintf(dumpsim_file,"DE_V            :  %d\n", PS.DE_V);
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- AGEX Latches --------------\n");
    fprintf(dumpsim_file,"AGEX_NPC        :  0x%04x\n", PS.AGEX_NPC );
    fprintf(dumpsim_file,"AGEX_SR1        :  0x%04x\n", PS.AGEX_SR1 );
    fprintf(dumpsim_file,"AGEX_SR2        :  0x%04x\n", PS.AGEX_SR2 );
    fprintf(dumpsim_file,"AGEX_CC         :  %d\n", PS.AGEX_CC );
    fprintf(dumpsim_file,"AGEX_IR         :  0x%04x\n", PS.AGEX_IR );
    fprintf(dumpsim_file,"AGEX_DRID       :  %d\n", PS.AGEX_DRID);
    fprintf(dumpsim_file,"AGEX_CS         :  ");
    for ( k = 0 ; k < NUM_AGEX_CS_BITS; k++) {
      fprintf(dumpsim_file,"%d",PS.AGEX_CS[k]);
    }
    fprintf(dumpsim_file,"\n");
    fprintf(dumpsim_file,"AGEX_V          :  %d\n", PS.AGEX_V);
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- MEM  Latches --------------\n");
    fprintf(dumpsim_file,"MEM_NPC         :  0x%04x\n", PS.MEM_NPC );
    fprintf(dumpsim_file,"MEM_ALU_RESULT  :  0x%04x\n", PS.MEM_ALU_RESULT );
    fprintf(dumpsim_file,"MEM_ADDRESS     :  0x%04x\n", PS.MEM_ADDRESS );
    fprintf(dumpsim_file,"MEM_CC          :  %d\n", PS.MEM_CC );
    fprintf(dumpsim_file,"MEM_IR          :  0x%04x\n", PS.MEM_IR );
    fprintf(dumpsim_file,"MEM_DRID        :  %d\n", PS.MEM_DRID);
    fprintf(dumpsim_file,"MEM_CS          :  ");
    for ( k = 0 ; k < NUM_MEM_CS_BITS; k++) {
      fprintf(dumpsim_file,"%d",PS.MEM_CS[k]);
    }
    fprintf(dumpsim_file,"\n");
    fprintf(dumpsim_file,"MEM_V           :  %d\n", PS.MEM_V);
    fprintf(dumpsim_file,"\n");

    fprintf(dumpsim_file,"------------- SR   Latches --------------\n");
    fprintf(dumpsim_file,"SR_NPC          :  0x%04x\n", PS.SR_NPC );
    fprintf(dumpsim_file,"SR_DATA         :  0x%04x\n",PS.SR_DATA );
    fprintf(dumpsim_file,"SR_ALU_RESULT   :  0x%04x\n", PS.SR_ALU_RESULT );
    fprintf(dumpsim_file,"SR_ADDRESS      :  0x%04x\n", PS.SR_ADDRESS );
    fprintf(dumpsim_file,"SR_IR           :  0x%04x\n", PS.SR_IR );
    fprintf(dumpsim_file,"SR_DRID         :  %d\n", PS.SR_DRID);
    fprintf(dumpsim_file,"SR_CS           :  ");
    for ( k = 0 ; k < NUM_SR_CS_BITS; k++) {
    fprintf(dumpsim_file, "%d",PS.SR_CS[k]);
    }
    fprintf(dumpsim_file,"\n");
    fprintf(dumpsim_file,"SR_V            :  %d\n", PS.SR_V);

    fprintf(dumpsim_file,"\n");


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

    case 'I':
    case 'i':
        idump(dumpsim_file);
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

	for (j = 0; j < NUM_CONTROL_STORE_BITS; j++) {
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

/***************************************************************/
/*                                                             */
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Zero out the memory array                       */
/*                                                             */
/***************************************************************/
void init_memory() {
    int i;

     for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}


/***************************************************************/
/*                                                             */
/* Procedure : init_state                                      */
/*                                                             */
/* Purpose   : Zero out all latches and registers              */
/*                                                             */
/***************************************************************/
void init_state() {

  memset(&PS, 0 ,sizeof(PipeState_Entry));
  memset(&NEW_PS, 0 , sizeof(PipeState_Entry));

  dep_stall       = 0;
  v_de_br_stall   = 0;
  v_agex_br_stall = 0;
  v_mem_br_stall  = 0;
  mem_stall       = 0;
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
	program_base = word >> 1 ;
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

    if (PC == 0) PC  = program_base << 1 ;
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
    init_state();

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* dcache_access                                               */
/*                                                             */
/***************************************************************/
void dcache_access(int dcache_addr, int *read_word, int write_word, int *dcache_r,
		    int mem_w0, int mem_w1) {

  int addr = dcache_addr >> 1 ;
  int random = CYCLE_COUNT % 9;

  if (!random) {
    *dcache_r = 0;
    *read_word = 0xfeed ;
  }
  else {
    *dcache_r = 1;

    *read_word = (MEMORY[addr][1] << 8) | (MEMORY[addr][0] & 0x00FF);
    if(mem_w0) MEMORY[addr][0] = write_word & 0x00FF;
    if(mem_w1) MEMORY[addr][1] = (write_word & 0xFF00) >> 8;
  }
}
/***************************************************************/
/*                                                             */
/* icache_access                                               */
/*                                                             */
/***************************************************************/
void icache_access(int icache_addr, int *read_word, int *icache_r) {

  int addr = icache_addr >> 1 ;
  int random = CYCLE_COUNT % 13;

  if (!random) {
    *icache_r = 0;
    *read_word = 0xfeed;
  }
  else {
    *icache_r = 1;
    *read_word = MEMORY[addr][1] << 8 | MEMORY[addr][0];
  }
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
/* Do not modify the above code.
   You are allowed to use the following global variables in your
   code. These are defined above.

   RUN_BIT
   REGS
   MEMORY

   PC
   N
   Z
   P

   dep_stall
   v_de_br_stall
   v_agex_br_stall
   v_mem_br_stall
   mem_stall
   icache_r

   PS
   NEW_PS


   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.


   Begin your code here 	  			       */
/***************************************************************/
#define COPY_AGEX_CS_START 3
#define COPY_MEM_CS_START 9
#define COPY_SR_CS_START  7

/* Signals generated by SR stage and needed by previous stages in the
   pipeline are declared below. */
int sr_reg_data,
    sr_n, sr_z, sr_p,
    v_sr_ld_cc,
    v_sr_ld_reg,
    sr_reg_id;


/************************* SR_stage() *************************/
void SR_stage() {

  /* You are given the code for SR_stage to get you started. Look at
     the figure for SR stage to see how this code is implemented. */

  switch (Get_DR_VALUEMUX1(PS.SR_CS))
  {
  case 0:
    sr_reg_data = PS.SR_ADDRESS ;
    break;
  case 1:
    sr_reg_data = PS.SR_DATA ;
    break;
  case 2:
    sr_reg_data = PS.SR_NPC ;
    break;
  case 3:
    sr_reg_data = PS.SR_ALU_RESULT ;
    break;
  }

  sr_reg_id = PS.SR_DRID;
  v_sr_ld_reg = Get_SR_LD_REG(PS.SR_CS) & PS.SR_V;
  v_sr_ld_cc = Get_SR_LD_CC(PS.SR_CS) & PS.SR_V ;

  /* CC LOGIC  */
  sr_n = ((sr_reg_data & 0x8000) ? 1 : 0);
  sr_z = ((sr_reg_data & 0xFFFF) ? 0 : 1);
  sr_p = 0;
  if ((!sr_n) && (!sr_z))
    sr_p = 1;

}


/************************* MEM_stage() *************************/
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

int get_bits(int value, int start, int end){
  int result;
  assert(start >= end );
  result = value >> end;
  result = result % ( 1 << ( start - end + 1 ) );
  return result;
}


int v_mem_cc, v_mem_ld_reg, mem_drid, mem_pcMux, trap, target_pc;
int LD_DE,LD_PC,LD_AGEX;
void MEM_stage() {

  int ii,jj = 0;

  /* your code for MEM stage goes here */
  /* implement the constant*/
  int DCACHE_EN, DCACHE_RW, WORD_OR_BYTE, DCACHE_ADDER, HIGH_BYTE;
  DCACHE_EN= Get_DCACHE_EN(PS.MEM_CS) & PS.MEM_V;
  DCACHE_RW=Get_DCACHE_RW(PS.MEM_CS);
  WORD_OR_BYTE=Get_DATA_SIZE(PS.MEM_CS);
  DCACHE_ADDER=PS.MEM_ADDRESS;
  HIGH_BYTE=PS.MEM_ADDRESS&0x1;

  //define we logic
  int we0=DCACHE_RW & (!HIGH_BYTE);
  int we1=DCACHE_RW & (WORD_OR_BYTE ^ HIGH_BYTE);

  //write the data to memory, also decide if write word or byte, if byte, high or low byte
  int write_in_data=0;
  if(WORD_OR_BYTE==1)
  {
      write_in_data=Low16bits(PS.MEM_ALU_RESULT);
  }
  else if(WORD_OR_BYTE==1 && HIGH_BYTE==0)
  {
      write_in_data=Low16bits(Low16bits(PS.MEM_ALU_RESULT)&0x00FF);
  }
  else if(WORD_OR_BYTE==1 && HIGH_BYTE==1)
  {
      write_in_data=Low16bits((Low16bits(PS.MEM_ALU_RESULT)&0x00FF)<<8);
  }

  int read_in_data=0;
  int dcache_assert;
  if(DCACHE_EN)
  {
      dcache_access(DCACHE_ADDER,&read_in_data,write_in_data,&dcache_assert,we0,we1);
  }

  //exame the read in data values

  if(WORD_OR_BYTE==0 && HIGH_BYTE==1)
  {
      read_in_data=Low16bits((read_in_data>>8) & 0x00FF);
  }
  else if (WORD_OR_BYTE==0 && HIGH_BYTE==0)
  {
      read_in_data=Low16bits(read_in_data & 0x00FF);
  }

  mem_stall=(!dcache_assert) & DCACHE_EN;

  v_mem_ld_reg= PS.MEM_V & Get_MEM_LD_REG(PS.MEM_CS);
  v_mem_cc=PS.MEM_V & Get_MEM_LD_CC(PS.MEM_CS);
  v_mem_br_stall=PS.MEM_V & Get_MEM_BR_STALL(PS.MEM_CS);
  mem_drid=PS.MEM_DRID;
  trap=read_in_data;
  target_pc=PS.MEM_ADDRESS;

  if(PS.MEM_V==1 && Get_UNCOND_OP(PS.MEM_CS==1))
  {
      mem_pcMux=1;
  }
  else if(PS.MEM_V==1 && Get_TRAP_OP(PS.MEM_CS)==1)
  {
      mem_pcMux=2;
  }
  else if(PS.MEM_V==1 && Get_BR_OP(PS.MEM_CS)==1)
  {
      int signal=((PS.MEM_IR & 0x0E00)>>9) & PS.MEM_CC;
      if(signal)
      {
          mem_pcMux=1;
      }
      else
      {
          mem_pcMux=0;
      }
  }
  else
  {
      mem_pcMux=0;
  }

  //update the new ps with the data in memory
  NEW_PS.SR_ADDRESS=PS.MEM_ADDRESS;
  NEW_PS.SR_NPC=PS.MEM_NPC;
  NEW_PS.SR_ALU_RESULT=PS.MEM_ALU_RESULT;
  NEW_PS.SR_IR=PS.MEM_IR;
  NEW_PS.SR_DRID=PS.MEM_DRID;

  if(DCACHE_EN==1 && dcache_assert==1)
  {
      NEW_PS.SR_DATA=read_in_data;
  }
  else
  {
      NEW_PS.SR_DATA=0;
  }

  NEW_PS.SR_V=PS.MEM_V & ((DCACHE_EN && dcache_assert) | (!DCACHE_EN));


  /* The code below propagates the control signals from MEM.CS latch
     to SR.CS latch. You still need to latch other values into the
     other SR latches. */
  for (ii=COPY_SR_CS_START; ii < NUM_MEM_CS_BITS; ii++) {
    NEW_PS.SR_CS [jj++] = PS.MEM_CS [ii];
  }

}


/************************* AGEX_stage() *************************/
int v_agex_cc, v_agex_ld_reg, agex_drid;
void AGEX_stage() {
  int adder1_output, adder2_output;
  //get adder1 output
  if(Get_ADDR1MUX(PS.AGEX_CS)==0)
  {
      adder1_output=PS.AGEX_NPC;
  }
  else
  {
      adder1_output=PS.AGEX_SR1;
  }

  //get adder2 output
  if(Get_ADDR2MUX(PS.AGEX_CS)==0)
  {
      adder2_output=0;
  }
  else if(Get_ADDR2MUX(PS.AGEX_CS)==1)
  {
      adder2_output=Low16bits(PS.AGEX_IR & 0x003F);
      if((adder2_output&0x0020)>>5==1)
      {
          adder2_output=Low16bits(adder2_output|(~0x003F));
      }
  }
  else if(Get_ADDR2MUX(PS.AGEX_CS)==2)
  {
      adder2_output=Low16bits(PS.AGEX_IR & 0X01FF);
      if((adder2_output&0x0100)>>8==1)
      {
          adder2_output=Low16bits(adder2_output|(~0x01FF));
      }
  }
  else if(Get_ADDR2MUX(PS.AGEX_CS)==3)
  {
      adder2_output=Low16bits(PS.AGEX_IR & 0X07FF);
      if((adder2_output&0x0400)>>10==1)
      {
          adder2_output=Low16bits(adder2_output|(~0x07FF));
      }
  }
  else
  {
      adder2_output=0;
  }

  //left shift operation
  int LSHF1_output;
  if(Get_LSHF1(PS.AGEX_CS)==1)
  {
      LSHF1_output=Low16bits(adder2_output<<1);
  }
  else
  {
      LSHF1_output=Low16bits(adder2_output);
  }

  //choose the output between the adder and the zext-lshf1 operation
  int address;
  if(Get_ADDRESSMUX(PS.AGEX_CS)==1)
  {
      address=Low16bits(adder1_output+LSHF1_output);
  }
  else
  {
      address=Low16bits((PS.AGEX_IR&0x00FF)<<1);
  }

  //ALU operations
  int SR1_input, SR2_input, ALU_output;
  SR1_input=Low16bits(PS.AGEX_SR1);
  if(Get_SR2MUX(PS.AGEX_CS)==0)
  {
      SR2_input=Low16bits(PS.AGEX_SR2);
  }
  else
  {
      SR2_input=Low16bits(PS.AGEX_IR & 0x001F);
      if((SR2_input&0x010)>>4==1)
      {
          SR2_input=Low16bits(SR2_input|0xFFE0);
      }
  }

  if(Get_ALUK(PS.AGEX_CS)==0)
  {
      ALU_output=SR1_input+SR2_input;
  }
  else if(Get_ALUK(PS.AGEX_CS)==1)
  {
      ALU_output=SR1_input & SR2_input;
  }
  else if(Get_ALUK(PS.AGEX_CS)==2)
  {
      ALU_output=SR1_input^SR2_input;
  }
  else if(Get_ALUK(PS.AGEX_CS)==3)
  {
      ALU_output=SR1_input;
  }
  else
  {
      ALU_output=0;
  }

  //shift operation
  int shift_input,shift_type,shift_amount,shift_output;

  shift_input=PS.AGEX_SR1;
  shift_type=(PS.AGEX_IR & 0X0030)>>4;
  shift_amount==(PS.AGEX_IR & 0X000F);

  if(shift_type==0)
  {
      shift_output=Low16bits(shift_input<<shift_amount);
  }
  else if(shift_type==1)
  {
      shift_output=Low16bits(shift_input>>shift_amount);
  }
  else if(shift_type==3)
  {
      shift_output=RSHF(shift_input, shift_amount, (shift_input)>>15 );
  }
  else
  {
      shift_output=0;
  }

  int mul_alu_output;
  if(Get_ALU_RESULTMUX(PS.AGEX_CS)==0)
  {
      mul_alu_output=Low16bits(shift_output);
  }
  else
  {
      mul_alu_output=Low16bits(ALU_output);
  }


  v_agex_br_stall=PS.AGEX_V & Get_AGEX_BR_STALL(PS.AGEX_CS);
  v_agex_ld_reg=PS.AGEX_V & Get_AGEX_LD_REG(PS.AGEX_CS);
  v_agex_cc=PS.AGEX_V & Get_AGEX_LD_CC(PS.AGEX_CS);
  agex_drid=PS.AGEX_DRID;





  int ii, jj = 0;
  int LD_MEM=mem_stall; /* You need to write code to compute the value of LD.MEM
		 signal */

  /* your code for AGEX stage goes here */



  if (!LD_MEM) {
    /* Your code for latching into MEM latches goes here */

    NEW_PS.MEM_ADDRESS=address;
    NEW_PS.MEM_NPC=PS.AGEX_NPC;
    NEW_PS.MEM_CC=PS.AGEX_CC;
    NEW_PS.MEM_ALU_RESULT=mul_alu_output;
    NEW_PS.MEM_IR=PS.AGEX_IR;
    NEW_PS.MEM_DRID=PS.AGEX_DRID;
    NEW_PS.MEM_V=PS.AGEX_V;
    /* The code below propagates the control signals from AGEX.CS latch
       to MEM.CS latch. */
    for (ii = COPY_MEM_CS_START; ii < NUM_AGEX_CS_BITS; ii++) {
      NEW_PS.MEM_CS [jj++] = PS.AGEX_CS [ii];
    }
  }
}



/************************* DE_stage() *************************/
void DE_stage() {

  int CONTROL_STORE_ADDRESS;  /* You need to implement the logic to
			         set the value of this variable. Look
			         at the figure for DE stage */
  int ii, jj = 0;
  /* You need to write code to compute the value of
		  LD.AGEX signal */

  /* your code for DE stage goes here */

  CONTROL_STORE_ADDRESS= get_bits(PS.DE_IR,15,11);
  int zero_bit=get_bits(PS.DE_IR,5,5);
  CONTROL_STORE_ADDRESS=(CONTROL_STORE_ADDRESS<<1)|zero_bit;

  int SR1_number=get_bits(PS.DE_IR,8,6);    //pass along SR1
  int SR2_decider=get_bits(PS.DE_IR,13,13); //get the 13th bits to identify if SR2 is at which bits
  int SR2_number=0;
  if(SR2_decider==1)
  {
      SR2_number=get_bits(PS.DE_IR,11,9);
  }
  else
  {
      SR2_number=get_bits(PS.DE_IR,2,0);
  }

  int DE_DRID;
  //see if the point to register is simply reg7 or other registers
  if(Get_DRMUX(CONTROL_STORE[CONTROL_STORE_ADDRESS])==0)
  {
      DE_DRID=get_bits(PS.DE_IR,11,9);
  }
  else
  {
      DE_DRID=7;
  }

  if(Get_DE_BR_OP(CONTROL_STORE[CONTROL_STORE_ADDRESS])==1)
  {
      dep_stall=v_agex_cc|v_mem_cc|v_sr_ld_cc;
  }

  int SR1_NEEDED_value=Get_SR1_NEEDED(CONTROL_STORE[CONTROL_STORE_ADDRESS]);
  if(SR1_NEEDED_value==1)
  {
      if((v_agex_ld_reg==1&&SR1_number==agex_drid) || (v_mem_ld_reg==1 && SR1_number==mem_drid)||(v_sr_ld_reg==1 && SR1_number==sr_reg_id))
      {
          dep_stall=1;
      }
  }

  int SR2_NEEDED_value=Get_SR2_NEEDED(CONTROL_STORE[CONTROL_STORE_ADDRESS]);
  if(SR2_NEEDED_value==1)
  {
      if((v_agex_ld_reg==1&&SR2_number==agex_drid) || (v_mem_ld_reg==1 && SR2_number==mem_drid)||(v_sr_ld_reg==1 && SR2_number==sr_reg_id))
      {
          dep_stall=1;
      }
  }
  dep_stall&=PS.DE_V;

  if(dep_stall==1)
  {
      NEW_PS.AGEX_V=0;
  }

  if(PS.DE_V==1 && Get_DE_BR_STALL(CONTROL_STORE[CONTROL_STORE_ADDRESS]==1))
  {
      v_de_br_stall=1;
  }
  else
  {
      v_de_br_stall=0;
  }

  if(v_sr_ld_reg==1)
  {
      REGS[sr_reg_id]=sr_reg_data;
  }

  LD_AGEX=!mem_stall;




  if (LD_AGEX) {
    /* Your code for latching into AGEX latches goes here */
    NEW_PS.AGEX_NPC=PS.DE_NPC;
    NEW_PS.AGEX_IR=PS.DE_IR;
    NEW_PS.AGEX_SR1=REGS[SR1_number];
    NEW_PS.AGEX_SR2=REGS[SR2_number];
    NEW_PS.AGEX_CC=(N<<2)|(Z<<1)|P;
    NEW_PS.AGEX_DRID=DE_DRID;

    NEW_PS.AGEX_V=!(dep_stall&v_de_br_stall)&PS.DE_V;



    /* The code below propagates the control signals from the CONTROL
       STORE to the AGEX.CS latch. */
    for (ii = COPY_AGEX_CS_START; ii< NUM_CONTROL_STORE_BITS; ii++) {
      NEW_PS.AGEX_CS[jj++] = CONTROL_STORE[CONTROL_STORE_ADDRESS][ii];
    }

    if(v_sr_ld_reg==1)
    {
        N=sr_n;
        Z=sr_z;
        P=sr_p;
    }
  }

}



/************************* FETCH_stage() *************************/
void FETCH_stage() {

  /* your code for FETCH stage goes here */
  int inst;
  icache_access(PC, &inst, &icache_r);
  LD_PC=v_de_br_stall|v_agex_br_stall|v_mem_br_stall;
  if(LD_PC)
  {
      if(mem_pcMux==0)
      {
          PC=PC+2;
      }
      else if(mem_pcMux==1)
      {
          PC=target_pc;
      }
      else if(mem_pcMux==2)
      {
          PC=trap;
      }
  }
  else
  {
      PC=PC;
  }

  LD_DE=(!mem_stall|!dep_stall);

  if(LD_DE==0)
  {
      if(icache_r==0 && LD_DE==0)
      {
          NEW_PS.DE_V=0;
      }
      else
      {
          NEW_PS.DE_V=1;
      }
      NEW_PS.DE_IR=inst;
      NEW_PS.DE_NPC=PC+2;
  }
}
