#include "SyntaxAnalysis.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

SyntaxAnalysis::SyntaxAnalysis(TokenList& tokenList): 
	m_tokenList(tokenList),
	m_tokenIterator(m_tokenList.begin()),
	m_currentToken(),
	m_hasSyntaxError(false),
	m_errorToken(),
	m_expectedToken(T_NO_TYPE),
	m_errorMessage(""),
	m_variables(),
	m_instructions(),
	m_functionName(""),
	m_labels(),
	m_branchTargets(),
	m_pendingLabel(""),
	m_variablePosition(0),
	m_instructionPosition(0){}

SyntaxAnalysis::~SyntaxAnalysis() { clearIntermediateRepresentation(); }

bool SyntaxAnalysis::Do()
{
	clearIntermediateRepresentation();

	m_labels.clear();
	m_branchTargets.clear();
	m_functionName.clear();
	m_pendingLabel.clear();

	m_variablePosition = 0;
	m_instructionPosition = 0;

	m_hasSyntaxError = false;
	m_errorToken = Token();
	m_expectedToken = T_NO_TYPE;
	m_errorMessage.clear();

	m_tokenIterator = m_tokenList.begin();

	try
	{
		if (m_tokenList.empty())
		{
			reportSyntaxError("Lista tokena je prazna.");
		}
		m_currentToken = getNextToken();
		Q();
		if (m_functionName.empty())
		{
			reportSyntaxError("Program nema deklaraciju funkcije oblika _func ime;");
		}
		if (m_instructions.empty())
		{
			reportSyntaxError("Program ne sadrzi nijednu instrukciju.");
		}
		validateBranchTargets();
		cout << "Sintaksna analiza se uspesno zavrsila" << endl;
		return true;
	}
	catch (const exception& e)
	{
		if (!m_hasSyntaxError)
		{
			m_hasSyntaxError = true;
			m_errorToken = m_currentToken;
			m_errorMessage = string("Neocekivana greska tokom sintaksne analize: ") + e.what();
		}
		clearIntermediateRepresentation();
		return false;
	}
}

void SyntaxAnalysis::Q()
{
	S();
	eat(T_SEMI_COL);
	L();
}

void SyntaxAnalysis::S()
{
	switch (m_currentToken.getType())
	{
	case T_MEM:
		parseMemoryDeclaration();
		break;
	case T_REG:
		parseRegisterDeclaration();
		break;
	case T_FUNC:
		parseFunctionDeclaration();
		break;
	case T_ID:
		parseLabelledInstruction();
		break;
	default:
		if (isInstructionStart(m_currentToken.getType()))
		{
			E();
		}
		else
		{
			reportSyntaxError("Token ne moze biti pocetak deklaracije, labele ili instrukcije.");
		}
		break;
	}
}

void SyntaxAnalysis::L()
{
	if (m_currentToken.getType() == T_END_OF_FILE)
	{
		eat(T_END_OF_FILE);
	}else 
	{
		Q();
	}
}

void SyntaxAnalysis::E()
{
	switch (m_currentToken.getType())
	{
	case T_ADD:
		parseThreeRegisterInstruction(T_ADD, I_ADD);
		break;
	case T_ADDI:
		parseAddImmediate();
		break;
	case T_SUB:
		parseThreeRegisterInstruction(T_SUB, I_SUB);
		break;
	case T_LA:
		parseLoadAddress();
		break;
	case T_LI:
		parseLoadImmediate();
		break;
	case T_LW:
		parseLoadWord();
		break;
	case T_SW:
		parseStoreWord();
		break;
	case T_B:
		parseBranch();
		break;
	case T_BLTZ:
		parseBranchLessThanZero();
		break;
	case T_NOP:
		parseNop();
		break;

	// dodate instrukcije
	case T_AND:
		parseThreeRegisterInstruction(T_AND, I_AND);
		break;
	case T_OR:
		parseThreeRegisterInstruction(T_OR, I_OR);
		break;
	case T_BEQ:
		parseBranchEqual();
		break;
	default:
		reportSyntaxError("Token ne predstavlja podrzanu MAVN instrukciju.");
		break;
	}
}

Token SyntaxAnalysis::getNextToken()
{
	if (m_tokenIterator != m_tokenList.end())
	{
		Token token = *m_tokenIterator;
		++m_tokenIterator;
		return token;
	}
	Token eofToken;
	eofToken.makeEofToken();
	return eofToken;
}

