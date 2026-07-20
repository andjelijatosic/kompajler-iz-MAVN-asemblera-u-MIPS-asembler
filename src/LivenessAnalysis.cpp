#include "LivenessAnalysis.h"

#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;

LivenessAnalysis::LivenessAnalysis(Instructions& instructions, const string& functionName): m_instructions(instructions), m_functionName(functionName), m_hasError(false), m_errorMessage(""){}

// Pokretanje kompletne analize zivotnog veka.
bool LivenessAnalysis::Do()
{
	m_hasError = false;
	m_errorMessage.clear();
	try
	{
		if (m_instructions.empty())
		{
			reportError("Lista instrukcija je prazna.");
		}
		clearAnalysisData();

		//Formiranje SUCC i PRED veza.
		buildControlFlowGraph();
		calculateInOutSets();
		cout << "Analiza zivotnog veka se uspesno zavrsila." << endl;
		return true;
	}
	catch (const exception& e)
	{
		if (!m_hasError)
		{
			m_hasError = true;
			m_errorMessage = string("Neocekivana greska tokom analize zivotnog veka: ") + e.what();
		}
		return false;
	}
}

// graf toka upravljanja
void LivenessAnalysis::buildControlFlowGraph()
{
	vector<Instruction*> instructionVector(m_instructions.begin(), m_instructions.end());
	for (size_t i = 0; i < instructionVector.size(); ++i)
	{
		Instruction* currentInstruction = instructionVector[i];
		if (currentInstruction == nullptr)
		{
			reportError("U listi instrukcija pronadjen je nullptr.");
		}
		Instruction* nextInstruction = nullptr;
		if (i + 1 < instructionVector.size())
		{
			nextInstruction = instructionVector[i + 1];
		}
		InstructionType type = currentInstruction->getType();
		if (type == I_B)
		{
			Instruction* targetInstruction = findInstructionByLabel(currentInstruction->getBranchTarget());
			if (targetInstruction == nullptr)
			{
				reportError("Nije pronadjena ciljna labela '" + currentInstruction->getBranchTarget() +"' za instrukciju b.");
			}
			connectInstructions(currentInstruction, targetInstruction);
		}
		else if (type == I_BLTZ || type == I_BEQ)
		{
			Instruction* targetInstruction = findInstructionByLabel(currentInstruction->getBranchTarget());
			if (targetInstruction == nullptr)
			{
				reportError("Nije pronadjena ciljna labela '" + currentInstruction->getBranchTarget() + "' za uslovnu instrukciju grananja.");
			}
			connectInstructions(currentInstruction, targetInstruction);
			if (nextInstruction != nullptr)
			{
				connectInstructions(currentInstruction, nextInstruction);
			}
		}
		else
		{
			if (nextInstruction != nullptr)
			{
				connectInstructions(currentInstruction, nextInstruction);
			}
		}
	}
}

Instruction* LivenessAnalysis::findInstructionByLabel(const string& label) const
{
	if (label.empty())
	{
		return nullptr;
	}
	if (label == m_functionName && !m_instructions.empty())
	{
		return m_instructions.front();
	}
	for (Instructions::const_iterator iterator = m_instructions.begin(); iterator != m_instructions.end(); ++iterator)
	{
		Instruction* instruction = *iterator;
		if (instruction != nullptr && instruction->getLabel() == label)
		{
			return instruction;
		}
	}
	return nullptr;
}

void LivenessAnalysis::connectInstructions(Instruction* from, Instruction* to)
{
	if (from == nullptr || to == nullptr)
	{
		return;
	}
	if (!instructionExists(to, from->getSucc()))
	{
		from->getSucc().push_back(to);
	}
	if (!instructionExists(from, to->getPred()))
	{
		to->getPred().push_back(from);
	}
}

bool LivenessAnalysis::instructionExists(Instruction* instruction, const list<Instruction*>& instructionList) const
{
	for (list<Instruction*>::const_iterator iterator = instructionList.begin(); iterator != instructionList.end(); ++iterator)
	{
		if (*iterator == instruction)
		{
			return true;
		}
	}
	return false;
}

// racunanje in and out skupova
void LivenessAnalysis::calculateInOutSets()
{
	vector<Instruction*> instructionVector(m_instructions.begin(), m_instructions.end());
	bool changed;
	do
	{
		changed = false;
		for (vector<Instruction*>::reverse_iterator iterator = instructionVector.rbegin(); iterator != instructionVector.rend(); ++iterator)
		{
			Instruction* instruction = *iterator;
			if (instruction == nullptr)
			{
				continue;
			}
			Variables oldIn = instruction->getIn();
			Variables oldOut = instruction->getOut();
			Variables newOut = calculateOutSet(instruction);
			Variables newIn = calculateInSet(instruction, newOut);
			instruction->getOut() = newOut;
			instruction->getIn() = newIn;
			if (!variablesEqual(oldIn, newIn) || !variablesEqual(oldOut, newOut))
			{
				changed = true;
			}
		}
	} while (changed);
}

Variables LivenessAnalysis::calculateOutSet(Instruction* instruction) const
{
	Variables result;
	if (instruction == nullptr)
	{
		return result;
	}
	const list<Instruction*>& successors = instruction->getSucc();
	for (list<Instruction*>::const_iterator iterator = successors.begin(); iterator != successors.end(); ++iterator)
	{
		Instruction* successor = *iterator;
		if (successor == nullptr)
		{
			continue;
		}
		const Variables& successorIn = successor->getIn();
		for (Variables::const_iterator variableIterator = successorIn.begin(); variableIterator != successorIn.end(); ++variableIterator)
		{
			addVariableUnique(result, *variableIterator);
		}
	}
	return result;
}

