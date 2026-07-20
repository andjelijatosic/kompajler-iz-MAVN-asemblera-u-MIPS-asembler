#include "ResourceAllocation.h"

#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;

ResourceAllocation::ResourceAllocation(InterferenceGraph& interferenceGraph): m_interferenceGraph(interferenceGraph), m_simplificationStack(), m_registerCount(__REG_NUMBER__), m_hasError(false), m_spillDetected(false), m_errorMessage(""){}

bool ResourceAllocation::Do()
{
	m_hasError = false;
	m_spillDetected = false;
	m_errorMessage.clear();

	try
	{
		clearStack();
		resetAssignments();
		if (!simplifyGraph())
		{
			return false;
		}
		if (!selectRegisters())
		{
			return false;
		}
		cout << "Dodela registara se uspesno zavrsila." << endl;
		return true;
	}
	catch (const exception& e)
	{
		m_hasError = true;
		m_errorMessage = string("Neocekivana greska tokom dodele registara: ") + e.what();
		return false;
	}
}

// faza uproscavanja
bool ResourceAllocation::simplifyGraph()
{
	const Variables& registerVariables = m_interferenceGraph.getRegisterVariables();
	const vector<vector<int>>& matrix = m_interferenceGraph.getInterferenceMatrix();
	const int numberOfVariables = static_cast<int>(registerVariables.size());
	if (matrix.size() != static_cast<size_t>(numberOfVariables))
	{
		reportError("Dimenzija matrice smetnji ne odgovara broju registarskih promenljivih.");
		return false;
	}
	for (vector<vector<int>>::const_iterator iterator = matrix.begin(); iterator != matrix.end(); ++iterator)
	{
		if (iterator->size() != static_cast<size_t>(numberOfVariables))
		{
			reportError("Matrica smetnji nije kvadratna.");
			return false;
		}
	}
	vector<Variable*> variableVector(registerVariables.begin(), registerVariables.end());
	vector<bool> removed(numberOfVariables, false);
	int remainingVariables = numberOfVariables;
	while (remainingVariables > 0)
	{
		int variableIndex = findNodeToRemove(removed, matrix);
		if (variableIndex < 0)
		{
			reportError("Nije moguce dodeliti registre. Detektovano je prelivanje jer graf zahteva vise od cetiri fizicka registra.", true);
			return false;
		}
		Variable* variable = variableVector[variableIndex];

		if (variable == nullptr)
		{
			reportError("U grafu smetnji pronadjena je neispravna registarska promenljiva.");
			return false;
		}
		m_simplificationStack.push(variable);
		removed[variableIndex] = true;
		--remainingVariables;
	}
	return true;
}

int ResourceAllocation::findNodeToRemove(const vector<bool>& removed,const vector<vector<int>>& matrix) const
{
	int selectedIndex = -1;
	int selectedDegree = -1;
	for (int index = 0; index < static_cast<int>(removed.size()); ++index)
	{
		if (removed[index])
		{
			continue;
		}
		int degree = calculateReducedDegree(index, removed, matrix);
		if (degree < m_registerCount && degree > selectedDegree)
		{
			selectedIndex = index;
			selectedDegree = degree;
		}
	}
	return selectedIndex;
}

int ResourceAllocation::calculateReducedDegree(int variableIndex, const vector<bool>& removed, const vector<vector<int>>& matrix) const
{
	if (variableIndex < 0 || variableIndex >= static_cast<int>(matrix.size()))
	{
		return -1;
	}

	int degree = 0;

	for (int neighborIndex = 0; neighborIndex<static_cast<int>(matrix[variableIndex].size()); ++neighborIndex)
	{
		if (neighborIndex == variableIndex)
		{
			continue;
		}
		if (removed[neighborIndex])
		{
			continue;
		}
		if (matrix[variableIndex][neighborIndex] == __INTERFERENCE__)
		{
			++degree;
		}
	}

	return degree;
}

// faza izbora registra

bool ResourceAllocation::selectRegisters()
{
	while (!m_simplificationStack.empty())
	{
		Variable* variable = m_simplificationStack.top();
		m_simplificationStack.pop();
		if (variable == nullptr)
		{
			reportError("Na steku uproscavanja pronadjena je neispravna promenljiva.");
			return false;
		}
		Regs selectedRegister = getAvailableRegister(variable);
		if (selectedRegister == no_assign)
		{
			reportError("Nije pronadjen slobodan fizicki registar za promenljivu '" + variable->getName() + "'. Detektovano je prelivanje.", true);
			return false;
		}
		variable->setAssignment(selectedRegister);
	}
	return true;
}

Regs ResourceAllocation::getAvailableRegister(Variable* variable) const
{
	if (variable == nullptr)
	{
		return no_assign;
	}
	const Regs availableRegisters[] =
	{
		t0,
		t1,
		t2,
		t3
	};
	for (int index = 0; index < m_registerCount; ++index)
	{
		Regs currentRegister = availableRegisters[index];
		if (!isRegisterUsedByNeighbor(variable, currentRegister))
		{
			return currentRegister;
		}
	}
	return no_assign;
}

bool ResourceAllocation::isRegisterUsedByNeighbor(Variable* variable, Regs reg) const
{
	Variables neighbors = m_interferenceGraph.getNeighbors(variable);
	for (Variables::const_iterator iterator = neighbors.begin(); iterator != neighbors.end(); ++iterator)
	{
		Variable* neighbor = *iterator;
		if (neighbor == nullptr)
		{
			continue;
		}
		if (neighbor->getAssignment() == reg)
		{
			return true;
		}
	}
	return false;
}

//pomocne funkcije
void ResourceAllocation::resetAssignments()
{
	const Variables& registerVariables = m_interferenceGraph.getRegisterVariables();
	for (Variables::const_iterator iterator = registerVariables.begin(); iterator != registerVariables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable != nullptr)
		{
			variable->setAssignment(no_assign);
		}
	}
}

void ResourceAllocation::clearStack()
{
	while (!m_simplificationStack.empty())
	{
		m_simplificationStack.pop();
	}
}

string ResourceAllocation::registerToString(Regs reg) const
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
		return "nije dodeljen";
	}
}

void ResourceAllocation::reportError(const string& message, bool spill)
{
	m_hasError = true;
	m_spillDetected = spill;
	m_errorMessage = message;
}

//javne funckije za ispis
bool ResourceAllocation::isSpillDetected() const
{
	return m_spillDetected;
}

void ResourceAllocation::printAssignments() const
{
	cout << endl;
	cout << "DODELA FIZICKIH REGISTARA" << endl;
	cout << endl;

	const Variables& registerVariables = m_interferenceGraph.getRegisterVariables();
	for (Variables::const_iterator iterator = registerVariables.begin(); iterator != registerVariables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable == nullptr)
		{
			continue;
		}
		cout << variable->getName() << " -> " << registerToString(variable->getAssignment())<< endl;
	}
	cout << endl;
}

void ResourceAllocation::printError() const
{
	if (m_hasError)
	{
		cout<< "Greska pri dodeli registara: " << m_errorMessage << endl;
	}
	else
	{
		cout << "Nema gresaka pri dodeli registara." << endl;
	}
}