void SyntaxAnalysis::eat(TokenType expectedType)
{
	if (m_currentToken.getType() == expectedType)
	{
		m_currentToken = getNextToken();
	}else
	{
		reportSyntaxError(expectedType, "Pronadjen je token drugacijeg tipa.");
	}
}

bool SyntaxAnalysis::isInstructionStart(TokenType type) const
{
	switch (type)
	{
	case T_ADD:
	case T_ADDI:
	case T_SUB:
	case T_LA:
	case T_LI:
	case T_LW:
	case T_SW:
	case T_B:
	case T_BLTZ:
	case T_NOP:
	case T_AND:
	case T_OR:
	case T_BEQ:
		return true;

	default:
		return false;
	}
}

string SyntaxAnalysis::tokenTypeToString(TokenType type) const
{
	switch (type)
	{
	case T_NO_TYPE:
		return "T_NO_TYPE";
	case T_ID:
		return "T_ID";
	case T_M_ID:
		return "T_M_ID";
	case T_R_ID:
		return "T_R_ID";
	case T_NUM:
		return "T_NUM";
	case T_WHITE_SPACE:
		return "T_WHITE_SPACE";
	case T_MEM:
		return "T_MEM";
	case T_REG:
		return "T_REG";
	case T_FUNC:
		return "T_FUNC";
	case T_ADD:
		return "T_ADD";
	case T_ADDI:
		return "T_ADDI";
	case T_SUB:
		return "T_SUB";
	case T_LA:
		return "T_LA";
	case T_LI:
		return "T_LI";
	case T_LW:
		return "T_LW";
	case T_SW:
		return "T_SW";
	case T_BLTZ:
		return "T_BLTZ";
	case T_B:
		return "T_B";
	case T_NOP:
		return "T_NOP";
	case T_AND:
		return "T_AND";
	case T_OR:
		return "T_OR";
	case T_BEQ:
		return "T_BEQ";
	case T_COMMA:
		return "T_COMMA";
	case T_L_PARENT:
		return "T_L_PARENT";
	case T_R_PARENT:
		return "T_R_PARENT";
	case T_COL:
		return "T_COL";
	case T_SEMI_COL:
		return "T_SEMI_COL";
	case T_COMMENT:
		return "T_COMMENT";
	case T_END_OF_FILE:
		return "T_END_OF_FILE";
	case T_ERROR:
		return "T_ERROR";

	default:
		return "UNKNOWN_TOKEN";
	}
}

void SyntaxAnalysis::reportSyntaxError(TokenType expectedType, const string& message)
{
	m_hasSyntaxError = true;
	m_errorToken = m_currentToken;
	m_expectedToken = expectedType;
	ostringstream stream;
	stream << "Syntax error! Token '" << m_currentToken.getValue() << "' (" << tokenTypeToString(m_currentToken.getType()) << ") nije ocekivan. Ocekivano: " << tokenTypeToString(expectedType) << ".";
	if (!message.empty())
	{
		stream << " " << message;
	}
	m_errorMessage = stream.str();
	throw runtime_error(m_errorMessage);
}

void SyntaxAnalysis::reportSyntaxError(const string& message)
{
	m_hasSyntaxError = true;
	m_errorToken = m_currentToken;
	m_expectedToken = T_NO_TYPE;
	ostringstream stream;
	stream << "Syntax error! Token '" << m_currentToken.getValue() << "' (" << tokenTypeToString(m_currentToken.getType()) << ") nije ocekivan. " << message;
	m_errorMessage = stream.str();
	throw runtime_error(m_errorMessage);
}

//deklaracija
void SyntaxAnalysis::parseMemoryDeclaration()
{
	eat(T_MEM);
	if (m_currentToken.getType() != T_M_ID)
	{
		eat(T_M_ID);
	}
	string variableName = m_currentToken.getValue();
	if (findVariable(variableName) != nullptr)
	{
		reportSyntaxError("Varijabla '" + variableName +"' je vec deklarisana.");
	}
	eat(T_M_ID);
	if (m_currentToken.getType() != T_NUM)
	{
		eat(T_NUM);
	}
	int value = 0;
	try
	{
		value = stoi(m_currentToken.getValue());
	}
	catch (const exception&)
	{
		reportSyntaxError("Vrednost memorijske varijable nije validni int.");
	}
	eat(T_NUM);
	declareVariable(variableName, Variable::MEM_VAR, value);
}

