#include "SpillCode.h"

#include <climits>
#include <iostream>
#include <stdexcept>

using namespace std;

/*
 * Pozicije originalnih promenljivih nisu bitne za alokaciju,
 * ali generisanim promenljivama dodeljujemo jedinstvene pozicije.
 */
namespace
{
	int generatedVariablePosition = 100000;
}

SpillCode::SpillCode(Variables& variables, Instructions& instructions, InterferenceGraph& interferenceGraph): 
	m_variables(variables), m_instructions(instructions), m_interferenceGraph(interferenceGraph), m_spillNumber(0), m_temporaryNumber(0), m_hasError(false), m_errorMessage(""){}

bool SpillCode::Do()
{
	m_hasError = false;
	m_errorMessage.clear();
	try
	{
		Variable* spilledVariable = selectSpillCandidate();
		if (spilledVariable == nullptr)
		{
			reportError("Nije pronadjena promenljiva pogodna za prelivanje.");
		}
		string spilledVariableName = spilledVariable->getName();
		Variable* spillMemory = createSpillMemory(spilledVariable);
		transformInstructions(spilledVariable, spillMemory);
		cout << "Izvrseno je prelivanje promenljive '" << spilledVariableName << "' u memorijsku promenljivu '" << spillMemory->getName() << "'." << endl;
		return true;
	}
	catch (const exception& e)
	{
		if (!m_hasError)
		{
			m_hasError = true;
			m_errorMessage = string("Neocekivana greska tokom prelivanja: ") + e.what();
		}
		return false;
	}
}

Variable* SpillCode::selectSpillCandidate() const
{
	const Variables& registerVariables = m_interferenceGraph.getRegisterVariables();
	Variable* selectedVariable = nullptr;
	int selectedOccurrences = INT_MAX;
	int selectedDegree = -1;
	for (Variables::const_iterator iterator = registerVariables.begin(); iterator != registerVariables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable == nullptr)
		{
			continue;
		}
		int degree =m_interferenceGraph.getDegree(variable);
		if (degree < __REG_NUMBER__)
		{
			continue;
		}
		int occurrences = countOccurrences(variable);
		if (occurrences < selectedOccurrences || (occurrences == selectedOccurrences && degree > selectedDegree))
		{
			selectedVariable = variable;
			selectedOccurrences = occurrences;
			selectedDegree = degree;
		}
	}
	if (selectedVariable == nullptr)
	{
		for (Variables::const_iterator iterator = registerVariables.begin(); iterator != registerVariables.end(); ++iterator)
		{
			Variable* variable = *iterator;
			if (variable == nullptr)
			{
				continue;
			}
			int degree = m_interferenceGraph.getDegree(variable);
			if (degree > selectedDegree)
			{
				selectedVariable = variable;
				selectedDegree = degree;
			}
		}
	}
	return selectedVariable;
}

int SpillCode::countOccurrences(Variable* variable) const
{
	if (variable == nullptr)
	{
		return 0;
	}
	int count = 0;
	for (Instructions::const_iterator instructionIterator = m_instructions.begin(); instructionIterator != m_instructions.end(); ++instructionIterator)
	{
		Instruction* instruction = *instructionIterator;
		if (instruction == nullptr)
		{
			continue;
		}
		const Variables& destinationVariables = instruction->getDst();
		for (Variables::const_iterator iterator = destinationVariables.begin(); iterator != destinationVariables.end(); ++iterator)
		{
			if (*iterator == variable)
			{
				++count;
			}
		}
		const Variables& sourceVariables = instruction->getSrc();
		for (Variables::const_iterator iterator = sourceVariables.begin(); iterator != sourceVariables.end(); ++iterator)
		{
			if (*iterator == variable)
			{
				++count;
			}
		}
	}
	return count;
}

Variable* SpillCode::createSpillMemory(Variable* spilledVariable)
{
	if (spilledVariable == nullptr)
	{
		reportError("Pokusaj pravljenja spill memorije za nullptr promenljivu.");
	}
	string spillName;
	bool nameExists;
	do
	{
		spillName = "spill_" + spilledVariable->getName() + "_" + to_string(m_spillNumber); ++m_spillNumber;
		nameExists = false;
		for (Variables::const_iterator iterator = m_variables.begin(); iterator != m_variables.end(); ++iterator)
		{
			Variable* variable = *iterator;
			if (variable != nullptr && variable->getName() == spillName)
			{
				nameExists = true;
				break;
			}
		}
	} while (nameExists);
	Variable* spillMemory = new Variable(spillName,generatedVariablePosition++, Variable::MEM_VAR, 0);
	m_variables.push_back(spillMemory);
	return spillMemory;
}

Variable* SpillCode::createTemporaryRegister(const string& purpose)
{
	string temporaryName;
	bool nameExists;
	do
	{
		temporaryName = "__spill_tmp_" + purpose + "_" + to_string(m_temporaryNumber);
		++m_temporaryNumber;
		nameExists = false;
		for (Variables::const_iterator iterator = m_variables.begin(); iterator != m_variables.end(); ++iterator)
		{
			Variable* variable = *iterator;
			if (variable != nullptr && variable->getName() == temporaryName)
			{
				nameExists = true;
				break;
			}
		}
	} while (nameExists);
	Variable* temporary = new Variable(temporaryName, generatedVariablePosition++, Variable::REG_VAR, 0);
	m_variables.push_back(temporary);
	return temporary;
}

