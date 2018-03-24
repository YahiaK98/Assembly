#pragma region Includes
#include <iostream>
#include <iomanip>
#include "stdlib.h"
#include <String>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <vector>
#include <sstream>
#include <tuple>
#include <cstdlib>
using namespace std;
#pragma endregion

#pragma region Types
struct instruction
{
	string text;
	unsigned int MachineCode;
	unsigned int rd, rs1, rs2, funct3, funct7, opcode; //homa dool
													   //Immediates combined/expanded from homa dool
	unsigned int UJ_imm, SB_imm;
};

struct label
{
	string name;
	int location;
};
#pragma endregion

#pragma region Globals
int registers[32] = { 0 };
unsigned int pc = 0x0;
char memory[8 * 1024] = { 0 };
vector<label> Labels;
int ErrorCode = -1;
#pragma endregion

#pragma region Definitions
int  GetImm(string temp);
int Assemble(string Text);
int FiveToMips(instruction);

instruction Parse(int MachineCode);

bool readFile(string filename);
bool IsInstruction(string temp);

void Execute(instruction inst, bool & finished);
void Run();
void Error();
void Output();
void MipsConvert();
#pragma endregion
//EC
// 0 -> file error
//
#pragma region Main
int main() {
	string file;
	getline(cin, file);

	if (!readFile(file))
	{
		ErrorCode = 0;
		Error();
	}
	else
	{
		Run();
		Output();
	}
	system("pause");
	return 0;
}

void Error()
{
	//TODO: create global errorstate variable and handle errors throughout functions and switch on error to give feedback to user and exit program
}

void Output()
{
	//TODO: Output the state of all registers, Memorydata, Machine code [Riscv and Mips] 
	//TODO: Output in files and cmd
}

void MipsConvert()
{
	//TODO: main function to handle all the conversion from riscv to mips.. initializes pc.. loops over all the memory calls Five to mips after parsing instructions
}

bool readFile(string filename)
{
	ifstream Infile;
	string temp;
	Infile.open(filename);
	if (!Infile.fail())
	{
		label tempL;
		int InstructionNo = 0;
		while (!Infile.eof())
		{
			bool IsLabel = false;
			getline(Infile, temp);
			for (int i = 0; i<temp.size() && !IsLabel; i++)
				if (temp.at(i) == ':')
				{
					tempL.location = InstructionNo * 4;
					tempL.name = temp.substr(0, i);
					IsLabel = true;
				}
			if (IsLabel)
				Labels.push_back(tempL);
			else
				InstructionNo++;
		}
		Infile.close();
		Infile.open(filename);
		while (!Infile.eof())
		{
			getline(Infile, temp);
			if (IsInstruction(temp))
			{
				int machine = Assemble(temp);
				memory[pc] = (machine & 0xFF000000)>>24;
				memory[pc + 1] = (machine & 0x00FF0000)>>16;
				memory[pc + 2] = (machine & 0x0000FF00)>>8;
				memory[pc + 3] = (machine & 0x000000FF);
				pc += 4;
			}
		}
		Infile.close();
		pc = 0x0;
		return true;
	}
	else
	{
		cout << "File Failed To load, Exiting.. " << endl;
		return false;
	}
}

void Run()
{
	int machine;
	pc = 0x0;
	bool finished = false;
	do 
	{
		pc += 4;
		
		 int part1 = (((memory[pc - 4]) << 24)&0xFF000000);
		 int part2 = (((memory[pc - 3]) << 16) & 0x00FF0000);
		 int part3 = (((memory[pc - 2]) << 8) & 0x0000FF00);
		 int part4 = ((memory[pc - 1]) & 0x000000FF);

		machine= (part1 + part2 + part3 + part4);
		if (machine)
		{
			instruction ToBeExecuted = Parse(machine);
			Execute(ToBeExecuted, finished);
		}
	} while (machine != 0&&!finished);

	if (!finished)
	{
		ErrorCode = 5; 
		Error();
	}
}
#pragma endregion