void SyntaxAnalysis::parseRegisterDeclaration()
{
	eat(T_REG);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string variableName = m_currentToken.getValue();
	if (findVariable(variableName) != nullptr)
	{
		reportSyntaxError("Varijabla '" + variableName + "' je vec deklarisana.");
	}
	eat(T_R_ID);
	declareVariable(variableName, Variable::REG_VAR);
}

void SyntaxAnalysis::parseFunctionDeclaration()
{
	eat(T_FUNC);
	if (m_currentToken.getType() != T_ID)
	{
		eat(T_ID);
	}
	string functionName = m_currentToken.getValue();
	if (!m_functionName.empty())
	{
		reportSyntaxError("Dozvoljeno je samo jednom deklarisati funckiju.");
	}

	if (labelExists(functionName))
	{
		reportSyntaxError("Ime funckije name '" + functionName + "' je vec iskorisceno kao labela.");
	}
	m_functionName = functionName;
	eat(T_ID);
}

void SyntaxAnalysis::parseLabelledInstruction()
{
	if (m_currentToken.getType() != T_ID)
	{
		eat(T_ID);
	}
	string labelName = m_currentToken.getValue();
	if (labelExists(labelName))
	{
		reportSyntaxError("Labela '" + labelName + "'je vec deklarisana.");
	}
	if (!m_functionName.empty() &&
		labelName == m_functionName)
	{
		reportSyntaxError("Labela ne moze imati isto ime kao funkcija.");
	}
	eat(T_ID);
	eat(T_COL);
	m_labels.push_back(labelName);
	m_pendingLabel = labelName;
	E();
}

