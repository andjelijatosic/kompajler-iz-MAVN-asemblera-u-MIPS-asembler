#pragma once

#include "InterferenceGraph.h"

#include <stack>
#include <string>
#include <vector>

using namespace std;

class ResourceAllocation
{
private:
	InterferenceGraph& m_interferenceGraph;
	stack<Variable*> m_simplificationStack;
	int m_registerCount;
	bool m_hasError;
	bool m_spillDetected;
	string m_errorMessage;

	// faza uproscavanja
	bool simplifyGraph();
	int findNodeToRemove(const vector<bool>& removed, const vector<vector<int>>& matrix) const;
	int calculateReducedDegree(int variableIndex, const vector<bool>& removed, const vector<vector<int>>& matrix) const;

	// faza izbora
	bool selectRegisters();
	Regs getAvailableRegister(Variable* variable) const;
	bool isRegisterUsedByNeighbor(Variable* variable, Regs reg) const;

	// pomocne funckije
	void resetAssignments();
	void clearStack();
	string registerToString(Regs reg) const;
	void reportError(const string& message, bool spill = false);

public:
	explicit ResourceAllocation(InterferenceGraph& interferenceGraph);
	bool Do();
	bool isSpillDetected() const;
	void printAssignments() const;
	void printError() const;
};