#pragma region Core
int Assemble(string Text)
{
	int Returned = 0;
	stringstream stream;
	stream << Text;
	string Operator;
	getline(stream, Operator, '\t');
	bool Assembled = false;

	//R_Format
	//Operand, funct3, funct7
	tuple<string, int, int> R_Format[10] =
	{
		make_tuple("add", 0,0),
		make_tuple("sub", 0,32),
		make_tuple("sll", 1,0),
		make_tuple("slt", 2,0),
		make_tuple("sltu", 3,0),
		make_tuple("xor", 4,0),
		make_tuple("srl", 5,0),
		make_tuple("sra", 5,32),
		make_tuple("or", 6,0),
		make_tuple("and", 7,0)
	};
	for (size_t i = 0; i < 10 && !Assembled; i++)
	{
		if (Operator == get<0>(R_Format[i]))
		{
			Assembled = true;
			string Tokens[3];
			int intTokens[3];
			for (int j = 0; j < 2; j++)
			{
				getline(stream, Tokens[j], ',');
				intTokens[j] = stoi(Tokens[j].substr(1, Tokens[j].size() - 1));
			}
			//Handling error
			stream >> Tokens[2];
			intTokens[2] = stoi(Tokens[2].substr(1, Tokens[2].size() - 1));

			Returned = Returned + 0x33;
			Returned = Returned + ((intTokens[0]) << 7);
			Returned = Returned + ((get<1>(R_Format[i])) << 12);
			Returned = Returned + ((intTokens[1]) << 15);
			Returned = Returned + ((intTokens[2]) << 20);
			Returned = Returned + ((get<2>(R_Format[i])) << 25);
		}
	}

	//MemoryLoad
	tuple <string, int> Memory_L[5] =
	{
		make_tuple("lb", 0),
		make_tuple("lh", 1),
		make_tuple("lw", 2),
		make_tuple("lbu", 4),
		make_tuple("lhu", 5)
	};
	for (size_t i = 0; i < 5 && !Assembled; i++)
	{
		if (Operator == get<0>(Memory_L[i]))
		{
			Assembled = true;
			int rd, rs1;
			int imm;
			string temp;
			getline(stream, temp, ',');
			rd = stoi(temp.substr(1, temp.size() - 1));
			getline(stream, temp, '(');
			imm = GetImm(temp);
			getline(stream, temp, ')');
			rs1 = stoi(temp.substr(1, temp.size() - 1));

			Returned = Returned + 0x3;
			Returned += (rd << 7);
			Returned += (get<1>(Memory_L[i]) << 12);
			Returned += (rs1 << 15);
			Returned += (imm << 20);
		}
	}

	//MemorySave
	tuple <string, int> Memory_S[3] =
	{
		make_tuple("sb", 0),
		make_tuple("sh", 1),
		make_tuple("sw", 2)
	};
	for (size_t i = 0; i < 3 && !Assembled; i++)
	{
		if (Operator == get<0>(Memory_L[i]))
		{
			Assembled = true;
			int rs2, rs1;
			int offset;
			string temp;
			getline(stream, temp, ',');
			rs2 = stoi(temp.substr(1, temp.size() - 1));
			getline(stream, temp, '(');
			offset = GetImm(temp);
			getline(stream, temp, ')');
			rs1 = stoi(temp.substr(1, temp.size() - 1));

			Returned = Returned + 0x23;
			Returned += ((offset & 0x1F) << 7);
			Returned += (get<1>(Memory_L[i]) << 12);
			Returned += (rs1 << 15);
			Returned += (rs2 << 20);
			Returned += ((offset & 0xFE0) << 25);

		}

	}

	//Branch 
	tuple <string, int> Branch_I[6] =
	{
		make_tuple("beq", 0),
		make_tuple("bne", 1),
		make_tuple("blt", 4),
		make_tuple("bge", 5),
		make_tuple("bltu", 6),
		make_tuple("bgeu", 7)
	};
	for (size_t i = 0; i < 6 && !Assembled; i++)
	{
		if (Operator == get<0>(Branch_I[i]))
		{
			Assembled = true;
			string Tokens[2];
			int intTokens[2];
			string LabelName;
			int relativeAddress;
			for (size_t j = 0; j < 2; j++)
			{
				getline(stream, Tokens[j], ',');
				intTokens[j] = stoi(Tokens[j].substr(1, Tokens[j].size() - 1));
			}
			stream >> LabelName;
			bool done = false;
			for (size_t i = 0; i <Labels.size() && !done; i++)
			{
				if (Labels[i].name == LabelName)
				{
					relativeAddress = Labels[i].location - pc;
					done = true;
				}
			}
			if (!done)
			{
				Error();
			}
			else
			{
				Returned += 0x63;
				Returned += (((relativeAddress & 0x1E) + ((relativeAddress & 0x800) >> 11)) << 7);
				Returned += (get<1>(Branch_I[i]) << 12);
				Returned += (intTokens[0] << 15);
				Returned += (intTokens[1] << 20);
				Returned += ((((relativeAddress & 0x7E0) >> 5) + ((relativeAddress & 0x1000) >> 6)) << 25);
			}
		}
	}

	//I_Format
	tuple <string, int> IType_1[6] =
	{
		make_tuple("addi", 0),
		make_tuple("slti", 2),
		make_tuple("sltiu", 3),
		make_tuple("xori", 4),
		make_tuple("ori", 6),
		make_tuple("andi", 7)
	};
	for (size_t i = 0; i < 6 && !Assembled; i++)
	{
		if (Operator == get<0>(IType_1[i]))
		{
			Assembled = true;
			int rd, rs1;
			int imm;
			string temp;
			getline(stream, temp, ',');
			rd = stoi(temp.substr(1, temp.size() - 1));
			getline(stream, temp, ',');
			rs1 = stoi(temp.substr(1, temp.size() - 1));
			stream >> temp;
			imm = GetImm(temp);

			Returned = Returned + 0x13;
			Returned += (rd << 7);
			Returned += (get<1>(IType_1[i]) << 12);
			Returned += (rs1 << 15);
			Returned += (imm << 20);
		}
	}
	//I_format2
	tuple <string, int, int> IType_2[3] =
	{
		make_tuple("slli", 1, 0),
		make_tuple("srli", 5, 0),
		make_tuple("srai", 5, 0b0100000)
	};
	for (size_t i = 0; i < 3 && !Assembled; i++)
	{
		if (Operator == get<0>(IType_2[i]))
		{
			Assembled = true;
			int rd, rs1;
			int shamt;
			string temp;
			getline(stream, temp, ',');
			rd = stoi(temp.substr(1, temp.size() - 1));
			getline(stream, temp, ',');
			rs1 = stoi(temp.substr(1, temp.size() - 1));
			stream >> temp;
			shamt = GetImm(temp);

			Returned = Returned + 0b0010011;
			Returned += (rd << 7);
			Returned += (get<1>(IType_2[i]) << 12);
			Returned += (rs1 << 15);
			Returned += (shamt << 20);
			Returned += (get<2>(IType_2[i]) << 25);
		}
	}

	//Specials
	if (!Assembled)
	{
		Assembled = true;
		if (Operator == "ecall" || Operator == "ECALL")
		{
			Returned = 0x73;
		}
		else if (Operator == "lui")
		{
			int rd, imm;
			string temp;
			getline(stream, temp, ',');
			rd = stoi(temp.substr(1, temp.size() - 1));
			stream >> temp;
			imm = GetImm(temp);
			Returned += 0b0110111;
			Returned += (rd << 7);
			Returned += (imm << 12);

		}
		else if (Operator == "auipc")
		{
			int rd, imm;
			string temp;
			getline(stream, temp, ',');
			rd = stoi(temp.substr(1, temp.size() - 1));
			stream >> temp;
			imm = GetImm(temp);
			Returned += 0b0010111;
			Returned += (rd << 7);
			Returned += (imm << 12);
		}
		else if (Operator == "jal")
		{
			int rd;
			string temp;
			string LabelName;
			int relativeAddress;
			getline(stream, temp, ',');
			rd = stoi(temp.substr(1, temp.size() - 1));

			stream >> LabelName;
			bool done = false;
			for (size_t i = 0; i < Labels.size() && !done; i++)
			{
				if (Labels[i].name == LabelName)
				{
					relativeAddress = Labels[i].location - pc;
					done = true;
				}
			}
			int immToSend = ((relativeAddress & 0x100000) + ((relativeAddress & 0x7FE) << 9) + ((relativeAddress & 0x800) >> 2) + ((relativeAddress & 0xFF000) >> 11)) >> 1;
			Returned += 0b1101111;
			Returned += (rd << 7);
			Returned += (immToSend << 12);
		}
		else if (Operator == "jalr")
		{
			int rd, rs1, imm;
			string temp;
			getline(stream, temp, ',');
			rd = stoi(temp.substr(1, temp.size() - 1));
			getline(stream, temp, ',');
			rs1 = stoi(temp.substr(1, temp.size() - 1));
			stream >> temp;
			imm = GetImm(temp);
			Returned += 0b1100111;
			Returned += (rd << 7);
			Returned += (rs1 << 15);
			Returned += (imm << 20);

		}
	}

	return Returned;
}

