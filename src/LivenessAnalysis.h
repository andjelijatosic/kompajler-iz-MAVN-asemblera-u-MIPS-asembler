#pragma once

#include "IR.h"

#include <string>

using namespace std;

class LivenessAnalysis
{
private:
	Instructions& m_instructions;
	string m_functionName;

	//podaci o eventualnoj gresci
	bool m_hasError;
	std::string m_errorMessage;
	
	// graf toka upravljanja
	void buildControlFlowGraph();
	Instruction* findInstructionByLabel(const std::string& label) const;
	void connectInstructions(Instruction* from, Instruction* to);
	bool instructionExists(Instruction* instruction, const std::list<Instruction*>& instructionList) const;
	
	// analiza zivotnog veka
	void calculateInOutSets();
	Variables calculateOutSet(Instruction* instruction) const;
	Variables calculateInSet(Instruction* instruction, const Variables& newOut) const;

	// pomocne funckije za skupove promenljivih
	bool variableExists(Variable* variable, const Variables& variables) const;
	void addVariableUnique(Variables& variables, Variable* variable) const;
	Variables unionVariables(const Variables& first, const Variables& second) const;
	Variables differenceVariables(const Variables& first, const Variables& second) const;
	bool variablesEqual(const Variables& first, const Variables& second) const;
	void clearAnalysisData();
	void reportError(const std::string& message);
	void printVariableSet(const Variables& variables) const;


public:
	LivenessAnalysis(Instructions& instructions, const std::string& functionName);
	bool Do();
	void printError() const;
	void printResults() const;
};