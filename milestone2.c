#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


#define MEMORY_SIZE 2048
uint32_t memory[MEMORY_SIZE]; //el uint32 de ashan unsigned int w ybaa 32 bit

#define WORD_SIZE 32

#define INSTRUCTION_START 0
#define INSTRUCTION_END 1023

#define DATA_START 1024
#define DATA_END 2047

#define NUM_REGISTERS 32
uint32_t registers[NUM_REGISTERS];

#define ZERO_REG 0 

int32_t PC = 0;

#define GET_OPCODE(instr)      ((instr >> 28) & 0xF)
//r-type
#define GET_R1(instr)          ((instr >> 23) & 0x1F)
#define GET_R2(instr)          ((instr >> 18) & 0x1F)
#define GET_R3(instr)          ((instr >> 13) & 0x1F)
#define GET_SHAMT(instr)       (instr & 0x1FFF)

// i-type
#define GET_IMMEDIATE(instr)   (instr & 0x3FFFF)  

//j-type 
#define GET_ADDRESS(instr)     (instr & 0x0FFFFFFF)
//ashan lw value el imm -ve
#define SIGN_EXTEND_IMM18(imm)  (((imm) & 0x20000) ? ((imm) | 0xFFFC0000) : (imm))

int *decodedArray = NULL;

int program_length = 0;
int flagprint=0;


int clk = 1;
int WB_NUM=-3;

int FINAL_RESULT;

int result_reg;
int mem_opcode;

int totalcycles;

typedef enum {
    OPCODE_ADD,
    OPCODE_SUB,
    OPCODE_MULI,
    OPCODE_ADDI,
    OPCODE_BNE,
    OPCODE_ANDI,
    OPCODE_ORI,
    OPCODE_J,
    OPCODE_SLL,
    OPCODE_SRL,
    OPCODE_LW,
    OPCODE_SW
} Opcode;

int FETCH_INST = 0;

int DECODE_INST = 1;

int EXCUTE_INST = 1;
int Excute_Flag = 0;

int MEM_INST = 1;
int MEM_FLAG = 0;

int flagflush;

int WB_INST = 1;
int WB_FLAG = 0;

//kolo 0 fl awl
void initialize (){
 for (int i=0;i<MEMORY_SIZE;i++){
    memory[i]=0;
 }
 for(int i=1;i<NUM_REGISTERS;i++){
    registers[i] = 0;
 }
 registers[ZERO_REG] = 0;

 PC=INSTRUCTION_START;

}

//keep R0 always 0
void enforce_zero_register() {
    registers[ZERO_REG] = 0;
}



#define PIPELINE_DEPTH 5 // Number of pipeline stages

typedef struct {
    int Fetch_Inst;
    int dest_reg;    // Destination register for write-back
    int result;
    int opcode;      // Result from the execute stage
    int32_t pc;
    int terminate;
    int flush;
} PipelineStage;

PipelineStage pipeline[PIPELINE_DEPTH];

// Initialize the pipeline stages
void init_pipeline() {
    for (int i = 0; i < PIPELINE_DEPTH; i++) {
        pipeline[i].Fetch_Inst = 0;
        pipeline[i].dest_reg = -1;
        pipeline[i].result = 0;
        pipeline[i].opcode = -1;
        pipeline[i].pc=-1;
        pipeline[i].terminate=0;
        pipeline[i].flush=0;
    }
}

void shift_pipeline() {
  // Shift results and destination registers one position
    for (int i = 4; i > 0; i--) {
        pipeline[i].Fetch_Inst = pipeline[i - 1].Fetch_Inst;
        pipeline[i].dest_reg = pipeline[i - 1].dest_reg;
        pipeline[i].result = pipeline[i - 1].result;
        pipeline[i].opcode = pipeline[i - 1].opcode;
        pipeline[i].pc= pipeline[i - 1].pc;
    }
}




//naming register
// void print_registers() {
//     for (int i = 0; i < NUM_REGISTERS; i++) {
//         printf("R%d = %u\n", i, registers[i]);
//     }
//     printf("PC = %u\n", PC);
// }

void print_registers() {
    printf("Register File (Signed and Unsigned):\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
        printf("R%-2d = %11u (signed: %d)\n", i, registers[i], (int32_t)registers[i]);
    }
    printf("PC = %d\n", PC);  // PC is int32_t, so signed is appropriate
}