int FiveToMips(instruction Parsed)
{
	//TODO: Converts given instruction to Mips format 
	//Similar to Execute function in terms of skeleton
	return 1;
}

instruction Parse(int MachineCode)
{
	//7-5-5-3-5-7
	instruction Returned;
	Returned.funct7 = ((MachineCode & 0xfe000000) >> 25);
	Returned.rs2 = ((MachineCode & 0x01f00000) >> 20);
	Returned.rs1 = ((MachineCode & 0x000f8000) >> 15);
	Returned.funct3 = ((MachineCode & 0x00007000) >> 12);
	Returned.rd = ((MachineCode & 0x00000f80) >> 7);
	Returned.opcode = (MachineCode & 0x0000007f);
	unsigned int part1, part2, part3, part4;
	part1 = (((Returned.funct7) >> 6) << 20)&0x100000;
	part2 = (MachineCode & 0xFF000);
	part3 = (((MachineCode & 0x100000) >> 9)&0x800);
	part4 = (((MachineCode & 0x7FE00000) >> 20) & (0x7FE));
	Returned.UJ_imm = part1 + part2 + part3 + part4;
	Returned.MachineCode = MachineCode;
	part1 = (((MachineCode & 0x80000000) >> 19)&0x1000); 
	part2 = (((MachineCode & 0x80) << 4)&0x800);
	part3 = (((MachineCode & 0x7E000000) >> 20)&0x7E0);
	part4 = (((MachineCode & 0xF00) >> 7)&0x1E);
	Returned.SB_imm = part1 + part2 + part3 + part4;
	return Returned;
}