void SyntaxAnalysis::parseThreeRegisterInstruction(TokenType tokenType, InstructionType instructionType)
{
	eat(tokenType);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string destinationName = m_currentToken.getValue();
	Variable* destination = requireVariable(destinationName,Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string source1Name = m_currentToken.getValue();
	Variable* source1 = requireVariable(source1Name, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string source2Name = m_currentToken.getValue();
	Variable* source2 = requireVariable(source2Name, Variable::REG_VAR);
	eat(T_R_ID);
	Variables destinationVariables;
	Variables sourceVariables;
	destinationVariables.push_back(destination);
	sourceVariables.push_back(source1);
	sourceVariables.push_back(source2);
	createInstruction(instructionType, destinationVariables, sourceVariables);
}

void SyntaxAnalysis::parseAddImmediate()
{
	eat(T_ADDI);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string destinationName = m_currentToken.getValue();
	Variable* destination = requireVariable(destinationName, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string sourceName = m_currentToken.getValue();
	Variable* source = requireVariable(sourceName, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_NUM)
	{
		eat(T_NUM);
	}
	int immediate = 0;
	try
	{
		immediate = stoi(m_currentToken.getValue());
	}
	catch (const exception&)
	{
		reportSyntaxError("Immediate value in addi is not valid.");
	}
	eat(T_NUM);
	Variables destinationVariables;
	Variables sourceVariables;
	destinationVariables.push_back(destination);
	sourceVariables.push_back(source);
	createInstruction(I_ADDI, destinationVariables, sourceVariables, true, immediate);
}

void SyntaxAnalysis::parseLoadAddress()
{
	eat(T_LA);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string destinationName = m_currentToken.getValue();
	Variable* destination = requireVariable(destinationName, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_M_ID)
	{
		eat(T_M_ID);
	}
	string memoryName = m_currentToken.getValue();
	Variable* memoryVariable = requireVariable(memoryName, Variable::MEM_VAR);
	eat(T_M_ID);
	Variables destinationVariables;
	Variables sourceVariables;
	destinationVariables.push_back(destination);
	sourceVariables.push_back(memoryVariable);
	createInstruction(I_LA, destinationVariables, sourceVariables);
}

void SyntaxAnalysis::parseLoadImmediate()
{
	eat(T_LI);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string destinationName = m_currentToken.getValue();
	Variable* destination = requireVariable(destinationName, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_NUM)
	{
		eat(T_NUM);
	}
	int immediate = 0;
	try
	{
		immediate = stoi(m_currentToken.getValue());
	}
	catch (const exception&)
	{
		reportSyntaxError("Immediate value in li is not valid.");
	}
	eat(T_NUM);
	Variables destinationVariables;
	Variables sourceVariables;
	destinationVariables.push_back(destination);
	createInstruction(I_LI, destinationVariables, sourceVariables, true, immediate);
}

void SyntaxAnalysis::parseLoadWord()
{
	eat(T_LW);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string destinationName = m_currentToken.getValue();
	Variable* destination = requireVariable(destinationName, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_NUM)
	{
		eat(T_NUM);
	}
	int offset = 0;
	try
	{
		offset = stoi(m_currentToken.getValue());
	}
	catch (const exception&)
	{
		reportSyntaxError("Offset value in lw is not valid.");
	}
	eat(T_NUM);
	eat(T_L_PARENT);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string baseName = m_currentToken.getValue();
	Variable* baseVariable = requireVariable(baseName, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_R_PARENT);
	Variables destinationVariables;
	Variables sourceVariables;
	destinationVariables.push_back(destination);
	sourceVariables.push_back(baseVariable);
	createInstruction(I_LW, destinationVariables, sourceVariables, true, offset);
}

void SyntaxAnalysis::parseStoreWord()
{
	eat(T_SW);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string valueName = m_currentToken.getValue();
	Variable* valueVariable = requireVariable(valueName, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_NUM)
	{
		eat(T_NUM);
	}
	int offset = 0;
	try
	{
		offset = stoi(m_currentToken.getValue());
	}
	catch (const exception&)
	{
		reportSyntaxError("Offset value in sw is not valid.");
	}
	eat(T_NUM);
	eat(T_L_PARENT);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string baseName = m_currentToken.getValue();
	Variable* baseVariable = requireVariable(baseName, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_R_PARENT);
	Variables destinationVariables;
	Variables sourceVariables;
	sourceVariables.push_back(valueVariable);
	sourceVariables.push_back(baseVariable);
	createInstruction(I_SW, destinationVariables, sourceVariables, true, offset);
}

void SyntaxAnalysis::parseBranch()
{
	eat(T_B);
	if (m_currentToken.getType() != T_ID)
	{
		eat(T_ID);
	}
	string target = m_currentToken.getValue();
	eat(T_ID);
	m_branchTargets.push_back(target);
	Variables destinationVariables;
	Variables sourceVariables;
	createInstruction(I_B, destinationVariables, sourceVariables, false, 0, target);
}

void SyntaxAnalysis::parseBranchLessThanZero()
{
	eat(T_BLTZ);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string registerName = m_currentToken.getValue();
	Variable* registerVariable = requireVariable(registerName, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_ID)
	{
		eat(T_ID);
	}
	string target = m_currentToken.getValue();
	eat(T_ID);
	m_branchTargets.push_back(target);
	Variables destinationVariables;
	Variables sourceVariables;
	sourceVariables.push_back(registerVariable);
	createInstruction(I_BLTZ, destinationVariables, sourceVariables, false, 0, target);
}

void SyntaxAnalysis::parseBranchEqual()
{
	eat(T_BEQ);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string source1Name = m_currentToken.getValue();
	Variable* source1 = requireVariable(source1Name, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_R_ID)
	{
		eat(T_R_ID);
	}
	string source2Name = m_currentToken.getValue();
	Variable* source2 = requireVariable(source2Name, Variable::REG_VAR);
	eat(T_R_ID);
	eat(T_COMMA);
	if (m_currentToken.getType() != T_ID)
	{
		eat(T_ID);
	}
	string target = m_currentToken.getValue();
	eat(T_ID);
	m_branchTargets.push_back(target);
	Variables destinationVariables;
	Variables sourceVariables;
	sourceVariables.push_back(source1);
	sourceVariables.push_back(source2);
	createInstruction(I_BEQ, destinationVariables, sourceVariables, false, 0, target);
}

void SyntaxAnalysis::parseNop()
{
	eat(T_NOP);
	Variables destinationVariables;
	Variables sourceVariables;
	createInstruction(I_NOP, destinationVariables, sourceVariables);
}

// rad sa promenljivima i instrukcijama
Variable* SyntaxAnalysis::findVariable(const string& name) const
{
	for (Variables::const_iterator iterator = m_variables.begin(); iterator != m_variables.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable != nullptr &&
			variable->getName() == name)
		{
			return variable;
		}
	}
	return nullptr;
}

Variable* SyntaxAnalysis::declareVariable(const string& name, Variable::VariableType type, int value)
{
	if (findVariable(name) != nullptr)
	{
		reportSyntaxError("Varijabla '" + name + "' je vec deklarisana.");
	}
	Variable* variable = new Variable(name, m_variablePosition, type, value);
	++m_variablePosition;
	m_variables.push_back(variable);
	return variable;
}

Variable* SyntaxAnalysis::requireVariable(const string& name, Variable::VariableType expectedType)
{
	Variable* variable = findVariable(name);
	if (variable == nullptr)
	{
		reportSyntaxError("Varijabla '" + name + "' nije deklarisana.");
	}
	if (variable->getType() != expectedType)
	{
		if (expectedType == Variable::REG_VAR)
		{
			reportSyntaxError("Varijabla '" + name + "' nije registarska.");
		}
		else if (expectedType == Variable::MEM_VAR)
		{
			reportSyntaxError("Varijabla '" + name + "' nije memorijska.");
		}
	}
	return variable;
}

Instruction* SyntaxAnalysis::createInstruction(InstructionType type, const Variables& dst, const Variables& src, bool hasImmediate, int immediate, const string& branchTarget)
{
	Instruction* instruction = new Instruction(m_instructionPosition, type, dst, src);
	++m_instructionPosition;
	if (hasImmediate)
	{
		instruction->setImmediate(immediate);
	}
	if (!branchTarget.empty())
	{
		instruction->setBranchTarget(branchTarget);
	}
	if (!m_pendingLabel.empty())
	{
		instruction->setLabel(m_pendingLabel);
		m_pendingLabel.clear();
	}
	for (Variables::const_iterator iterator = dst.begin(); iterator != dst.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable == nullptr || variable->getType() != Variable::REG_VAR)
		{
			continue;
		}
		if (find(instruction->getDef().begin(), instruction->getDef().end(), variable) == instruction->getDef().end())
		{
			instruction->addDef(variable);
		}
	}
	for (Variables::const_iterator iterator = src.begin(); iterator != src.end(); ++iterator)
	{
		Variable* variable = *iterator;
		if (variable == nullptr || variable->getType() != Variable::REG_VAR)
		{
			continue;
		}

		if (find(instruction->getUse().begin(), instruction->getUse().end(),variable) == instruction->getUse().end())
		{
			instruction->addUse(variable);
		}
	}
	m_instructions.push_back(instruction);
	return instruction;
}

bool SyntaxAnalysis::labelExists(const string& label) const
{
	for (list<string>::const_iterator iterator = m_labels.begin(); iterator != m_labels.end(); ++iterator)
	{
		if (*iterator == label)
		{
			return true;
		}
	}

	return false;
}

void SyntaxAnalysis::validateBranchTargets()
{
	for (list<string>::const_iterator iterator = m_branchTargets.begin(); iterator != m_branchTargets.end(); ++iterator)
	{
		const string& target = *iterator;
		if (!labelExists(target) && target != m_functionName)
		{
			reportSyntaxError("Branch instrukcija koristi nedefinisanu labelu '" + target + "'.");
		}
	}
}

void SyntaxAnalysis::clearIntermediateRepresentation()
{
	for (Instructions::iterator iterator = m_instructions.begin(); iterator != m_instructions.end(); ++iterator)
	{
		delete* iterator;
	}
	m_instructions.clear();
	for (Variables::iterator iterator = m_variables.begin(); iterator != m_variables.end(); ++iterator)
	{
		delete* iterator;
	}
	m_variables.clear();
}

//ispis

void SyntaxAnalysis::printSyntaxError() const
{
	if (m_hasSyntaxError)
	{
		cout << m_errorMessage << endl;
	}
	else
	{
		cout << "Nema syntax error!" << endl;
	}
}

Variables& SyntaxAnalysis::getVariables()
{
	return m_variables;
}

Instructions& SyntaxAnalysis::getInstructions()
{
	return m_instructions;
}

const string& SyntaxAnalysis::getFunctionName() const
{
	return m_functionName;
}

const list<string>& SyntaxAnalysis::getLabels() const
{
	return m_labels;
}