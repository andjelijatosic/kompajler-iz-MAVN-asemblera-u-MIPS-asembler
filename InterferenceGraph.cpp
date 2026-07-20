#include "InterferenceGraph.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace std;

InterferenceGraph::InterferenceGraph(Variables& variables, Instructions& instructions): m_variables(variables), m_instructions(instructions), m_registerVariables(), m_interferenceMatrix(), m_hasError(false), m_errorMessage(""){}

bool InterferenceGraph::Do()
{
	m_hasError = false;
	m_errorMessage.clear();

	try
	{
		clearGraph();
		collectRegisterVariables();
		initializeMatrix();
		buildGraph();
		cout << "Graf smetnji je uspesno formiran." << endl;
		return true;
	}
	catch (const exception& e)
	{
		if (!m_hasError)
		{
			m_hasError = true;
			m_errorMessage = string("Neocekivana greska tokom formiranja grafa smetnji: ") + e.what();
		}
		return false;
	}
}

// formiranje grafa smetnji
void InterferenceGraph::collectRegisterVariables()
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
			m_registerVariables.push_back(variable);
		}
	}
}

void InterferenceGraph::initializeMatrix()
{
	const size_t numberOfVariables = m_registerVariables.size();
	m_interferenceMatrix.assign(numberOfVariables, vector<int>(numberOfVariables, __EMPTY__)
	);
}


/*
 * Za svaku instrukciju posmatraju se
 * njeni DEF i OUT skupovi.
 *
 * Za svaku promenljivu A iz DEF i svaku promenljivu
 * B iz OUT dodaje se smetnja ako A i B nisu iste.
 */
void InterferenceGraph::buildGraph()
{
	for (Instructions::iterator instructionIterator = m_instructions.begin(); instructionIterator != m_instructions.end(); ++instructionIterator)
	{
		Instruction* instruction = *instructionIterator;
		if (instruction == nullptr)
		{
			continue;
		}
		const Variables& defVariables = instruction->getDef();
		const Variables& outVariables = instruction->getOut();
		for (Variables::const_iterator defIterator = defVariables.begin(); defIterator != defVariables.end(); ++defIterator)
		{
			Variable* definedVariable = *defIterator;
			if (definedVariable == nullptr || definedVariable->getType() != Variable::REG_VAR)
			{
				continue;
			}
			for (Variables::const_iterator outIterator = outVariables.begin(); outIterator != outVariables.end(); ++outIterator)
			{
				Variable* liveVariable = *outIterator;
				if (liveVariable == nullptr || liveVariable->getType() != Variable::REG_VAR)
				{
					continue;
				}
				if (definedVariable == liveVariable)
				{
					continue;
				}
				addInterference(definedVariable, liveVariable);
			}
		}
	}
}

void InterferenceGraph::addInterference(Variable* first, Variable* second)
{
	if (first == nullptr || second == nullptr || first == second)
	{
		return;
	}
	int firstIndex = findVariableIndex(first);
	int secondIndex = findVariableIndex(second);
	if (firstIndex < 0 || secondIndex < 0)
	{
		reportError("Pokusaj dodavanja smetnje za promenljivu koja nije cvor grafa.");
	}
	m_interferenceMatrix[firstIndex][secondIndex] = __INTERFERENCE__;
	m_interferenceMatrix[secondIndex][firstIndex] = __INTERFERENCE__;
}

int InterferenceGraph::findVariableIndex(Variable* variable) const
{
	int index = 0;
	for (Variables::const_iterator iterator = m_registerVariables.begin(); iterator != m_registerVariables.end(); ++iterator)
	{
		if (*iterator == variable)
		{
			return index;
		}
		++index;
	}
	return -1;
}

void InterferenceGraph::clearGraph()
{
	m_registerVariables.clear();
	m_interferenceMatrix.clear();
}

void InterferenceGraph::reportError(const string& message)
{
	m_hasError = true;
	m_errorMessage = message;
	throw runtime_error(message);
}

// funkcije za pristup grafu
bool InterferenceGraph::hasInterference(Variable* first, Variable* second) const
{
	int firstIndex = findVariableIndex(first);
	int secondIndex = findVariableIndex(second);
	if (firstIndex < 0 || secondIndex < 0)
	{
		return false;
	}

	return m_interferenceMatrix[firstIndex][secondIndex] == __INTERFERENCE__;
}

int InterferenceGraph::getDegree(Variable* variable) const
{
	int variableIndex = findVariableIndex(variable);
	if (variableIndex < 0)
	{
		return -1;
	}
	int degree = 0;
	for (size_t column = 0; column < m_interferenceMatrix[variableIndex].size(); ++column)
	{
		if (m_interferenceMatrix[variableIndex][column] == __INTERFERENCE__)
		{
			++degree;
		}
	}

	return degree;
}

Variables InterferenceGraph::getNeighbors(Variable* variable) const
{
	Variables neighbors;
	int variableIndex = findVariableIndex(variable);
	if (variableIndex < 0)
	{
		return neighbors;
	}
	int currentIndex = 0;
	for (Variables::const_iterator iterator = m_registerVariables.begin(); iterator != m_registerVariables.end(); ++iterator)
	{
		if (m_interferenceMatrix[variableIndex][currentIndex] == __INTERFERENCE__)
		{
			neighbors.push_back(*iterator);
		}
		++currentIndex;
	}
	return neighbors;
}

const Variables&InterferenceGraph::getRegisterVariables() const
{
	return m_registerVariables;
}

const vector<vector<int>>&InterferenceGraph::getInterferenceMatrix() const
{
	return m_interferenceMatrix;
}


//ispis
void InterferenceGraph::printGraph() const
{
	cout << endl;
	cout << "MATRICA SMETNJI" << endl;
	cout << endl;

	if (m_registerVariables.empty())
	{
		cout << "Nema registarskih promenljivih." << endl;
		return;
	}
	cout << setw(8) << " ";
	for (Variables::const_iterator iterator = m_registerVariables.begin(); iterator != m_registerVariables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable != nullptr)
		{
			cout << setw(6) << variable->getName();
		}
	}
	cout << endl;
	// redovi
	int rowIndex = 0;
	for (Variables::const_iterator rowIterator = m_registerVariables.begin(); rowIterator != m_registerVariables.end(); ++rowIterator)
	{
		Variable* rowVariable = *rowIterator;
		if (rowVariable == nullptr)
		{
			++rowIndex;
			continue;
		}
		cout << setw(8) << rowVariable->getName();
		for (size_t columnIndex = 0; columnIndex < m_interferenceMatrix[rowIndex].size(); ++columnIndex)
		{
			cout << setw(6) << m_interferenceMatrix[rowIndex][columnIndex];
		}
		cout << endl;
		++rowIndex;
	}
	cout << endl;
	cout << "RANGOVI CVOROVA" << endl;
	cout << endl;
	for (Variables::const_iterator iterator = m_registerVariables.begin(); iterator != m_registerVariables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable == nullptr)
		{
			continue;
		}
		cout << variable->getName() << " -> " << getDegree(variable) << endl;
	}
	cout << endl;
}

void InterferenceGraph::printError() const
{
	if (m_hasError)
	{
		cout << "Greska pri formiranju grafa smetnji: " << m_errorMessage << endl;
	}
	else
	{
		cout << "Nema gresaka pri formiranju grafa smetnji." << endl;
	}
}