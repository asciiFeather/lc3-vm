/*
Process:
    1.) Know the architecture (instruction set, memory, etc).
    2.) Implement the memory, registers and opcodes.
    3.) Define the opcodes and construct instruction set.
    4.) Define trap routines.

Compile with `cl /EHsc main.c` or `gcc main.c`

How do programs get loaded?
    The origin is first read, from there the rest of the data will be read and executed.
*/
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <signal.h>
#include <Windows.h>
#include <conio.h>

HANDLE hStdin = INVALID_HANDLE_VALUE;
/* 
Store the registers in an enum then implement them in an array. 
    Explanation:
        General purpose registers (R0-R7) is used to perform any calculations.
        Program counter (R_PC): address of next instruction to execute.
        Condition flags (R_COND): information about prev calculation.
*/
// in most cases, r0 is the DR.
enum 
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, // program counter.
    R_COND,
    R_COUNT
};

// Define the condition flags. LC3 has 3 condition flags to indicate the prev calculation. 
enum 
{
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2
};

/* Instruction set 
Each instruction is 16 bits long, with the left 4 bits storing the opcode, the rest are used for parameters.
Instructions accept a parameter and an opcode. Opcode represents a task the CPU knows how to perform.*/
enum
{
    OP_BR = 0, // branch
    OP_ADD,
    OP_LD,
    OP_ST,
    OP_JSR, // jump register
    OP_AND,
    OP_LDR, // load register
    OP_STR, // store register
    OP_RTI,
    OP_NOT,
    OP_LDI, // load indirect
    OP_STI, // store indirect
    OP_JMP, // jump back.
    OP_RES,
    OP_LEA,
    OP_TRAP
};


/**
 * Memory mapped registers are not located in the register table but are instead located in memory.
 * To reach these registers, you must map their location in memory.
*/
enum
{
    MR_KBSR = 0xFE00, // is a key pressed?
    MR_KBDR = 0xFE02 // what is the key pressed?
};

// Enum for trap routines/codes, similar to opcodes, these routines act as functions for a specific task.
// Similar to OS system calls

// When a trap code is called a C function will be called. When complete, return to the instructions.
enum 
{
    TRAP_GETC = 0x20, /* get character from keyboard. */
    TRAP_OUT = 0x21, /* output a character. */
    TRAP_PUTS = 0x22, /* output string */
    TRAP_IN = 0x23, /* get character from keyboard, echoed in the terminal. */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25 /* hlt */
};

uint16_t memory[UINT16_MAX]; // has 65536 locations. 128kb only
uint16_t registers[R_COUNT];

// Extend the sign to reach 16 bits.
// If positive fill it in with 0s, if negative fill it in with 1s.
uint16_t signExtend(uint16_t x, int bitCount)
{
    if ((x >> (bitCount - 1)) & 1) {
        x |= (0xFFFF << bitCount);
    }
    return x;
}

uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

// Update the flags everytime a value is written to a register.
// Update the condition if its set to a negative, positive or zero.
void updateFlgs(uint16_t r)
{
    if (registers[r] == 0)
    {
        registers[R_COND] = FL_ZRO;
    }
    else if (registers[r] >> 15) // if the value is shifted to the left most bit and it is a 1, set the flag to negative.
    {
        registers[R_COND] = FL_NEG;
    }
    else
    {
        registers[R_COND] = FL_POS;
    }
}

void readImageFile(FILE* f)
{
    // origin: where in memory to place the image.
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, f);
    origin = swap16(origin);

    // maximum file size it can use.
    uint16_t maxRead = UINT16_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), maxRead, f);

    /** swap to little endian. 
     * store the most significant byte to the largest memory address, 
     * store the least-significant to the smallest.**/
    while (read-- > 0)
    {
        // The asterisk (*) turns a pointer into a value, we want the value in this case.
        *p = swap16(*p); // swap each uint16_t that is loaded to fit in with the little endian computers, most of the modern computers.
        ++p;
    }
}

// Takes a path
int readImage(const char* imagePath)
{
    FILE* file = fopen(imagePath, "rb");
    if (! file) { return 0; };
    readImageFile(file);
    fclose(file);
    return 1;
}


uint16_t check_key()
{
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}

void mem_write(uint16_t address, uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t addr)
{
    if (addr == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1<<15); // get the keyboard status
            memory[MR_KBDR] = getchar(); // get the key
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[addr];
}

/* Input buffering. */
#pragma region INPUT_BUFF
DWORD fdwMode, fdwOldMode;

void disable_input_buffering()
{
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwOldMode); // save old mode
    fdwMode = fdwOldMode
            ^ ENABLE_ECHO_INPUT  /* no input echo */
            ^ ENABLE_LINE_INPUT; /* return when one or
                                    more characters are available */
    SetConsoleMode(hStdin, fdwMode); /* set new mode */
    FlushConsoleInputBuffer(hStdin); /* clear buffer */
}

void restore_input_buffering()
{
    SetConsoleMode(hStdin, fdwOldMode);
}
#pragma endregion

