#pragma once

#include "InterferenceGraph.h"

#include <string>

using namespace std;

/*
 * Klasa SpillCode menja medjureprezentaciju programa
 * kada nije moguce obojiti graf smetnji sa cetiri registra.
 *
 * Za izabranu registarsku promenljivu:
 *
 * 1. pravi memorijsku spill promenljivu,
 * 2. pre svakog citanja ubacuje I_LW_SPILL,
 * 3. posle svakog upisa ubacuje I_SW_SPILL,
 * 4. originalni operand zamenjuje kratkozivecim
 *    privremenim registarskim promenljivama.
 */

class SpillCode
{
private:
	Variables& m_variables;
	Instructions& m_instructions;
	InterferenceGraph& m_interferenceGraph;
	int m_spillNumber;
	int m_temporaryNumber;
	bool m_hasError;
	string m_errorMessage;
	Variable* selectSpillCandidate() const;
	int countOccurrences(Variable* variable) const;
	Variable* createSpillMemory(Variable* spilledVariable);
	Variable* createTemporaryRegister(const std::string& purpose);
	void transformInstructions(Variable* spilledVariable, Variable* spillMemory);
	Instruction* createSpillLoad(Variable* temporary, Variable* spillMemory);
	Instruction* createSpillStore(Variable* temporary, Variable* spillMemory);
	bool replaceVariable(Variables& variables, Variable* oldVariable, Variable* newVariable);
	void rebuildUseDef(Instruction* instruction);
	bool variableExists(Variable* variable, const Variables& variables) const;
	void renumberInstructions();
	void reportError(const std::string& message);

public:
	SpillCode(Variables& variables, Instructions& instructions, InterferenceGraph& interferenceGraph);
	bool Do();
	void printError() const;
};