Variables LivenessAnalysis::calculateInSet(Instruction* instruction, const Variables& newOut) const
{
	Variables result;
	if (instruction == nullptr)
	{
		return result;
	}
	Variables outMinusDef = differenceVariables(newOut, instruction->getDef());
	result = unionVariables(instruction->getUse(), outMinusDef);
	return result;
}


// pomocne funckije za skupove prom
bool LivenessAnalysis::variableExists(Variable* variable, const Variables& variables) const
{
	for (Variables::const_iterator iterator = variables.begin(); iterator != variables.end(); ++iterator)
	{
		if (*iterator == variable)
		{
			return true;
		}
	}
	return false;
}

void LivenessAnalysis::addVariableUnique(Variables& variables, Variable* variable) const
{
	if (variable == nullptr)
	{
		return;
	}
	if (!variableExists(variable, variables))
	{
		variables.push_back(variable);
	}
}

Variables LivenessAnalysis::unionVariables(const Variables& first, const Variables& second) const
{
	Variables result;
	for (Variables::const_iterator iterator = first.begin(); iterator != first.end(); ++iterator)
	{
		addVariableUnique(result, *iterator);
	}
	for (Variables::const_iterator iterator = second.begin(); iterator != second.end(); ++iterator)
	{
		addVariableUnique(result, *iterator);
	}
	return result;
}

Variables LivenessAnalysis::differenceVariables(const Variables& first, const Variables& second) const
{
	Variables result;
	for (Variables::const_iterator iterator = first.begin(); iterator != first.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (!variableExists(variable, second))
		{
			addVariableUnique(result, variable);
		}
	}
	return result;
}

bool LivenessAnalysis::variablesEqual(const Variables& first, const Variables& second) const
{
	if (first.size() != second.size())
	{
		return false;
	}
	for (Variables::const_iterator iterator = first.begin(); iterator != first.end(); ++iterator)
	{
		if (!variableExists(*iterator, second))
		{
			return false;
		}
	}
	return true;
}

void LivenessAnalysis::clearAnalysisData()
{
	for (Instructions::iterator iterator = m_instructions.begin(); iterator != m_instructions.end(); ++iterator)
	{
		Instruction* instruction = *iterator;
		if (instruction == nullptr)
		{
			continue;
		}
		instruction->getSucc().clear();
		instruction->getPred().clear();
		instruction->getIn().clear();
		instruction->getOut().clear();
	}
}

void LivenessAnalysis::reportError(const string& message)
{
	m_hasError = true;
	m_errorMessage = message;
	throw runtime_error(message);
}

//ispis rezultata
void LivenessAnalysis::printVariableSet(const Variables& variables) const
{
	cout << "{ ";
	bool firstVariable = true;
	for (Variables::const_iterator iterator = variables.begin(); iterator != variables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable == nullptr)
		{
			continue;
		}
		if (!firstVariable)
		{
			cout << ", ";
		}
		cout << variable->getName();
		firstVariable = false;
	}
	cout << " }";
}

void LivenessAnalysis::printError() const
{
	if (m_hasError)
	{
		cout << "Greska u analizi zivotnog veka: " << m_errorMessage << endl;
	}
	else
	{
		cout << "Nema gresaka u analizi zivotnog veka." << endl;
	}
}

void LivenessAnalysis::printResults() const
{
	cout << endl;
	cout << "REZULTATI ANALIZE ZIVOTNOG VEKA" << endl;
	cout << endl;

	for (Instructions::const_iterator iterator = m_instructions.begin(); iterator != m_instructions.end(); ++iterator)
	{
		Instruction* instruction = *iterator;
		if (instruction == nullptr)
		{
			continue;
		}
		cout << endl;
		cout<< "Instrukcija " << instruction->getPosition() + 1 << ":";
		if (!instruction->getLabel().empty())
		{
			cout << " labela = " << instruction->getLabel();
		}
		cout << endl;

		cout << "USE  = ";
		printVariableSet(instruction->getUse());
		cout << endl;

		cout << "DEF  = ";
		printVariableSet(instruction->getDef());
		cout << endl;

		cout << "IN   = ";
		printVariableSet(instruction->getIn());
		cout << endl;

		cout << "OUT  = ";
		printVariableSet(instruction->getOut());
		cout << endl;

		cout << "SUCC = { ";
		bool firstSuccessor = true;
		const list<Instruction*>& successors = instruction->getSucc();
		for (list<Instruction*>::const_iterator succIterator = successors.begin(); succIterator != successors.end(); ++succIterator)
		{
			Instruction* successor = *succIterator;
			if (successor == nullptr)
			{
				continue;
			}
			if (!firstSuccessor)
			{
				cout << ", ";
			}
			cout << successor->getPosition() + 1;
			firstSuccessor = false;
		}
		cout << " }" << endl;
		cout << "PRED = { ";
		bool firstPredecessor = true;
		const list<Instruction*>& predecessors = instruction->getPred();
		for (list<Instruction*>::const_iterator predIterator = predecessors.begin(); predIterator != predecessors.end(); ++predIterator)
		{
			Instruction* predecessor = *predIterator;
			if (predecessor == nullptr)
			{
				continue;
			}
			if (!firstPredecessor)
			{
				cout << ", ";
			}
			cout << predecessor->getPosition() + 1;
			firstPredecessor = false;
		}
		cout << " }" << endl;
	}
	cout << endl;
}