void handle_interrupt(int sgnal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

int main(int argc, char const *argv[])
{
    // Accept inputs from user to load in images.
    if (argc < 2) {
        // if nothing is given show a usage guide.
        printf("main.exe <image-file1> ... \n");
        exit(2); // exit program
    }

    for (int j = 1; j < argc; ++j)
    {
        if (!readImage(argv[j]))
        {
            printf("Failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    // Set the PC to it's start position, 0x3000 by default
    enum { PC_START = 0x3000 };
    registers[R_PC] = PC_START; // Add 0x3000 to the program counter as it's address.

    int running = 1; // is it running?
    while (running)
    {
        // Fetch an instruction from memory to the address of program counter which is now 0x3000.
        // Increment PC
        uint16_t instr = mem_read(registers[R_PC]++);
        uint16_t op = instr >> 12; // grab the opcode through shifting instr to the left 12 times.

        // now construct the instruction set.
        switch (op)
        {
            case OP_ADD:
                /*
                Takes in three values, first - sum value, second - first number to be added, third - second number to be added (is a register or not depends on the mode).
                    Clarification of modes:
                        - Register mode: uses registers to store each value, `ADD R2 R0 R1` adds the contents of R0 to R1 then stores it to R2
                        - Immediate mode: can use a value instead of using registers, this value is stored in the imm5 space. Can only use 2^5=32 (unsigned) though.
                    Architecture:
                        Register mode:
                            0001 (OP_ADD) | DR (destination) | SR1 (first no.) | 0 (mode 0 = register, 1 = immediate) | 00 (unused) | SR2 (2nd no.)
                        Immediate mode:
                            0001 (OP_ADD) | DR (destination) | SR1 (first no.) | 1 (mode, in this case 1, which eq to immediate) | imm5 (value of 2nd no., instead of register)
                
                */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    // First no. (SR1)
                    // Located at the 6th bit (shift right 6 times n >> 6).
                    uint16_t r1 = (instr >> 6) & 0x7;
                    // Check if we are in immediate mode.
                    // imm_flag is located at the 5th bit (shift right 5 times n >> 5).
                    uint16_t imm_flag = (instr >> 5) & 0x1;
                    
                    if (imm_flag)
                    {
                        uint16_t imm5 = signExtend(instr & 0x1F, 5);
                        registers[r0] = registers[r1] + imm5; // Get the result of sr1 + imm5 and store it in DR.
                    }
                    else // go to register mode.
                    {
                        // create the sr2 register for the second value.
                        uint16_t r2 = instr & 0x7;
                        registers[r0] = registers[r1] + registers[r2]; // Get the result of sr1 + sr2 and store it in DR.
                    }
                    
                    // When an instruction modifies a register, the condition flags must update.
                    updateFlgs(r0);   
                }
                break;
            case OP_AND:
                {
                    // Similar to OP_ADD except it uses the and operator.
                    // r1 & r2
                    // r1 & imm5
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                    uint16_t imm_flag = (instr >> 5) & 0x1;

                    if (imm_flag)
                    {
                        uint16_t imm5 = signExtend(instr & 0x1F, 5);
                        registers[r0] = registers[r1] & imm5;
                    }
                    else 
                    {
                        // Register mode.
                        uint16_t r2 = instr & 0x7;
                        registers[r0] = registers[r1] & registers[r2];
                    }

                    updateFlgs(r0);
                }
                break;
            case OP_NOT:
                {
                    // The bitwise complement of SR are stored in DR.
                    uint16_t r0 = (instr >> 9) & 0x7;
                    // source register. 
                    uint16_t r1 = (instr >> 6) & 0x7;
                    registers[r0] = ~registers[r1];  // perform a bitwise NOT function (~)
                    updateFlgs(r0);
                }
                break;
            case OP_BR:
                {
                    // Checks if the conditions are true then adds pcOffset with current PC.
                    uint16_t pcOffset = signExtend(instr & 0x1FF, 9);
                    uint16_t condFlag = (instr >> 9) & 0x7;
                    if (condFlag & registers[R_COND])
                    {
                        registers[R_PC] += pcOffset;
                    }
                }
                break;
            case OP_JMP:
                {
                    // handles RET: special case for JMP happens when R1 is 7.
                    uint16_t r1 = (instr >> 6) & 0x7;
                    registers[R_PC] = registers[r1]; // program counter gets set to r1.
                }
                break;
            case OP_JSR:
                {
                    uint16_t longFlag = (instr >> 11) & 1;
                    registers[R_R7] = registers[R_PC];
                    if (longFlag)
                    {
                        // Similar to immediate mode. long flag mode.
                        uint16_t longPcOffset = signExtend(instr & 0x7FF, 11);
                        // Add longPcOffset to the current PC
                        registers[R_PC] += longPcOffset; 
                    }
                    else 
                    {
                        uint16_t r1 = (instr >> 6) & 0x7;
                        registers[R_PC] += registers[r1]; // Jump register's register.
                    }
                }
                break;
            case OP_LD:
                {
                    // LD is similar to LDI except used for nearer locations.
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pcOffset = signExtend(instr & 0x1FF, 9);
                    // look at the memory location of current PC with the pcOffset.
                    registers[r0] = mem_read(registers[R_PC] + pcOffset);
                    updateFlgs(r0); // update flags because r0 (DR) was modified.
                }
                break;
            case OP_LDI:
                // load a value from memory into a register.
                // Binary layout: 
                // 1010 (OP_LDI) | DR (dest reg) | PCOffset9 (Embedded value (similar to imm5), an address of where to load from)
                /*
                * Process: 
                * 1.) Sign extend 9 bit value.
                * 2.) Add it to the current PC.
                * */
                // Result gives address to a location in memory.
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    // PCOffset9.
                    uint16_t pcOffset = signExtend(instr & 0x1FF, 9); 
                    // look at the memory location of the location of current PC + pcOffset.
                    registers[r0] = mem_read(mem_read(registers[R_PC] + pcOffset));

                    updateFlgs(r0);
                }
                break;
            case OP_LDR:
                {
                    // Sign extend bits 5:0 (offset6) to 16 bits. Add this value to the contents of the register, baseR. 
                    // contents are stored at DR.

                    uint16_t r0 = (instr >> 9) & 0x7;
                    // BaseR
                    uint16_t r1 = (instr >> 6) & 0x7;
                    // offset6
                    uint16_t offset = signExtend(instr & 0x3F, 6);
                    registers[r0] = mem_read(registers[r1] + offset);
                    updateFlgs(r0); 
                }
                break;
            case OP_LEA:
                // Sign extend bits 8:0 which is PCoffset9 to 16 bits.
                // Add this value to the current PC.
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pcOffset = signExtend(instr & 0x1FF, 9);
                    registers[r0] = registers[R_PC] + pcOffset;
                    updateFlgs(r0);
                }
                break;
            case OP_ST:
                // SR is stored in the mem location which is a sign extend of 8:0 (PCoffset9) to 16 bits.
                // Add this value to the current PC and store in memory.
                {
                    // source register.
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pcOffset = signExtend(instr & 0x1FF, 9);
                    mem_write(registers[R_PC] + pcOffset, registers[r0]);
                }
                break;
            case OP_STI:
                {
                    // Similar to ST contents of SR are stored in memory through PCOffset9 which is sign extended to 16 bits.
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pcOffset = signExtend(instr & 0x1FF, 9);
                    mem_write(mem_read(registers[R_PC] + pcOffset), registers[0]); // write the address of pcOffset incremented with current PC then store r0 to offset.
                }
                break;
            case OP_STR:
                {
                    // Contents of SR are stored in the offset6 which gets sign extended to 16 bits
                    // Write this value + the SR contents to the memory location.
                    // SR
                    uint16_t r0 = (instr >> 9) & 0x7;
                    // BaseR
                    uint16_t r1 = (instr >> 6) & 0x7;
                    // offset6
                    uint16_t offset = signExtend(instr & 0x3F, 6);
                    mem_write(registers[r1]+offset, registers[r0]);
                }
                break;
            case OP_TRAP:
                switch (instr & 0xFF) 
                {
                    case TRAP_GETC:
                        // Read a character from the keyboard but not echoed.
                        registers[R_R0] = (uint16_t)getchar(); // read the char from keyboard then store it in R0.
                        break;
                    case TRAP_OUT:
                        // Write a char in R0 to the console.
                        putc((char)registers[R_R0], stdout); // put R0's char then output it to the console.
                        fflush(stdout); // clean stdout
                        break;
                    case TRAP_PUTS:
                        // outputs a string.
                        // store the first character's address in R0 before the trap begins.
                        // the string starts from the first character's address all the way to the last.
                        // each character is not stored in a byte but instead a single memory location.
                        {
                            /* one character per word. */
                            // convert each string to chars and output them individually
                            uint16_t* c = memory + registers[R_R0];
                            while (*c) 
                            {
                                putc((char)*c, stdout); // put each character then output them.
                                ++c; // get all c.
                            }
                            fflush(stdout); // clean up.
                        }
                        break;
                    case TRAP_IN:
                        {
                            // print a prompt.
                            // read character from keyboard.
                            // echo character.
                            // store in R0.
                            printf(" ");
                            char c = getchar();
                            putc(c, stdout);
                            registers[R_R0] = (uint16_t)c;
                        } 
                        break;
                    case TRAP_PUTSP:
                        {
                            /*
                            this uses one character per byte.
                            prints two characters for every address.
                            */
                            uint16_t* c = memory + registers[R_R0];
                            while (*c)
                            {
                                char char1 = (*c) & 0xFF; // [7:0]
                                putc(char1, stdout);
                                char char2 = (*c) >> 8; // ASCII code to be written to the console.
                                if (char2) putc(char2, stdout);
                                ++c; // put together the characters.
                            }
                            fflush (stdout);
                        }
                        break;
                    case TRAP_HALT:
                        puts("Done!");
                        fflush(stdout);
                        running = 0;
                        break;
                }
                break;
            case OP_RES: // reserved (unused)
                abort(); 
            case OP_RTI: // unused as well.
                abort();
            default:
                // return an error
                // BAD_OPCODE
                abort();
                break;

        }
    }

    restore_input_buffering();
    return 0;
}