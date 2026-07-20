#pragma once

#include "LexicalAnalysis.h"
#include "IR.h"

#include <list>
#include <string>

using namespace std;

class SyntaxAnalysis {
private:
	TokenList& m_tokenList;		//referenca da se ne bi pravila kopija cele liste tokena
	TokenList::iterator m_tokenIterator;	//pokazuje na sledeci token koji treba procitati
	Token m_currentToken;		//token koji se trenutno analizira

	// podaci o sintaksnoj gresci
	bool m_hasSyntaxError;
	Token m_errorToken;
	TokenType m_expectedToken;
	string m_errorMessage;

	Variables m_variables;			// sadrzi sve memorijske i registarske promenljive
	Instructions m_instructions;	// sadrzi sve MAVN instrukcije

	string m_functionName;			// deklarisana pomocu _func main;

	list<string> m_labels;			//lista svih labela u programu
	list<string> m_branchTargets;	//lista svih labela koje se koriste kao odredista instrukcija grananja
									//na kraju se poredi sa m_labels kako bi se proverilo da li svaka upotrebljena kabela zaista postoji
	string m_pendingLabel;			//labela koja se pridruzuje sledecoj instrukciji, npr kod loop -> add
	int m_variablePosition;
	int m_instructionPosition;

	/*
	 * Gramatika MAVN jezika:
	 *
	 * Q -> S ; L
	 *
	 * L -> Q
	 * L -> eof
	 *
	 * S -> _mem mid num
	 * S -> _reg rid
	 * S -> _func id
	 * S -> id : E
	 * S -> E
	 *
	 * E -> add  rid, rid, rid
	 * E -> addi rid, rid, num
	 * E -> sub  rid, rid, rid
	 * E -> la   rid, mid
	 * E -> li   rid, num
	 * E -> lw   rid, num(rid)
	 * E -> sw   rid, num(rid)
	 * E -> b    id
	 * E -> bltz rid, id
	 * E -> nop
	 *
	 * Dodatne instrukcije:
	 *
	 * E -> and rid, rid, rid
	 * E -> or  rid, rid, rid
	 * E -> beq rid, rid, id
	 */

	void Q();
	void S();
	void L();
	void E();

	// rad sa tokenima
	Token getNextToken();
	void eat(TokenType expectedType);
	bool isInstructionStart(TokenType type) const;
	string tokenTypeToString(TokenType type) const;
	void reportSyntaxError(
		TokenType expectedType,
		const string& message
	);
	void reportSyntaxError(const string& message);

	// deklaracija
	void parseMemoryDeclaration();
	void parseRegisterDeclaration();
	void parseFunctionDeclaration();
	void parseLabelledInstruction();

	// analiza instrukcija
	void parseThreeRegisterInstruction(
		TokenType tokenType,
		InstructionType instructionType
	);
	void parseAddImmediate();
	void parseLoadAddress();
	void parseLoadImmediate();
	void parseLoadWord();
	void parseStoreWord();
	void parseBranch();
	void parseBranchLessThanZero();
	void parseBranchEqual();
	void parseNop();

	// rad sa promenljivima i instrukcijama
	Variable* findVariable(const std::string& name) const;
	Variable* declareVariable(
		const std::string& name,
		Variable::VariableType type,
		int value = 0
	);
	Variable* requireVariable(
		const std::string& name,
		Variable::VariableType expectedType
	);
	Instruction* createInstruction(
		InstructionType type,
		const Variables& dst,
		const Variables& src,
		bool hasImmediate = false,
		int immediate = 0,
		const std::string& branchTarget = ""
	);
	bool labelExists(const std::string& label) const;
	void validateBranchTargets();
	void clearIntermediateRepresentation();

public:
	explicit SyntaxAnalysis(TokenList& tokenList);
	~SyntaxAnalysis();
	bool Do();
	void printSyntaxError() const;
	Variables& getVariables();
	Instructions& getInstructions();
	const string& getFunctionName() const;
	const list<string>& getLabels() const;
};
