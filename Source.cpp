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
#include <conio.h>
using namespace std;
#pragma endregion

#pragma region Types
struct instruction
{
	string text;
	unsigned int MachineCode;
	unsigned int rd, rs1, rs2, funct3, funct7, opcode; 												   
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
vector<string> OutputStrings;
int ErrorCode = -1;
#pragma endregion

#pragma region Definitions
int  GetImm(string temp);
int Assemble(string Text);
int FiveToMips(instruction);

instruction Parse(int MachineCode);

bool readFile(string filename);
bool IsInstruction(string temp);

void OutputOutputs();
void Execute(instruction inst, bool & finished);
void Run();
void Error();
void Output();
void MipsConvert(string);
#pragma endregion

#pragma region ConversionHandlers
unsigned int RiscToMipsRegs[32] =
{
	0,//zero
	31, //ra
	29, //sp
	28, //gp
	1, //at
	26,27, //k0-k1 [reserved]
	8, //t0
	30, //fp
	9,  //t1 
	2,3, //v0-v1
	4,5,6,7, //a0-a3
	10,11, //t2-t3
	16,17,18,19,20,21,22,23,//s0-s7
	12,13,14,15,24,25 //t4-t9
};
#pragma endregion

//EC
// 0 -> file error
// 2 ->Unsupported instruction
// 5 ->not proper termination of program

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
		OutputOutputs();
		MipsConvert(file);
	}
	system("pause");
	return 0;
}

void Error()
{
	//TODO: create global errorstate variable and handle errors throughout functions and switch on error to give feedback to user and exit program
	switch (ErrorCode)
	{
	case 0: 
		cout << "ERROR! Loading file " << endl;
		break; 
	case 2:
		cout << "ERROR! Program contains unsupported instructions (for mips conversion) [Pseudo-instructions]" << endl;
		break; 
	case 5: 
		cout << "WARNING: Program ended unexpectedly, should be ended with an ECALL" << endl;
		break;
	default:
		cout << "ERROR! Unhandled Error Occured" << endl;
		break;
	}
	if (ErrorCode != 5)
	{
		system("pause");
		exit(0);
	}
}

void MipsConvert(string filename)
{
	cout << "Convert To mips module initiated" << endl;
	pc = 0x0; 
	int machine;
	ofstream outputfile; 
	string outfilename = filename.substr(0, filename.size() - 2) + "_Mips.bin";
	outputfile.open(outfilename);
	do
	{

		pc += 4;

		int part1 = (((memory[pc - 4]) << 24) & 0xFF000000);
		int part2 = (((memory[pc - 3]) << 16) & 0x00FF0000);
		int part3 = (((memory[pc - 2]) << 8) & 0x0000FF00);
		int part4 = ((memory[pc - 1]) & 0x000000FF);

		machine = (part1 + part2 + part3 + part4);
			if (machine)
			{
				instruction ToBeExecuted = Parse(machine);
				int Mips_Code = FiveToMips(ToBeExecuted);
				cout << "0x" << hex << setw(8) << setfill('0') << Mips_Code << endl;
				outputfile << "0x" << hex << setw(8) << setfill('0') << Mips_Code << endl;

			}

	} while (machine != 0);
	outputfile.close();

	//TODO: main function to handle all the conversion from riscv to mips.. initializes pc.. loops over all the memory calls Five to mips after parsing instructions
}

