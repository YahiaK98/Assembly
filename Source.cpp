#include <iostream>
#include <iomanip>
#include "stdlib.h"
#include <String>
#include <cmath>
using namespace std;

struct instruction {
	string text;
	unsigned int MachineCode;
	unsigned int rd, rs1, rs2, funct3, funct7, opcode;
	unsigned int I_imm, S_imm, B_imm, U_imm, J_imm;
};

int registers[32] = { 0 };
unsigned int pc = 0x0;
//char memory[8 * 1024];

void Assemble_Inst(instruction& inst) {

	// Generate instruction machine code

	// Executing instructions
	if (inst.opcode == 0x33)
	{
		switch (inst.funct3)
		{
		case 0:
			if (inst.funct7 == 32)
				registers[inst.rd] = registers[inst.rs1] - registers[inst.rs2]; // sub
			else
				registers[inst.rd] = registers[inst.rs1] + registers[inst.rs2]; // add
				break;
		case 1:
				registers[inst.rd] = (registers[inst.rs1]) << (registers[inst.rs2] & 0x0000001F); // SLL (lower 5 bits)
				break;
		case 2:
			if (signed(registers[inst.rs1]) < signed(registers[inst.rs2])) // SLT 
				registers[inst.rd] = 1;
			else
				registers[inst.rd] = 0;
				break;
		case 3:
			if (unsigned(registers[inst.rs1]) < unsigned(registers[inst.rs2])) // SLTU
				registers[inst.rd] = 1;
			else
				registers[inst.rd] = 0;
			break;
		case 4:
				registers[inst.rd] = registers[inst.rs1] ^ registers[inst.rs2]; // XOR
				break;
		case 5:
			if (inst.funct7 == 0)
				registers[inst.rd] = (registers[inst.rs1]) >> (registers[inst.rs2] & 0x0000001F); // SRL
			else
			{	// SRA	
				int signed_bit;
				signed_bit = registers[inst.rs1] & 0x80000000;
				int shamt = registers[inst.rs2] & 0x0000001F;
				if (signed_bit)
					for (int i = 0; i < shamt; i++)
					{
						registers[inst.rd] = registers[inst.rs1] >> 1;
						registers[inst.rd] = registers[inst.rd] | 0x80000000;
					}
				else
					registers[inst.rd] = registers[inst.rs1] >> shamt;
			}
				break;
		case 6:
				registers[inst.rd] = registers[inst.rs1] | registers[inst.rs2]; // OR
				break;
		case 7:
				registers[inst.rd] = registers[inst.rs1] & registers[inst.rs2]; // AND
				break;
		}
	}
	else if (inst.opcode == 0x13)
	{
		switch (inst.funct3)
		{
		case 0: 
				registers[inst.rd] = registers[inst.rs1] + ((inst.funct7 << 5) + inst.rs2); // ADDI
				break;
		case 1:
				registers[inst.rd] = registers[inst.rs1] << inst.rs2; // SLLI
				break;
		case 2:
			if (signed(registers[inst.rs1]) < signed(((inst.funct7 << 5) + inst.rs2))) // SLTI 
				registers[inst.rd] = 1;
			else
				registers[inst.rd] = 0;
				break;
		case 3: 
			if (unsigned(registers[inst.rs1]) < (unsigned((inst.funct7 << 5) + inst.rs2))) // SLTIU
				registers[inst.rd] = 1;
			else
				registers[inst.rd] = 0;
				break;
		case 4:
				registers[inst.rd] = registers[inst.rs1] ^ ((inst.funct7 << 5) + inst.rs2); // XORI
				break;	
		case 5:
			if (inst.funct7 == 0)
				registers[inst.rd] = registers[inst.rs1] >> inst.rs2; // SRLI
			else
			{	// SRAI	
				int signed_bit;
				signed_bit = registers[inst.rs1] & 0x80000000;
				int shamt = inst.rs2;
				if (signed_bit)
					for (int i = 0; i < shamt; i++)
					{
						registers[inst.rd] = registers[inst.rs1] >> 1;
						registers[inst.rd] = registers[inst.rd] | 0x80000000;
					}
				else
					registers[inst.rd] = registers[inst.rs1] >> shamt;
			}
				break;
		case 6:
				registers[inst.rd] = registers[inst.rs1] | ((inst.funct7 << 5) + inst.rs2); // ORI
				break;
		case 7:
				registers[inst.rd] = registers[inst.rs1] & ((inst.funct7 << 5) + inst.rs2); // ANDI
				break;
		}
	}
	else if (inst.opcode == 0x37) // LUI
	{
		int immediate = (inst.funct7 << 5) + inst.rs2;
		immediate = (immediate << 5) + inst.rs1;
		immediate = (immediate << 3) + inst.funct3;
		registers[inst.rd] = immediate << 12;
	}
	else if (inst.opcode == 0x17) // AUIPC
	{
		int immediate = (inst.funct7 << 5) + inst.rs2;
		immediate = (immediate << 5) + inst.rs1;
		immediate = (immediate << 3) + inst.funct3;
		immediate = immediate << 12;
		pc += immediate;
		registers[inst.rd] = immediate;
	}
	else if (inst.opcode == 0x6F) // JAL
	{
		signed int offset = inst.funct7 >> 6; // imm[20]
		offset = (offset << 8) + ((inst.rs1 << 3) + inst.funct3); // imm[19:12]
		offset = (offset << 1) + ((inst.rs2 & 0x1)); // imm[11]
		offset = (offset << 10) + (((inst.funct7 & 0x3F) << 4) + (inst.rs2 >> 1));
		registers[inst.rd] = pc + 4;
		pc += offset;
	}
	else if (inst.opcode == 0x67)
	{
		// JALR
	}
	else if (inst.opcode == 0x63)
	{
		switch (inst.funct3)
		{
		case 0: // BEQ
				break;
		case 1: // BNE
				break;
		case 4: // BLT
				break;
		case 5: // BGE
				break;
		case 6: // BLTU
				break;
		case 7: // BGEU
				break;
		}
	}
	else if (inst.opcode == 0x03)
	{
		switch (inst.funct3)
		{
		case 0: // LB
				break;
		case 1: // LH
				break;
		case 2: // LW
				break;
		case 4: // LBU
				break;
		case 5: // LHU
				break;
		}
	}
	else if (inst.opcode == 0x23)
	{
		switch (inst.funct3)
		{
		case 0: // SB
			break;
		case 1: // SH
			break;
		case 2: // SW
			break;
		}
	}
	else if (inst.opcode == 0x73)
	{
		// ECALL
	}
}


int main() {	
	int x = 0b01001010;
	int y = x & 0x3F;
	cout << y << endl;

	system("pause");
	return 0;
}