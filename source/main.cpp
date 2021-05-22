#include <iostream>
#include "registers.h"

/* registers */
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
    R_PC, /* program counter */
    R_COND,
    R_COUNT
};

/* memory. */
uint16_t memory[UINT16_MAX];
uint16_t reg[R_COUNT]

/* opcodes. */
enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

/* condition flags */
enum
{
    FL_POS = 1 << 0, /* P */
    FL_ZRO = 1 << 1, /* Z */
    FL_NEG = 1 << 2, /* N */
};

void read_image_file(FILE* file)
{
    /* the origin tells us where in memory to place the image */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    /* we know the maximum file size so we only need one fread */
    uint16_t max_read = UINT16_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    /* swap to little endian */
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}

int read_image(const char* image_path)
{
    FILE* file = fopen(image_path, "rb");
    if (!file) { return 0; };
    read_image_file(file);
    fclose(file);
    return 1;
}

// Extend the sign if it doesnt match 16 bits.
// For positive (+) integers fill it with 0 to the left (>>).
// For negative (-) integers fill it with 1 to the left (>>).

// Like:
// 1010 Becomes 0000 0000 0000 1010
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

// Update flags.
// If it is a positive one fill in 0s
// Else if it is a negative one fill in 1s.
void update_flags(uint16_t r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15) /* Place it
    at the left-most bit 2nd to the last (15).*/
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}


int main(int argc, const char* argv[]){
    /* load arguments to load programs. */
    if (argc < 2)
    {
        /* show usage string */
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j)
    {
        if (!read_image(argv[j]))
        {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    // Setup. 
#ifdef _WIN32
	#include "win_setup.h"
	
	signal(SIGINT, Windows::handle_interrupt);
	disable_input_buffering();
#endif // _WIN32
	
#ifdef __unix__
	#include "unix_setup.h"
	
	signal(SIGINT, Unix::handle_interrupt);
	disable_input_buffering();
#endif // __unix__
	
	// set the PC to starting position/address. 
	// default = 0x3000
	enum { PC_START = 0x3000 };
	reg[R_PC] = PC_START
	
	
	// Main loop.
	// Switch statements for the different operations.
	int running = 1; // 1 is true.
	while (running) {
		
		/* fetch. */
		uint16_t instr = mem_read(reg[R_PC]++); // Read each instruction from memory.
		uint16_t op = instr >> 12;
		
		switch (op)
		{
			case OP_ADD:
				{
					// destination register. (DR)
					uint16_t r0 = (instr >> 9) & 0x7;
					// first operand. (SR1)
					
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
			case OP_RES:
			case OP_RTI:
			default:
				// Insert "BAD OPCODE" operation here.
				break;
		}
		
		
	}
	
	

    return 0;
}
