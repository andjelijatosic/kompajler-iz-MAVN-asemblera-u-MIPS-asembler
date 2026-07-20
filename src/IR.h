#pragma once

#include "Types.h"


/**
 * This class represents one variable from program code.
 */
class Variable
{
public:
	enum VariableType
	{
		MEM_VAR,
		REG_VAR,
		NO_TYPE
	};

	Variable() : m_type(NO_TYPE), m_name(""), m_position(-1), m_assignment(no_assign), m_value(0) {}
	Variable(const std::string name, int pos, VariableType type, int value = 0) : m_type(type), m_name(name), m_position(pos), m_assignment(no_assign), m_value(value) {}
	
	// getteri i setteri
	VariableType getType() const {return m_type;}

	void setType(VariableType type) {m_type = type;}

	const std::string& getName() const { return m_name;}

	void setName(const std::string& name) {m_name = name;}

	int getPosition() const {return m_position;}

	void setPosition(int position) {m_position = position;}

	Regs getAssignment() const {return m_assignment;}

	void setAssignment(Regs assignment) {m_assignment = assignment;}

	int getValue() const {return m_value;}

	void setValue(int value) {m_value = value;}

private:
	VariableType m_type;		// razlikuje REG_VAR i MEM_VAR
	std::string m_name;
	int m_position;
	Regs m_assignment;			// kasnije dobija t0, t1, t2 ili t3
	int m_value;				// cuva pocetnu vrednost memorijske promenljive	
};


/**
 * This type represents list of variables from program code.
 */
typedef std::list<Variable*> Variables;

/**
 * This class represents one instruction in program code.
 */
class Instruction
{
public:
	Instruction () : m_position(0), m_type(I_NO_TYPE), m_immediate(0), m_hasImmediate(false), m_label(""), m_branchTarget("") {}
	Instruction (int pos, InstructionType type, const Variables& dst, const Variables& src) :
		m_position(pos), m_type(type), m_dst(dst), m_src(src), m_immediate(0), m_hasImmediate(false), m_label(""), m_branchTarget("") {}

	// getteri i setteri
	int getPosition() const {return m_position;}

	void setPosition(int position){m_position = position;}

	InstructionType getType() const{return m_type;}

	void setType(InstructionType type){m_type = type;}

	Variables& getDst(){return m_dst;}

	const Variables& getDst() const{return m_dst;}

	Variables& getSrc() {return m_src;}

	const Variables& getSrc() const {return m_src;}

	Variables& getUse() {return m_use;}

	const Variables& getUse() const {return m_use;}

	Variables& getDef() {return m_def;}

	const Variables& getDef() const {return m_def;}

	Variables& getIn() {return m_in;}

	const Variables& getIn() const {return m_in;}

	Variables& getOut() {return m_out;}

	const Variables& getOut() const {return m_out;}

	std::list<Instruction*>& getSucc() {return m_succ;}

	const std::list<Instruction*>& getSucc() const {return m_succ;}

	std::list<Instruction*>& getPred() {return m_pred;}

	const std::list<Instruction*>& getPred() const {return m_pred;}

	void addDst(Variable* variable) {
		if (variable != nullptr)
			m_dst.push_back(variable);
	}

	void addSrc(Variable* variable) {
		if (variable != nullptr)
			m_src.push_back(variable);
	}

	void addUse(Variable* variable) {
		if (variable != nullptr)
			m_use.push_back(variable);
	}

	void addDef(Variable* variable) {
		if (variable != nullptr)
			m_def.push_back(variable);
	}

	void addSuccessor(Instruction* instruction) {
		if (instruction != nullptr)
			m_succ.push_back(instruction);
	}

	void addPredecessor(Instruction* instruction) {
		if (instruction != nullptr)
			m_pred.push_back(instruction);
	}

	void setImmediate(int immediate) {
		m_immediate = immediate;
		m_hasImmediate = true;
	}

	int getImmediate() const {return m_immediate;}

	bool hasImmediate() const { return m_hasImmediate;}

	void setLabel(const std::string& label) {m_label = label;}

	const std::string& getLabel() const {return m_label;}

	void setBranchTarget(const std::string& target) {m_branchTarget = target;}

	const std::string& getBranchTarget() const {return m_branchTarget;}

private:
	int m_position;
	InstructionType m_type;
	
	Variables m_dst;
	Variables m_src;

	// analiza zivotnog veka
	Variables m_use;
	Variables m_def;
	Variables m_in;
	Variables m_out;

	// veze u grafu toka upravljanja
	std::list<Instruction*> m_succ;
	std::list<Instruction*> m_pred;

	// konstanta koja cuva broj kod li/addi ili pomeraj kod lw/sw
	int m_immediate;
	bool m_hasImmediate;

	// labela koja se nalazi ispred svake instrukcije
	std::string m_label;

	// odredisna labela kod b, bltz i beq
	std::string m_branchTarget;

};


/**
 * This type represents list of instructions from program code.
 */
typedef std::list<Instruction*> Instructions;