// void print_memory_binary() {
//     printf("Memory Contents (Binary):\n");
//     for (int i = 0; i < MEMORY_SIZE; i++) {
//         if (memory[i] != 0) {
//             printf("memory[%d] = ", i);
//             for (int bit = 31; bit >= 0; bit--) {
//                 printf("%u", (memory[i] >> bit) & 1);
//             }
//             printf("\n");
//         }
//     }
// }

// void print_memory_binary() {
//     printf("Memory Contents (Decimal and Binary):\n");
//     for (int i = 0; i < MEMORY_SIZE; i++) {
//         if (memory[i] != 0) {
//             // Print index and decimal value
//             printf("memory[%d] = %d\t, (", i, memory[i]);

//             // Print 32-bit binary
//             for (int bit = 31; bit >= 0; bit--) {
//                 printf("%u", (memory[i] >> bit) & 1);
//             }

//             printf(")\n");
//         }
//     }
// }

void print_memory_binary() {
    printf("Memory Contents (Decimal and Binary):\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i] != 0) {
            printf("memory[%d] = %u (signed: %d)\t(", i, memory[i], (int32_t)memory[i]);

            for (int bit = 31; bit >= 0; bit--) {
                printf("%u", (memory[i] >> bit) & 1);
            }

            printf(")\n");
        }
    }
}




void load_program_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return;
    }

    char line[256];
    int instr_index = INSTRUCTION_START;

    while (fgets(line, sizeof(line), file) != NULL && instr_index <= INSTRUCTION_END) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;

        uint32_t instruction = 0;
        char op[10];
        unsigned int a, b, c;

        if (sscanf(line, "%s R%u R%u R%u", op, &a, &b, &c) == 4) {
            if (strcmp(op, "ADD") == 0)
                instruction = (OPCODE_ADD << 28) | (a << 23) | (b << 18) | (c << 13);
            else if (strcmp(op, "SUB") == 0)
                instruction = (OPCODE_SUB << 28) | (a << 23) | (b << 18) | (c << 13);
        }
        else if (sscanf(line, "%s R%u R%u %u", op, &a, &b, &c) == 4) {
            if (strcmp(op, "ADDI") == 0)
                instruction = (OPCODE_ADDI << 28) | (a << 23) | (b << 18) | (c & 0x3FFFF);
            else if (strcmp(op, "MULI") == 0)
                instruction = (OPCODE_MULI << 28) | (a << 23) | (b << 18) | (c & 0x3FFFF);
            else if (strcmp(op, "ANDI") == 0)
                instruction = (OPCODE_ANDI << 28) | (a << 23) | (b << 18) | (c & 0x3FFFF);
            else if (strcmp(op, "ORI") == 0)
                instruction = (OPCODE_ORI << 28) | (a << 23) | (b << 18) | (c & 0x3FFFF);
            else if (strcmp(op, "LW") == 0)
                instruction = (OPCODE_LW << 28) | (a << 23) | (b << 18) | (c & 0x3FFFF);
            else if (strcmp(op, "SW") == 0)
                instruction = (OPCODE_SW << 28) | (a << 23) | (b << 18) | (c & 0x3FFFF);
            else if (strcmp(op, "SLL") == 0)
                instruction = (OPCODE_SLL << 28) | (a << 23) | (b << 18) | (c & 0x1FFF);
            else if (strcmp(op, "SRL") == 0)
                instruction = (OPCODE_SRL << 28) | (a << 23) | (b << 18) | (c & 0x1FFF);
            else if (strcmp(op, "BNE") == 0)
                instruction = (OPCODE_BNE << 28) | (a << 23) | (b << 18) | (c & 0x3FFFF);
        }
        else if (sscanf(line, "%s %u", op, &a) == 2 && strcmp(op, "J") == 0) {
            instruction = (OPCODE_J << 28) | (a & 0x0FFFFFFF);
        }
        else {
            printf("Unknown instruction: %s", line);
            continue;
        }

        memory[instr_index] = instruction;
        instr_index++;

       
    }
    memory[instr_index]=-1;

    program_length = instr_index;
    fclose(file);
}