void Execute(instruction inst, bool & finished)
{
	int signed_bit;
	int immediate;
	unsigned int immediateU;
	signed int offset;
	int value;
	int address;
	// Executing instructions
	if (inst.opcode == 0x33)
	{
		if (inst.rd != 0)
		{
			switch (inst.funct3)
			{
			case 0:
				if (inst.rd != 0)
				{
					if (inst.funct7 == 32)
						registers[inst.rd] = registers[inst.rs1] - registers[inst.rs2]; // SUB
					else
						registers[inst.rd] = registers[inst.rs1] + registers[inst.rs2]; // ADD
				}
				break;
			case 1:
				if (inst.rd != 0)
					registers[inst.rd] = (registers[inst.rs1]) << (registers[inst.rs2] & 0x0000001F); // SLL (lower 5 bits)
				break;
			case 2:
				if (inst.rd != 0)
				{
					int rs1 = (registers[inst.rs1]);
					int rs2 = (registers[inst.rs2]);
					if (rs1 <rs2) // SLT -- CHECK
						registers[inst.rd] = 1;
					else
						registers[inst.rd] = 0;
				}
				break;
			case 3:
				if (inst.rd != 0)
				{
					unsigned int rs1, rs2;
					rs1 = registers[inst.rs1];
					rs2 = registers[inst.rs2];
					if (rs1 < rs2) // SLTU -- CHECK
						registers[inst.rd] = 1;
					else
						registers[inst.rd] = 0;
				}
				break;
			case 4:
				if (inst.rd != 0)
					registers[inst.rd] = registers[inst.rs1] ^ registers[inst.rs2]; // XOR
				break;
			case 5:
				if (inst.funct7 == 0)
					registers[inst.rd] = (registers[inst.rs1]) >> (registers[inst.rs2] & 0x0000001F); // SRL
				else
				{	// SRA	
					signed_bit = registers[inst.rs1] & 0x80000000;
					unsigned int shamt = registers[inst.rs2] & 0x0000001F;
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
	}
	else if (inst.opcode == 0x13)
	{
		if (inst.rd != 0)
		{
			switch (inst.funct3)
			{
			case 0:
				immediate = (((inst.funct7 << 5) + inst.rs2));
				signed_bit = immediate >> 11; 
				immediate = immediate & 0xFFF;
				if (signed_bit)
					immediate = immediate | 0xFFFFF000;

				registers[inst.rd] = registers[inst.rs1] + immediate; // ADDI
				break;
			case 1:
				immediate = inst.rs2;
				signed_bit = immediate >> 4;
				immediate = immediate & 0x1F;
				if (signed_bit)
					immediate = immediate | 0xFFFFFFF0;

				registers[inst.rd] = registers[inst.rs1] << immediate; // SLLI
				break;
			case 2:
				immediate = ((inst.funct7 << 5) + inst.rs2);
				signed_bit = immediate >> 11;
				immediate = immediate & 0xFFF;

				if (signed_bit)
					immediate = immediate | 0xFFFFF000;

				if (registers[inst.rs1] < immediate) // SLTI -- CHECK
					registers[inst.rd] = 1;
				else
					registers[inst.rd] = 0;
				break;
			case 3:
				immediateU = ((inst.funct7 << 5) + inst.rs2)&0xFFF;
				if ((registers[inst.rs1]) <immediateU) // SLTIU -- CHECK
					registers[inst.rd] = 1;
				else
					registers[inst.rd] = 0;
				break;
			case 4:
				immediate = ((inst.funct7 << 5) + inst.rs2);
				signed_bit = immediate >> 11;
				immediate = immediate & 0xFFF;

				if (signed_bit)
					immediate = immediate | 0xFFFFF000;

				registers[inst.rd] = registers[inst.rs1] ^ immediate; // XORI
				break;
			case 5:
				if (inst.funct7 == 0) 
				{
					immediate = inst.rs2 & 0x1F;
					signed_bit = immediate >> 4;

					if (signed_bit)
						immediate = immediate | 0xFFFFFFF0;

					registers[inst.rd] = registers[inst.rs1] >> immediate;
				}// SRLI
				else
				{	// SRAI	
					signed_bit = (registers[inst.rs1] & 0x80000000)>>31;
					unsigned int shamt = inst.rs2;
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
				immediate = (inst.funct7 << 5) + inst.rs2;
				signed_bit = immediate >> 11;
				immediate = immediate & 0xFFF;

				if (signed_bit)
					immediate = immediate | 0xFFFFF000;

				registers[inst.rd] = registers[inst.rs1] | (immediate); // ORI
				break;
			case 7:
				immediate = (inst.funct7 << 5) + inst.rs2;
				signed_bit = immediate >> 11;
				immediate = immediate & 0xFFF;

				if (signed_bit)
					immediate = immediate | 0xFFFFF000;
				registers[inst.rd] = registers[inst.rs1] & (immediate); // ANDI
				break;
			}
		}
	}
	else if (inst.opcode == 0x37) // LUI
	{
		if (inst.rd != 0)
		{
			immediate = (inst.MachineCode & 0xFFFFF000);
			registers[inst.rd] = immediate;
		}
	}
	else if (inst.opcode == 0x17) // AUIPC
	{
		if (inst.rd != 0)
		{
			immediate = (inst.MachineCode & 0xFFFFF000);
			registers[inst.rd] = immediate+pc;
		}
	}
	else if (inst.opcode == 0x6F) // JAL -- CHECK
	{
		offset = inst.UJ_imm;
		signed_bit = offset >> 20; 
		if (signed_bit)
		{
			offset = offset | 0xFFE00000;
		}
		else
		{
			offset = offset & 0x001FFFFF;
		}
		if (inst.rd != 0)
			registers[inst.rd] = pc + 4;
		pc += offset;
		pc -= 4;
	}
	else if (inst.opcode == 0x67)
	{
		// JALR
		immediate = ((inst.funct7 << 5) + inst.rs2) & 0xFFF;
		signed_bit = immediate >> 11;
		immediate = immediate & 0xFFF;
		if (signed_bit)
			immediate = immediate | 0xFFFFF000;
		offset = registers[inst.rs1] + immediate;
		offset = offset & 0xFFFFFFFE;
		if (inst.rd != 0)
			registers[inst.rd] = pc + 4;

		pc = offset; 
		pc -= 4;
	}
	else if (inst.opcode == 0x63)
	{
		switch (inst.funct3)
		{
		case 0: // BEQ
			if (registers[inst.rs1] == registers[inst.rs2])
			{
				immediate = inst.SB_imm;
				signed_bit = immediate >> 12;
				immediate = immediate & 0x1FFF;
				if (signed_bit)
					immediate = immediate | 0xFFFFF000;
				pc += immediate; // CURRENT pc
				pc -= 4;
			}
			break;
		case 1: // BNE
			if (registers[inst.rs1] != registers[inst.rs2])
			{
				immediate = inst.SB_imm;
				signed_bit = immediate >> 12;
				immediate = immediate & 0x1FFF;
				if (signed_bit)
					immediate = immediate | 0xFFFFF000;
				pc += immediate; // CURRENT pc
				pc -= 4;
			}
			break;
		case 4: // BLT
			if (registers[inst.rs1] < registers[inst.rs2])
			{
				immediate = inst.SB_imm;
				signed_bit = immediate >> 12;
				immediate = immediate & 0x1FFF;
				if (signed_bit)
					immediate = immediate | 0xFFFFF000;
				pc += immediate; // CURRENT pc
				pc -= 4;
			}
			break;
		case 5: // BGE
			if (registers[inst.rs1] >= registers[inst.rs2])
			{
				immediate = inst.SB_imm;
				signed_bit = immediate >> 12;
				immediate = immediate & 0x1FFF;
				if (signed_bit)
					immediate = immediate | 0xFFFFF000;
				pc += immediate; // CURRENT pc
				pc -= 4;
			}
			break;
		case 6: // BLTU -- CHECK
			if (registers[inst.rs1] < registers[inst.rs2])
			{
				immediateU = inst.SB_imm;
				pc += immediateU; // CURRENT pc
				pc -= 4;
			}
			break;
		case 7: // BGEU -- CHECK
			if (registers[inst.rs1] >= registers[inst.rs2])
			{
				immediateU = inst.SB_imm;
				pc += immediateU; // CURRENT pc
				pc -= 4;
			}
			break;
		}
	}
	else if (inst.opcode == 0x03)
	{
		switch (inst.funct3)
		{
		case 0:	// LB 
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;
			else
				offset = offset & 0x00000FFF;
			address = offset + registers[inst.rs1]; // base + offset
			value = memory[address]; // taking 8 bits
			signed_bit = value >> 7;
			if (signed_bit) // sign extension
				value = value | 0xFFFFFF00;
			else
				value = value & 0x000000FF;
			if (inst.rd != 0)
				registers[inst.rd] = value; // loading into rd
			break;
		case 1: // LH
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;
			else
				offset = offset & 0x00000FFF;
			address = offset + registers[inst.rs1]; // base + offset
			value = memory[address]; // first 8 bits
			value = (value << 8) + memory[address + 1];
			signed_bit = value >> 15;
			if (signed_bit)
				value = value | 0xFFFF0000;
			else
				value = value & 0x0000FFFF;
			if (inst.rd != 0)
				registers[inst.rd] = value;
			break;
		case 2: // LW
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;
			else
				offset = offset & 0x00000FFF;
			address = offset + registers[inst.rs1]; // base + offset
			value = memory[address]; // first 8 bits
			value = (value << 8) + memory[address + 1];
			value = (value << 8) + memory[address + 2];
			value = (value << 8) + memory[address + 3];
			if (inst.rd != 0)
				registers[inst.rd] = value;
			break;
		case 4: // LBU
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;
			else
				offset = offset & 0x00000FFF;
			address = offset + registers[inst.rs1]; // base + offset
			value = memory[address]; // taking 8 bits
			value = value & 0x000000FF; // zero extension
			if (inst.rd != 0)
				registers[inst.rd] = value; // loading into rd
			break;
		case 5: // LHU
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;
			else
				offset = offset & 0x00000FFF;
			address = offset + registers[inst.rs1]; // base + offset
			value = memory[address]; // first 8 bits
			value = (value << 8) + memory[address + 1];
			value = value & 0x0000FFFF; // zero extension
			if (inst.rd != 0)
				registers[inst.rd] = value;
			break;
		}
	}
	else if (inst.opcode == 0x23)
	{
		switch (inst.funct3)
		{
		case 0: // SB
			offset = (inst.funct7 << 5) + inst.rd;
			signed_bit = offset >> 11;
			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;
			else
				offset = offset & 0x00000FFF;
			address = offset + registers[inst.rs1];
			memory[address] = registers[inst.rs2] & 0x000000FF; // storing lower 8 bits
			break;
		case 1: // SH
			offset = (inst.funct7 << 5) + inst.rd;
			signed_bit = offset >> 11;
			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;
			else
				offset = offset & 0x00000FFF;
			address = offset + registers[inst.rs1];
			/*if (address % 2 == 1) // aligned on multiples of 2
				address += 1;*/
			//address assumed to be hardcoded
			// storing lowest 16 bits
			memory[address] = (registers[inst.rs2] & 0x0000FF00) >> 8;
			memory[address + 1] = registers[inst.rs2] & 0x000000FF;
			break;

		case 2: // SW
			offset = (inst.funct7 << 5) + inst.rd;
			signed_bit = offset >> 11;
			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;
			else
				offset = offset & 0x00000FFF;
			address = offset + registers[inst.rs1];
			/*
			if (address % 4 == 1) // Aligned on multiples of 4
				address += 3;
			else if (address % 4 == 2)
				address += 2;
			else if (address % 4 == 3)
				address += 1;*/
			//address assumed to be hardcoded

			memory[address] = (registers[inst.rs2] & 0xFF000000) >> 24;
			memory[address + 1] = (registers[inst.rs2] & 0x00FF0000) >> 16;
			memory[address + 2] = (registers[inst.rs2] & 0x0000FF00) >> 8;
			memory[address + 3] = (registers[inst.rs2] & 0x000000FF);
			break;
		}
	}
	else if (inst.opcode == 0x73)
	{
		// ECALL, checking x10
		if (registers[10] == 1) // printing integer
		{
			cout << registers[11];
		}
		else if (registers[10] == 4) // printing a null-terminated string
		{
			bool null_reached = false;
			address = registers[11]; // address of null-terminated string
			while (!null_reached)
			{
				char temp = memory[address];
				if (temp == '\0')
					null_reached = true;
				else
					cout << temp;
				address++;
			}
		}
		else if (registers[10] == 5) // reading an integer
		{
			int number;
			cin >> number;
			registers[11] = number;
		}
		else if (registers[10] == 10) // terminate execution
		{
			finished = true;
		}
	}
}
#pragma endregion

#pragma region Helpers
bool IsInstruction(string temp)
{
	for (int i = 0; i < temp.size(); i++)
	{
		if (temp[i] == ':')
			return false;
	}
	return true;
}

int  GetImm(string temp)
{
	int Returned;
	if (temp.substr(0, 2) == "0x")
	{
		char*p;
		Returned = strtol(temp.c_str(), &p, 16);
	}
	else
	{
		Returned = stoi(temp);
	}
	return Returned;


}
#pragma endregion