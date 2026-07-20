#pragma once

#include "IR.h"

#include <string>
#include <vector>

using namespace std;

 /*
 * Graf se cuva kao matrica smetnji:
 *
 *     0 - promenljive nisu u smetnji
 *     1 - promenljive jesu u smetnji
 */
class InterferenceGraph
{
private:
	Variables& m_variables;
	Instructions& m_instructions;
	Variables m_registerVariables;
	vector<vector<int>> m_interferenceMatrix;
	bool m_hasError;
	string m_errorMessage;

	// formiranje grafa smetnji
	void collectRegisterVariables();
	void initializeMatrix();
	void buildGraph();
	void addInterference(Variable* first, Variable* second);
	int findVariableIndex(Variable* variable) const;
	void clearGraph();
	void reportError(const std::string& message);

public:
	InterferenceGraph(Variables& variables, Instructions& instructions);
	bool Do();
	bool hasInterference(Variable* first, Variable* second) const;
	int getDegree(Variable* variable) const;
	Variables getNeighbors(Variable* variable) const;
	const Variables& getRegisterVariables() const;
	const vector<vector<int>>&getInterferenceMatrix() const;
	void printGraph() const;
	void printError() const;
};