void SpillCode::transformInstructions(Variable* spilledVariable, Variable* spillMemory)
{
	if (spilledVariable == nullptr || spillMemory == nullptr)
	{
		reportError("Neispravni parametri transformacije prelivanja.");
	}
	Instructions transformedInstructions;
	bool variableWasFound = false;
	for (Instructions::iterator iterator = m_instructions.begin(); iterator != m_instructions.end(); ++iterator)
	{
		Instruction* instruction = *iterator;

		if (instruction == nullptr)
		{
			continue;
		}
		bool isUsed =variableExists(spilledVariable, instruction->getSrc());
		bool isDefined = variableExists(spilledVariable, instruction->getDst());

		if (!isUsed && !isDefined)
		{
			transformedInstructions.push_back(instruction);
			continue;
		}
		variableWasFound = true;
		Variable* temporary = createTemporaryRegister(spilledVariable->getName());
		if (isUsed)
		{
			Instruction* spillLoad = createSpillLoad(temporary, spillMemory);
			if (!instruction->getLabel().empty())
			{
				spillLoad->setLabel(instruction->getLabel());
				instruction->setLabel("");
			}
			transformedInstructions.push_back(spillLoad);
			replaceVariable(instruction->getSrc(), spilledVariable, temporary);
		}

		if (isDefined)
		{
			replaceVariable(instruction->getDst(), spilledVariable, temporary);
		}
		rebuildUseDef(instruction);
		transformedInstructions.push_back(instruction);
		if (isDefined)
		{
			Instruction* spillStore = createSpillStore(temporary, spillMemory);
			transformedInstructions.push_back(spillStore);
		}
	}
	if (!variableWasFound)
	{
		reportError("Izabrana promenljiva se ne koristi ni u jednoj instrukciji.");
	}
	m_instructions.swap(transformedInstructions);
	for (Variables::iterator iterator = m_variables.begin(); iterator != m_variables.end(); ++iterator)
	{
		if (*iterator == spilledVariable)
		{
			m_variables.erase(iterator);
			break;
		}
	}
	delete spilledVariable;
	renumberInstructions();
}

Instruction* SpillCode::createSpillLoad(Variable* temporary, Variable* spillMemory)
{
	Variables destinationVariables;
	Variables sourceVariables;
	destinationVariables.push_back(temporary);
	sourceVariables.push_back(spillMemory);
	Instruction* instruction = new Instruction(0, I_LW_SPILL, destinationVariables, sourceVariables);
	rebuildUseDef(instruction);
	return instruction;
}

Instruction* SpillCode::createSpillStore(Variable* temporary, Variable* spillMemory)
{
	Variables destinationVariables;
	Variables sourceVariables;
	sourceVariables.push_back(temporary);
	sourceVariables.push_back(spillMemory);
	Instruction* instruction = new Instruction(0, I_SW_SPILL,destinationVariables,sourceVariables);
	rebuildUseDef(instruction);
	return instruction;
}

bool SpillCode::replaceVariable(Variables& variables, Variable* oldVariable, Variable* newVariable)
{
	bool replaced = false;
	for (Variables::iterator iterator = variables.begin(); iterator != variables.end(); ++iterator)
	{
		if (*iterator == oldVariable)
		{
			*iterator = newVariable;
			replaced = true;
		}
	}
	return replaced;
}

void SpillCode::rebuildUseDef(Instruction* instruction)
{
	if (instruction == nullptr)
	{
		return;
	}
	instruction->getUse().clear();
	instruction->getDef().clear();
	const Variables& destinationVariables = instruction->getDst();
	for (Variables::const_iterator iterator = destinationVariables.begin(); iterator != destinationVariables.end(); ++iterator)
	{
		Variable* variable = *iterator;

		if (variable == nullptr || variable->getType() != Variable::REG_VAR)
		{
			continue;
		}
		if (!variableExists(variable, instruction->getDef()))
		{
			instruction->addDef(variable);
		}
	}
	const Variables& sourceVariables = instruction->getSrc();
	for (Variables::const_iterator iterator = sourceVariables.begin(); iterator != sourceVariables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable == nullptr || variable->getType() != Variable::REG_VAR)
		{
			continue;
		}
		if (!variableExists(variable, instruction->getUse()))
		{
			instruction->addUse(variable);
		}
	}
}

bool SpillCode::variableExists(Variable* variable, const Variables& variables) const
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

void SpillCode::renumberInstructions()
{
	int position = 0;
	for (Instructions::iterator iterator = m_instructions.begin(); iterator != m_instructions.end(); ++iterator)
	{
		Instruction* instruction = *iterator;
		if (instruction == nullptr)
		{
			continue;
		}
		instruction->setPosition(position);
		++position;
	}
}

void SpillCode::reportError(const string& message)
{
	m_hasError = true;
	m_errorMessage = message;
	throw runtime_error(message);
}

void SpillCode::printError() const
{
	if (m_hasError)
	{
		cout << "Greska tokom prelivanja: " << m_errorMessage << endl;
	}
	else
	{
		cout << "Nema gresaka tokom prelivanja." << endl;
	}
}