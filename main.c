/*
Process:
    1.) Know the architecture (instruction set, memory, etc).
    2.) Implement the memory, registers and opcodes.
    3.) Define the opcodes and construct instruction set.
    4.) Define trap routines.

Compile with `cl /EHsc main.c` or `gcc main.c`
*/
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <Windows.h>
#include <conio.h>

HANDLE hStdin = INVALID_HANDLE_VALUE;

/* There are 65536 locations in memory each of which stores a 16 bit value. */
uint16_t memory[UINT16_MAX];

/* 
Store the registers in an enum then implement them in an array. 
    Explanation:
        General purpose registers (R0-R7) is used to perform any calculations.
        Program counter (R_PC): address of next instruction to execute.
        Condition flags (R_COND): information about prev calculation.
*/
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

uint16_t registers[R_COUNT]

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

// Define the condition flags. LC3 has 3 condition flags to indicate the prev calculation. 
enum 
{
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2
};

// Extend the sign to reach 16 bits.
// If positive fill it in with 0s, if negative fill it in with 1s.
uint16_t signExtend(uint16_t x, int bitCount)
{
    if ((x >> (bitCount - 1)) & 1) {
        x |= (0xFFFF << bitCount);
    }
    return x;
}

// Update the flags everytime a value is written to a register.
// Update the condition if its set to a negative, positive or zero.
void updateFlgs(uint16_t r)
{
    if (registers[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (registers[r] >> 15) // if the value is shifted to the left most bit and it is a 1, set the flag to negative.
    {
        reg[R_COND] = FL_NEG;
    }
    else 
    {
        reg[R_COND] = FL_POS;
    }
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
        if (!read_image(argv[j]))
        {
            printf("Failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    // Set the PC to it's start position, 0x3000 by default
    enum { PC_START = 0x3000 };
    registers[R_PC] = PC_START; // Add 0x3000 to the program counter as it's address.

    int running = 1; // is it running?
    while (running)
    {
        // Fetch an instruction from memory to the address of program counter which is now 0x3000.
        // Increment PC
        uint16_t instr = mem_read(registers[R_PC]++)
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
                    uint16_t dr = (instr >> 9) & 0x7;
                    // First no. (SR1)
                    uint16_t r1 = 
                
                }
                break;
            case OP_AND:
                break;
            case OP_NOT:
                break;
            case OP_BR:
                break;
            case OP_JMP:
                break;
            case OP_JSR:
                break;
            case OP_LD:
                break;
            case OP_LDI:
                break;
            case OP_LDR:
                break;
            case OP_LEA:
                break;
            case OP_ST:
                break;
            case OP_STI:
                break;
            case OP_STR:
                break;
            case OP_TRAP:
                break;
            case OP_RES: // reserved (unused)
            case OP_RTI: // unused as well.
            default:
                // return an error
                // BAD_OPCODE
                break;

        }
    }

    return 0;
}