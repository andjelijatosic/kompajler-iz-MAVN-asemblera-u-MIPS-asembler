#pragma once

#include "IR.h"

#include <ostream>
#include <string>

using namespace std;

/*
 * Generisani fajl sadrzi:
 *
 *     .data
 *     memorijske promenljive
 *
 *     .text
 *     .globl main
 *     MIPS instrukcije
 */
class CodeGenerator
{
private:
	Variables& m_variables;
	Instructions& m_instructions;
	string m_functionName;
	string m_outputFileName;
	bool m_hasError;
	string m_errorMessage;

	//generisanje sekcija
	void generateDataSection(ostream& output);
	void generateTextSection(ostream& output);
	void generateInstruction(ostream& output, Instruction* instruction);

	// pomocne funkcije za operande
	Variable* getVariableAt(const Variables& variables, int index) const;
	string getRegisterName(Variable* variable);
	string registerToString(Regs reg) const;
	void validateRegisterAssignments();
	void validateRegisterVariable(Variable* variable);
	void reportError(const std::string& message);


public:
	CodeGenerator(Variables& variables, Instructions& instructions, const std::string& functionName,const std::string& outputFileName);
	bool Do();
	void printError() const;
	const std::string& getOutputFileName() const;
};
