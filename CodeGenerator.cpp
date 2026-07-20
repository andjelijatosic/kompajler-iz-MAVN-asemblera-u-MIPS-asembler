#include "CodeGenerator.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;

CodeGenerator::CodeGenerator(Variables& variables, Instructions& instructions, const string& functionName, const string& outputFileName):
	m_variables(variables), m_instructions(instructions), m_functionName(functionName), m_outputFileName(outputFileName), m_hasError(false), m_errorMessage(""){}

bool CodeGenerator::Do()
{
	m_hasError = false;
	m_errorMessage.clear();
	try
	{
		if (m_outputFileName.empty())
		{
			reportError("Ime izlaznog fajla nije zadato.");
		}
		if (m_functionName.empty())
		{
			reportError("Ime funkcije nije zadato.");
		}
		if (m_instructions.empty())
		{
			reportError("Lista instrukcija je prazna.");
		}

		validateRegisterAssignments();
		ofstream outputFile(m_outputFileName.c_str());
		if (!outputFile.is_open())
		{
			reportError("Nije moguce otvoriti izlazni fajl '" + m_outputFileName + "'.");
		}
		generateDataSection(outputFile);
		generateTextSection(outputFile);
		outputFile.close();
		if (outputFile.fail())
		{
			reportError("Doslo je do greske prilikom zatvaranja izlaznog fajla.");
		}
		cout << "MIPS kod je uspesno generisan u fajlu: " << m_outputFileName << endl;
		return true;
	}
	catch (const exception& e)
	{
		if (!m_hasError)
		{
			m_hasError = true;
			m_errorMessage = string("Neocekivana greska tokom generisanja koda: ") + e.what();
		}
		return false;
	}
}

// generisanje .data sekcije
void CodeGenerator::generateDataSection(
	ostream& output
)
{
	output << ".data" << endl;
	output << endl;
	for (Variables::iterator iterator = m_variables.begin(); iterator != m_variables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable == nullptr)
		{
			continue;
		}
		if (variable->getType() == Variable::MEM_VAR)
		{
			output << variable->getName() << ": .word " << variable->getValue() << endl;
		}
	}
	output << endl;
}

// generisanje .text sekcije
void CodeGenerator::generateTextSection(
	ostream& output
)
{
	output << ".text" << endl;
	output << ".globl " << m_functionName << endl;
	output << endl;

	// Ime funkcije je ulazna labela
	output << m_functionName << ":" << endl;
	for (Instructions::iterator iterator = m_instructions.begin(); iterator != m_instructions.end(); ++iterator)
	{
		Instruction* instruction = *iterator;
		if (instruction == nullptr)
		{
			continue;
		}
		if (!instruction->getLabel().empty())
		{
			output << instruction->getLabel() << ":" << endl;
		}
		generateInstruction(output, instruction);
	}
}