void flushing(){
    pipeline[0].Fetch_Inst=0;
    pipeline[0].dest_reg=-1;
    pipeline[0].opcode=-1;
    pipeline[0].pc=-1;
    pipeline[0].result=0;
    pipeline[0].flush=1;

    pipeline[1].Fetch_Inst=0;
    pipeline[1].dest_reg=-1;
    pipeline[1].opcode=-1;
    pipeline[1].pc=-1;
    pipeline[1].result=0;
    pipeline[1].flush=1;

    pipeline[2].Fetch_Inst=0;
    pipeline[2].dest_reg=-1;
    pipeline[2].opcode=-1;
    pipeline[2].pc=-1;
    pipeline[2].result=0;
    pipeline[2].flush=1;

     for (int i=0;i<7;i++){
             decodedArray[i]=0;
       }
    


}

void  fetch(){
    
     
        if(program_length>PC && (clk % 2 == 1)){
        
            int instruction = memory[PC];
            if(instruction==-1){
                 pipeline[1].terminate=1;
            }
            
            FETCH_INST++;
            PC++;
            printf("Fetching instruction number %d from the Memory with value: %d\n" , PC ,instruction );
            pipeline[0].pc=PC;
            pipeline[0].Fetch_Inst = instruction;
            
            //
            pipeline[1].Fetch_Inst = pipeline[0].Fetch_Inst;
            pipeline[1].dest_reg = pipeline[0].dest_reg;
            pipeline[1].result = pipeline[0].result;
            pipeline[1].opcode = pipeline[0].opcode;
            pipeline[1].pc=PC;
            pipeline[1].terminate=0;
            pipeline[1].flush=0;
            
           
            
        }   
        //
       else if (PC >= program_length )
    {



       if(clk % 2 == 1){
          pipeline[1].terminate=1;
       }
        
        printf( "Finished Fetching All Instructions\n" );
    }
    
}


void decode(){
    
    
    if (pipeline[1].terminate==1){
        printf("finished decoding\n");
        if(clk % 2 == 1){
          pipeline[2].terminate=1;
       }
 
        
    }
    else{



    int instruction = pipeline[1].Fetch_Inst;


    int *OutgoingArray = malloc(7 * sizeof(int));

     if (clk % 2 == 0)
    {
    
    uint32_t opcode = GET_OPCODE(instruction);
    OutgoingArray[0] = opcode;
    uint32_t R1 = GET_R1(instruction);
    OutgoingArray[1] = R1;
    uint32_t R2 = GET_R2(instruction);
    OutgoingArray[2] = R2;
    uint32_t R3 = GET_R3(instruction);
    OutgoingArray[3] = R3;
    uint32_t Shamt = GET_SHAMT(instruction);
    OutgoingArray[4] = Shamt;
    int32_t Imm = SIGN_EXTEND_IMM18(GET_IMMEDIATE(instruction));
    OutgoingArray[5] = Imm;
    uint32_t Address = GET_ADDRESS(instruction);
    OutgoingArray[6] = Address;
     

    decodedArray = OutgoingArray;

    if (instruction != 0 && (pipeline[1].pc <= program_length)) 
        {
            printf("Decoding instruction number %d with code: %d\n",pipeline[1].pc , instruction);
            pipeline[1].dest_reg = decodedArray[1];
            pipeline[1].opcode = decodedArray[0];
        }
        else
        {
             if(pipeline[1].pc==-1){
                    printf("No Decoding Due To Flushing.\n");
            }
            else{
            printf( "Finished Decoding All Instructions\n");
            }
        }

    }

     else 
    {
        if (instruction != 0)
        { 
            if (pipeline[1].pc <= program_length)
            {
                if(pipeline[1].pc==-1){
                    printf("No Decoding Due To Flushing.\n");
            }
                pipeline[1].dest_reg = decodedArray[1]; 
                printf("Decoding instruction number %d with code : %d\n",pipeline[1].pc , instruction);
                pipeline[1].opcode = decodedArray[0];
                DECODE_INST++;

                //
                pipeline[2].Fetch_Inst = pipeline[1].Fetch_Inst;
                pipeline[2].dest_reg = pipeline[1].dest_reg;
                pipeline[2].result = pipeline[1].result;
                pipeline[2].opcode = pipeline[1].opcode;
                pipeline[2].pc= pipeline[1].pc;
                pipeline[2].flush= pipeline[1].flush;
            }
            else 
            {
                if(pipeline[1].pc==-1){
                    printf("No Decoding Due To Flushing.\n");
                 }
                 else{
                printf( "Finished Decoding All Instructions\n" );
                 }
            }
            Excute_Flag = 1;
        }
        if(clk!=1){

        
        if(pipeline[1].pc==-1){
                    printf("No Decoding Due To Flushing.\n");
                 }
      }
    }
    
    

    }


}