bool readFile(string filename)
{
	ofstream OutFile;
	string outfilename; 
	ifstream Infile;
	string temp;
	outfilename = filename.substr(0, filename.size() - 2) + "_Out.bin";

	OutFile.open(outfilename);
	Infile.open(filename);
	if (!Infile.fail())
	{
		label tempL;
		int InstructionNo = 0;
		while (!Infile.eof())
		{
			bool IsLabel = false;
			getline(Infile, temp);
			for (unsigned int i = 0; i<temp.size() && !IsLabel; i++)
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
				cout << "0x" << hex  << setw(8) << setfill('0') << machine << endl;
				OutFile << "0x" << hex << setw(8) << setfill('0') << machine << endl;
				memory[pc] = (machine & 0xFF000000)>>24;
				memory[pc + 1] = (machine & 0x00FF0000)>>16;
				memory[pc + 2] = (machine & 0x0000FF00)>>8;
				memory[pc + 3] = (machine & 0x000000FF);
				pc += 4;
			}
		}
		Infile.close();
		OutFile.close();
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
	int choice = int('Y');

	do 
	{
		if (choice != int('c') && choice != int('C'))
		{
			cout << "Execute next instruction? any key for next, N to terminate, C for continuous: ";
			choice = _getch();
		}

		system("CLS");
		Output();
		
		pc += 4;

		int part1 = (((memory[pc - 4]) << 24) & 0xFF000000);
		int part2 = (((memory[pc - 3]) << 16) & 0x00FF0000);
		int part3 = (((memory[pc - 2]) << 8) & 0x0000FF00);
		int part4 = ((memory[pc - 1]) & 0x000000FF);

		machine = (part1 + part2 + part3 + part4);
		if (choice!=int('N')&& choice != int('n'))
		{
			if (machine)
			{
				instruction ToBeExecuted = Parse(machine);
				Execute(ToBeExecuted, finished);
			}
		}
		else
		{
			finished = true;
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
		if (Operator == get<0>(Memory_S[i]))
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
			Returned += (get<1>(Memory_S[i]) << 12);
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

int FiveToMips(instruction inst)
{
	int signed_bit;
	int immediate;
	unsigned int Returned;
	signed int offset;
	unsigned int opcode; 
	unsigned int rs, rt, rd, SHAMT, funct;
	// Executing instructions
	if (inst.opcode == 0x33)
	{
		opcode = 0;
		rs = RiscToMipsRegs[inst.rs1]; 
		rt = RiscToMipsRegs[inst.rs2];
		rd = RiscToMipsRegs[inst.rd];
		SHAMT = 0;
			switch (inst.funct3)
			{
			case 0:
				if (inst.funct7 == 32)
					funct = 0x22; // SUB
				else
					funct = 0x20; // ADD
				break;
			case 1:
				SHAMT = (registers[inst.rs2] & 0x0000001F);
				funct=0;
				break;
			case 2:
				funct = 0x2a; 					//SLT
				break;
			case 3:
				funct = 0x2b;
			//SLTU
				break;
			case 4:
				funct = 0b100110;
				 // XOR
				break;
			case 5:
				if (inst.funct7 == 0)
				{
					funct = 0b000110; // SRLV
				}
				else
				{	// SRA (implementing what could have been SRAV)	
					unsigned int shamt = registers[inst.rs2] & 0x0000001F;
					SHAMT = shamt; 
					funct = 0b000011;
				}
				break;
			case 6:
				funct = 0b100101; // OR
				break;
			case 7:
				funct = 100100; // AND
				break;
			}
			if (inst.opcode == 0x13)
			{
				rs = 0; //not used in shift R-instructions
				if (inst.funct3 == 1)
				{
					immediate = inst.rs2;
					immediate = immediate & 0x1F;
					SHAMT = immediate; 
					funct = 0;// SLL
				}
				else if (inst.funct3 == 5)
				{
					if (inst.funct7 == 0)
					{
						 SHAMT = inst.rs2 & 0x1F;
						 funct = 0b10;
					}// SRL
					else
					{	// SRA
						funct = 0b000011;
						SHAMT = inst.rs2;
					}
				}
			}
			Returned = ((opcode & 0x3F) << 26) + ((rs & 0x1F) << 21) + ((rt & 0x1F) << 16) + ((rd & 0x1F) << 11) + ((SHAMT & 0x1F) << 6 )+ (funct&0x3F);
	}
	else if (inst.opcode == 0x13)
	{
		rs = RiscToMipsRegs[inst.rs1];
		rt = RiscToMipsRegs[inst.rd];
		immediate = (((inst.funct7 << 5) + inst.rs2)) & 0xFFF;
		signed_bit = immediate >>11; 
		if (signed_bit)
		{
			immediate = immediate | 0xFFFFF000;
		}
			switch (inst.funct3)
			{
			case 0:
				opcode = 0b001000;
				// ADDI
				break;
			case 2:
				opcode = 0b001010;
				//SLTI
				break;
			case 3:
				immediate = immediate & 0xFFF;
				opcode = 0b001011;
				break;
				// SLTIU -- CHECK
			case 4:
				opcode = 0b001110;
				// XORI
			case 6:
				opcode = 0b001101;// ORI
				break;
			case 7:
				 // ANDI
				opcode = 0b001100;
				break;
			}
			immediate = immediate & 0xFFFF;
			Returned = ((opcode & 0x3F) << 26) + ((rs & 0x1F) << 21 )+ ((rt & 0x1F) << 16) + immediate;
	}
	else if (inst.opcode == 0x37) // LUI
	{
		immediate = (inst.MachineCode & 0xFFFF0000); //TODO:took higher 16
	}
	else if (inst.opcode == 0x17) // AUIPC
	{
		//TODO: Assume NO AUIPC in mips
	}
	else if (inst.opcode == 0x6F)
	{
		//JAL
		offset = inst.UJ_imm;
		signed_bit = offset >> 20;
		offset = offset & 0x001FFFFF;

		if (signed_bit)
		{
			offset = offset | 0xFFE00000;
		}
		offset = offset & 0x3FFFFFF;
		opcode = 0b000011;
		//immediate = ((pc + offset)>>2)+(pc&0xF0000000); outside assumed memory 
		//TODO: Assumption
		immediate = ((pc + offset) >> 2);

		Returned = ((opcode & 0x3F) << 26 )+ (immediate & 0x3FFFFFF);
	}
	else if (inst.opcode == 0x67)
	{
		// JALR
		//TODO: Assumed to take Mips' JalR instead of JR

		rs = RiscToMipsRegs[inst.rs1];
		rd = inst.rd;
		opcode = 0;
		rt = 0;
		SHAMT = 0;
		funct = 0b001001;
		Returned = ((opcode & 0x3F) << 26 )+ ((rs & 0x1F) << 21 )+ ((rt & 0x1F) << 16) +((rd & 0x1F) << 11) + ((SHAMT & 0x1F) << 6) + (funct & 0x3F);
	}
	else if (inst.opcode == 0x63)
	{
		rs = RiscToMipsRegs[inst.rs1];
		rt = RiscToMipsRegs[inst.rs2];
		immediate = inst.SB_imm;
		signed_bit = immediate >> 12;
		immediate = immediate & 0x1FFF;
		if (signed_bit)
			immediate = immediate | 0xFFFFF000;
		immediate = immediate & 0xFFFF;

		switch (inst.funct3)
		{
		case 0: // BEQ
			opcode = 0b000100;
			break;
		case 1: // BNE
			opcode = 0b000101;
			break;
		default: 
			ErrorCode = 2;
			Error();
			break;
		}
		immediate = immediate & 0xFFFF;
		Returned = ((opcode & 0x3F) << 26 )+ ((rs & 0x1F) << 21 )+ ((rt & 0x1F) << 16) + immediate;

			//TODO: WE don't handle pseudo mips instructions, even if they are native in risc v.
	}
	else if (inst.opcode == 0x03)
	{
		rs = RiscToMipsRegs[inst.rs1];
		rt = RiscToMipsRegs[inst.rd];
		immediate = (inst.funct7 << 5) + inst.rs2;
		signed_bit = immediate >> 11;
		immediate = immediate & 0xFFF;
		if (signed_bit)
			immediate = immediate | 0xFFFFF000;
		immediate = immediate & 0xFFFF;

		switch (inst.funct3)
		{
		case 0:	// LB 
			opcode = 0b100000;
			break;
		case 1: // LH
			opcode = 0b100001;
			break;
		case 2: // LW
			opcode = 0b100011;
			break;
		case 4: // LBU
			immediate = immediate & 0xFFF;
			opcode = 0b100100;
			break;
		case 5: // LHU
			immediate = immediate & 0xFFF;
			opcode = 0b100101;
			break;
		}
		immediate = immediate & 0xFFFF;
		Returned = ((opcode & 0x3F) << 26) + ((rs & 0x1F) << 21) + ((rt & 0x1F) << 16) + immediate;

	}
	else if (inst.opcode == 0x23)
	{
		rs = RiscToMipsRegs[inst.rs1];
		rt = RiscToMipsRegs[inst.rs2];
		immediate = (inst.funct7 << 5) + inst.rd;
		signed_bit = immediate >> 11;
		immediate = immediate & 0xFFF;
		if (signed_bit)
			immediate = immediate | 0xFFFFF000;
		immediate = immediate & 0xFFFF;
		switch (inst.funct3)
		{
		case 0: // SB
			opcode = 0b101000;
			break;
		case 1: // SH
			opcode = 0b101001;
			break;
		case 2: // SW
			opcode = 0b101011;
			break;
		}
		immediate = immediate & 0xFFFF;
		Returned = ((opcode & 0x3F) << 26) + ((rs & 0x1F) << 21) + ((rt & 0x1F) << 16) + immediate;

	}
	else if (inst.opcode == 0x73)
	{
		// ECALL
		Returned = 0b1100;
	}
	return Returned;
}

instruction Parse(int MachineCode)
{
	instruction Returned;
	unsigned int part1, part2, part3, part4;

	Returned.funct7 = ((MachineCode & 0xfe000000) >> 25);
	Returned.rs2 = ((MachineCode & 0x01f00000) >> 20);
	Returned.rs1 = ((MachineCode & 0x000f8000) >> 15);
	Returned.funct3 = ((MachineCode & 0x00007000) >> 12);
	Returned.rd = ((MachineCode & 0x00000f80) >> 7);
	Returned.opcode = (MachineCode & 0x0000007f);
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
	unsigned int rs1U, rs2U;
	unsigned int part1, part2, part3, part4;

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
					rs1U = registers[inst.rs1];
					rs2U = registers[inst.rs2];
					if (rs1U < rs2U) // SLTU -- CHECK
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
					registers[inst.rd] = registers[inst.rs1];
					if (signed_bit)
						for (unsigned int i = 0; i < shamt; i++)
						{
							registers[inst.rd] = registers[inst.rd] >> 1;
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
				rs1U = (registers[inst.rs1]);
				if (rs1U <immediateU) // SLTIU -- CHECK
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


					registers[inst.rd] = registers[inst.rs1] >> immediate;
				}// SRLI
				else
				{	// SRAI	
					signed_bit = (registers[inst.rs1] & 0x80000000)>>31;
					unsigned int shamt = inst.rs2;
					if (signed_bit)
						for (unsigned int i = 0; i < shamt; i++)
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
			rs1U = registers[inst.rs1]; 
			rs2U = registers[inst.rs2];
			if (rs1U< rs2U)
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
		case 7: // BGEU -- CHECK
			rs1U = registers[inst.rs1];
			rs2U = registers[inst.rs2];
			if (rs1U >= rs2U)
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
		}
	}
	else if (inst.opcode == 0x03)
	{
		switch (inst.funct3)
		{
		case 0:	// LB 
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			offset = offset & 0x00000FFF;

			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;

			address = offset + registers[inst.rs1]; // base + offset
			value = memory[address]; // taking 8 bits
			signed_bit = value >> 7;
			value = value & 0x000000FF;
			if (signed_bit) // sign extension
				value = value | 0xFFFFFF00;

			if (inst.rd != 0)
				registers[inst.rd] = value; // loading into rd
			break;
		case 1: // LH
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			offset = offset & 0x00000FFF;

			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;

			address = offset + registers[inst.rs1]; // base + offset
			unsigned int part1, part2;
			part1 = ((memory[address]&0xff)<<8)&0xff00; // first 8 bits
			part2 =  (memory[address + 1]) & 0xff;
			value = part1 + part2;
			signed_bit = value >> 15;
			value = value & 0x0000FFFF;

			if (signed_bit)
				value = value | 0xFFFF0000;

			if (inst.rd != 0)
				registers[inst.rd] = value;
			break;
		case 2: // LW
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			offset = offset & 0x00000FFF;

			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;

			address = offset + registers[inst.rs1]; // base + offset
			part1 = ((memory[address] & 0xff) << 24) & 0xff000000; // first 8 bits
			part2 = ((memory[address+1] & 0xff) << 16) & 0x00ff0000; 
			part3 = ((memory[address+2] & 0xff) << 8) & 0x0000ff00; 
			part4 = ((memory[address+3] & 0xff)) & 0xff; 
			value = part1 + part2 + part3 + part4;
			if (inst.rd != 0)
				registers[inst.rd] = value;
			break;
		case 4: // LBU
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			offset = offset & 0x00000FFF;

			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;

			address = offset + registers[inst.rs1]; // base + offset
			value = memory[address]; // taking 8 bits
			value = value & 0x000000FF;

			if (inst.rd != 0)
				registers[inst.rd] = value; // loading into rd
			break;
		case 5: // LHU
			offset = (inst.funct7 << 5) + inst.rs2; // 12 bits
			signed_bit = offset >> 11;
			offset = offset & 0x00000FFF;

			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;

			address = offset + registers[inst.rs1]; // base + offset
			part1 = ((memory[address] & 0xff) << 8) & 0xff00; // first 8 bits
			part2 = (memory[address + 1]) & 0xff;
			value = part1 + part2;
			value = value & 0x0000FFFF;

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
			offset = offset & 0x00000FFF;

			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;

			address = offset + registers[inst.rs1];
			memory[address] = (registers[inst.rs2] & 0xFF); // storing lower 8 bits
			break;
		case 1: // SH
			offset = (inst.funct7 << 5) + inst.rd;
			signed_bit = offset >> 11;
			offset = offset & 0x00000FFF;

			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;
			address = offset + registers[inst.rs1];

			memory[address] = (registers[inst.rs2] & 0x0000FF00) >> 8;
			memory[address + 1] = registers[inst.rs2] & 0x000000FF;
			break;

		case 2: // SW
			offset = (inst.funct7 << 5) + inst.rd;
			signed_bit = offset >> 11;
			offset = offset & 0x00000FFF;

			if (signed_bit) // sign extension
				offset = offset | 0xFFFFF000;

			address = offset + registers[inst.rs1];

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
			cout << "OUTPUT: " << registers[11];
			stringstream ss; 
			ss << registers[11];
			OutputStrings.push_back(ss.str());
		}
		else if (registers[10] == 4) // printing a null-terminated string
		{
			string OutputStr = "";
			bool null_reached = false;
			address = registers[11]; // address of null-terminated string
			while (!null_reached)
			{
				char temp = memory[address];
				if (temp == '\0')
					null_reached = true;
				else
					OutputStr += to_string(temp);
				address++;
			}
			cout <<"OUTPUT: "<< OutputStr << endl;
			OutputStrings.push_back(OutputStr);
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
void OutputOutputs()
{
	cout << "Final Output States: " << endl;

	for (size_t i = 0; i < OutputStrings.size(); i++)
	{
		cout << left << setfill('_') << "Output" << setw(2) << i << ":";
		cout << OutputStrings.at(i) << endl;
	}
	int choice = int('Y');
	cout << "Press any key to enter ToMipsConverter: ";
	choice = _getch();
	system("CLS");
}

void Output()
{
	cout << "Registers States Now: " << endl;
	for (unsigned int i = 0; i < 32; i++)
	{
		cout << left << setfill('_') << "x" << setw(2) << i << ":";
		cout << right << dec << setw(8) << registers[i] << endl;
	}
}

bool IsInstruction(string temp)
{
	for (unsigned int i = 0; i < temp.size(); i++)
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