void CodeGenerator::generateInstruction(ostream& output, Instruction* instruction)
{
	if (instruction == nullptr)
	{
		reportError("Pokusaj generisanja nullptr instrukcije.");
	}
	const Variables& destinationVariables = instruction->getDst();
	const Variables& sourceVariables = instruction->getSrc();
	Variable* destination = nullptr;
	Variable* source1 = nullptr;
	Variable* source2 = nullptr;
	switch (instruction->getType())
	{
	case I_ADD:
		destination = getVariableAt(destinationVariables, 0);
		source1 = getVariableAt(sourceVariables, 0);
		source2 = getVariableAt(sourceVariables, 1);
		output << "\tadd " << getRegisterName(destination) << ", " << getRegisterName(source1) << ", " << getRegisterName(source2) << endl;
		break;
	case I_ADDI:
		destination = getVariableAt(destinationVariables, 0); 
		source1 = getVariableAt(sourceVariables, 0);
		output << "\taddi " << getRegisterName(destination) << ", " << getRegisterName(source1) << ", " << instruction->getImmediate() << endl;
		break;
	case I_SUB:
		destination = getVariableAt(destinationVariables, 0);
		source1 = getVariableAt(sourceVariables, 0);
		source2 = getVariableAt(sourceVariables, 1);
		output << "\tsub " << getRegisterName(destination) << ", " << getRegisterName(source1) << ", " << getRegisterName(source2) << endl;
		break;
	case I_LA:
		destination = getVariableAt(destinationVariables, 0);
		source1 = getVariableAt(sourceVariables, 0);
		if (source1 == nullptr || source1->getType() != Variable::MEM_VAR)
		{
			reportError("Instrukcija la nema validnu memorijsku promenljivu.");
		}
		output << "\tla " << getRegisterName(destination) << ", " << source1->getName() << endl;
		break;
	case I_LI:
		destination = getVariableAt(destinationVariables, 0);
		output << "\tli " << getRegisterName(destination) << ", " << instruction->getImmediate() << endl;
		break;
	case I_LW:
		destination = getVariableAt(destinationVariables, 0);
		source1 = getVariableAt(sourceVariables, 0);
		output << "\tlw " << getRegisterName(destination) << ", " << instruction->getImmediate() << "(" << getRegisterName(source1) << ")" << endl;
		break;
	case I_SW:
		 /*
		 * source1 je vrednost koja se upisuje,
		 * source2 je bazni registar adrese.
		 */
		source1 = getVariableAt(sourceVariables, 0); 
		source2 = getVariableAt(sourceVariables, 1);
		output << "\tsw " << getRegisterName(source1) << ", " << instruction->getImmediate() << "(" << getRegisterName(source2) << ")" << endl;
		break;
	case I_B:
		if (instruction->getBranchTarget().empty())
		{
			reportError("Instrukcija b nema ciljnu labelu.");
		}
		output << "\tb " << instruction->getBranchTarget() << endl;
		break;
	case I_BLTZ:
		source1 = getVariableAt(sourceVariables, 0);
		if (instruction->getBranchTarget().empty())
		{
			reportError("Instrukcija bltz nema ciljnu labelu.");
		}
		output << "\tbltz " << getRegisterName(source1) << ", " << instruction->getBranchTarget() << endl;
		break;
	case I_NOP:
		output << "\tnop" << endl;
		break;

	//dodate instrukcije
	case I_AND:
		destination = getVariableAt(destinationVariables, 0);
		source1 = getVariableAt(sourceVariables, 0);
		source2 = getVariableAt(sourceVariables, 1);
		output << "\tand " << getRegisterName(destination) << ", " << getRegisterName(source1) << ", " << getRegisterName(source2) << endl;
		break;
	case I_OR:
		destination = getVariableAt(destinationVariables, 0);
		source1 = getVariableAt(sourceVariables, 0);
		source2 = getVariableAt(sourceVariables, 1);
		output << "\tor " << getRegisterName(destination) << ", " << getRegisterName(source1) << ", " << getRegisterName(source2) << endl;
		break;
	case I_BEQ:
		source1 = getVariableAt(sourceVariables, 0);
		source2 =getVariableAt(sourceVariables, 1);
		if (instruction->getBranchTarget().empty())
		{
			reportError("Instrukcija beq nema ciljnu labelu.");
		}
		output << "\tbeq " << getRegisterName(source1) << ", " << getRegisterName(source2) << ", " << instruction->getBranchTarget() << endl;
		break;
	case I_LW_SPILL:
		destination = getVariableAt(destinationVariables, 0);
		source1 = getVariableAt(sourceVariables, 0);
		if (source1 == nullptr || source1->getType() != Variable::MEM_VAR) 
		{
			reportError("I_LW_SPILL nema validnu memorijsku promenljivu.");
		}
		output << "\tlw " << getRegisterName(destination) << ", " << source1 -> getName() << endl;
		break;
	case I_SW_SPILL:
		source1 = getVariableAt(sourceVariables, 0);
		source2 = getVariableAt(sourceVariables, 1);
		if (source2 == nullptr || source2->getType() != Variable::MEM_VAR)
		{
			reportError("I_SW_SPILL nema validnu memorijsku promenljivu.");
		}
		output << "\tsw " << getRegisterName(source1) << ", " << source2->getName() << endl;
		break;
	case I_NO_TYPE:
	default:
		reportError("Pronadjena je instrukcija nepodrzanog tipa.");
		break;
	}
}

// pomocne funckije
Variable* CodeGenerator::getVariableAt(const Variables& variables, int index) const
{
	if (index < 0)
	{
		return nullptr;
	}
	int currentIndex = 0;
	for (Variables::const_iterator iterator = variables.begin(); iterator != variables.end(); ++iterator)
	{
		if (currentIndex == index)
		{
			return *iterator;
		}
		++currentIndex;
	}
	return nullptr;
}
string CodeGenerator::getRegisterName(Variable* variable)
{
	validateRegisterVariable(variable);
	return registerToString(variable->getAssignment());
}
string CodeGenerator::registerToString(Regs reg) const
{
	switch (reg)
	{
	case t0:
		return "$t0";
	case t1:
		return "$t1";
	case t2:
		return "$t2";
	case t3:
		return "$t3";
	case no_assign:
	default:
		return "";
	}
}
void CodeGenerator::validateRegisterAssignments()
{
	for (Variables::iterator iterator = m_variables.begin(); iterator != m_variables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable == nullptr)
		{
			continue;
		}
		if (variable->getType() == Variable::REG_VAR)
		{
			validateRegisterVariable(variable);
		}
	}
}

void CodeGenerator::validateRegisterVariable(Variable* variable)
{
	if (variable == nullptr)
	{
		reportError("Instrukcija sadrzi nullptr operand.");
	}

	if (variable->getType() != Variable::REG_VAR)
	{
		reportError("Promenljiva '" + variable->getName() + "' nije registarska promenljiva.");
	}

	if (variable->getAssignment() == no_assign)
	{
		reportError("Promenljivoj '" + variable->getName() + "' nije dodeljen fizicki registar.");
	}
}

void CodeGenerator::reportError(const string& message)
{
	m_hasError = true;
	m_errorMessage = message;
	throw runtime_error(message);
}

// ispis
void CodeGenerator::printError() const
{
	if (m_hasError)
	{
		cout << "Greska pri generisanju MIPS koda: " << m_errorMessage << endl;
	}
	else
	{
		cout << "Nema gresaka pri generisanju MIPS koda." << endl;
	}
}
const string&
CodeGenerator::getOutputFileName() const
{
	return m_outputFileName;
}