int x;

void execute(){
   
     
  

    
    if (pipeline[2].terminate==1){
        printf("finished executing\n");
        pipeline[3].terminate=1;
       
       
    }

    else{
        int instruction= pipeline[2].Fetch_Inst;
   

    int result=0;
    
    
    if(clk %2 ==0){
         x=0;
         flagflush=0;
        if (decodedArray != NULL && Excute_Flag == 1){

        switch (decodedArray[0]){
         case OPCODE_ADD:
           result = registers[decodedArray[2]] + registers[decodedArray[3]];
            break;
        case OPCODE_SUB:
            result = registers[decodedArray[2]] - registers[decodedArray[3]];
            break;
        case OPCODE_ADDI:
            result = registers[decodedArray[2]] + decodedArray[5];
            break;
        case OPCODE_MULI:
            result = registers[decodedArray[2]] * decodedArray[5];
            break;
        case OPCODE_ANDI:
            result = registers[decodedArray[2]] & decodedArray[5];
            break;
        case OPCODE_ORI:
           result = registers[decodedArray[2]] | decodedArray[5]; 
            break;
        case OPCODE_SLL:
            result = registers[decodedArray[2]] << decodedArray[4];
            break;
        case OPCODE_SRL:
             result = registers[decodedArray[2]] >> decodedArray[4];
            break;
        case OPCODE_BNE:
             if(registers[decodedArray[1]] != registers[decodedArray[2]]){
               x= pipeline[2].pc + decodedArray[5];
               flagflush=1;
             }
            break;
        case OPCODE_J:
             x= (pipeline[2].pc & 0xF0000000) | decodedArray[6];
             flagflush=1;
            break;
        case OPCODE_LW:
        case OPCODE_SW:
            result = registers[decodedArray[2]] + decodedArray[5];
            
            break;
        default:
            printf("[EX] Unknown opcode: %d\n", decodedArray[0]);
            break;
    }

     if (pipeline[2].pc <= program_length)
            {
                if(pipeline[2].pc==-1){
                    printf("No Execution Due To Flushing.\n");
                }

                else{
                printf("Excuting instruction number: %d\n", pipeline[2].pc);
                }
            }
            else
            {
                printf( "Finished Excuting All Instructions\n" );
            }
         
            FINAL_RESULT = result;
            if (pipeline[2].Fetch_Inst != -1) 
            {
                pipeline[2].result = result;
                
                result_reg = pipeline[2].dest_reg; 
                mem_opcode = pipeline[2].opcode;

                
            }

        }

    }
    else{

        if (Excute_Flag == 1)
        {
            if (pipeline[2].pc <= program_length)
            {
                 if(pipeline[2].pc==-1){
                    printf("No Execution Due To Flushing.\n");
                }
                else{
                printf("Excuting instruction number: %d\n", pipeline[2].pc);
                EXCUTE_INST++;
                MEM_FLAG = 1;
                pipeline[2].result = FINAL_RESULT;
              
                pipeline[2].dest_reg = result_reg;
                pipeline[2].opcode = mem_opcode;

                //
                pipeline[3].Fetch_Inst = pipeline[2].Fetch_Inst;
                pipeline[3].dest_reg = pipeline[2].dest_reg;
                pipeline[3].result = pipeline[2].result;
                pipeline[3].opcode = pipeline[2].opcode;
                pipeline[3].pc= pipeline[2].pc;
                 pipeline[3].flush= pipeline[2].flush;
               
               
                }
                

            }
            else
            {
                printf( "Finished Excuting All Instructions\n" );
            }
        }

    }
    }

}

void memory_access(){
    
    
    if (pipeline[3].terminate==1){
        printf("finished memory access\n");
        pipeline[4].terminate=1;
        
    }
   
    else{
     

   
    int res=  pipeline[3].result;

    if (clk % 2 == 0)
    {
        if (MEM_FLAG == 1) {
            int temp_opcode = pipeline[3].opcode;
            int temp_result = pipeline[3].result;
            int temp_reg = pipeline[3].dest_reg;
            if (temp_opcode != -1) {
                if (temp_opcode == OPCODE_LW) {
                    printf("Memory Access At Instruction Number: %d\n", pipeline[3].pc );
                    printf("Memory at index %d is being read from memory with value: %d\n",temp_result,memory[temp_result]);
                    res =memory[temp_result];
                    // registers[temp_reg] = memory[temp_result];
                } else if (temp_opcode == OPCODE_SW) {
                    memory[temp_result] = registers[temp_reg];
                    printf("Memory Access At Instruction Number: %d\n", MEM_INST);
                    printf( "Storing in Memory at index %d with value %d from register %d\n" ,temp_result,registers[temp_reg],temp_reg);
                }
                else
                {
                     if(pipeline[3].pc==-1){
                    printf("No Memory Access Due To Flushing.\n");
                }
                    printf( "Memory Access At Instruction Number %d with no effect\n" , pipeline[3].pc );    
                }
            }

              if (pipeline[3].pc <= program_length) {
                MEM_INST++;
                WB_FLAG = 1;
            }
        }

                

                pipeline[4].Fetch_Inst = pipeline[3].Fetch_Inst;
                pipeline[4].dest_reg = pipeline[3].dest_reg;
                pipeline[4].result = res;
                pipeline[4].opcode = pipeline[3].opcode;
                pipeline[4].pc= pipeline[3].pc;
                pipeline[4].flush= pipeline[3].flush;

                pipeline[3].Fetch_Inst = 0;
                pipeline[3].dest_reg = -1;
                pipeline[3].result = 0;
                pipeline[3].opcode = -1;
                pipeline[3].pc=-1;
                pipeline[3].terminate=0;
                pipeline[3].flush=0;
    }
    }

}

void writeback(){
   
    

    
    if (pipeline[4].terminate==1){
        printf("finished writeback and program\n");
       
    }
   
    else{

   

    if (WB_FLAG == 1 && (clk % 2 == 1))
    {
        int First_Reg;
        int result;
       

        if (pipeline[4].pc <= program_length)
        {
            if(pipeline[4].pc==-1){
                 printf("No Write Back Due To Flushing .\n");
            }
            else{
            printf("Writing back at instruction number: %d\n",pipeline[4].pc);
            WB_INST++;
            }
        } 

        First_Reg = pipeline[4].dest_reg;
        result = pipeline[4].result;
        int temp_opcode = pipeline[4].opcode;
         if (First_Reg > 0 && First_Reg < 32) 
        {
         switch (temp_opcode){
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_ADDI:
        case OPCODE_MULI:
        case OPCODE_ANDI:
        case OPCODE_ORI:
        case OPCODE_SLL:
        case OPCODE_SRL:
        case OPCODE_LW:
            registers[First_Reg] = result;
            printf( "Register Number %d is changed to %d\n" , First_Reg, result);   
            break;
        case OPCODE_SW:
        case OPCODE_BNE:    
        case OPCODE_J:
            printf("No Write Back Here\n");
            break;
        default:
            
            printf("[EX] Unknown opcode: %d\n", temp_opcode);
            break;
    }
    pipeline[4].Fetch_Inst = 0;
    pipeline[4].dest_reg = -1;
    pipeline[4].result = 0;
    pipeline[4].opcode = -1;
    pipeline[4].pc=-1;
    pipeline[4].terminate=0;
    pipeline[4].flush=0;
   }
   else 
        { 
    
            if(pipeline[4].pc==-1 ){
        
            }
            else{
                 printf("Invalid register index: %d ,cannot be overwritten \n", First_Reg);
            }
        }   

    }

     
        if (clk % 2 == 1)
    {
        WB_NUM++;
    }
    
    }

}

   


void run_program() {
    while(pipeline[4].terminate==0) {
        printf("\nCycle :%d\n", clk);
        printf("\nPC :%d\n", PC);
        
        writeback();
        memory_access();
        execute();
        decode();
        fetch(); 
        if(clk % 2 == 1){

       
         if(flagflush==1){
                  PC=x;
                  flushing();
                }
         }
        // shift_pipeline();
        // print_registers();
        // print_memory_binary();
        
        clk++;
    }
}




int main() {
   

    initialize();
    load_program_from_file("program.asm");
    
    totalcycles=( 7 + ((program_length - 1) *2));
    printf("Number of Instructions for this file is :%d\n",program_length);
    // printf("Number of Cycles for this file is :%d\n",totalcycles);
    
    init_pipeline();
    run_program();
    
    print_memory_binary();
    print_registers();

    return 0;
}
