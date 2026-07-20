#include "LexicalAnalysis.h"
#include "SyntaxAnalysis.h"
#include "LivenessAnalysis.h"
#include "InterferenceGraph.h"
#include "ResourceAllocation.h"
#include "CodeGenerator.h"
#include "SpillCode.h"

#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>

using namespace std;

void main()
{
	try
	{
		std::string fileName = ".\\..\\examples\\simple.mavn";
		std::string outputFileName = ".\\..\\examples\\simple.s";
		bool retVal = false;

		// leksicka analiza
		LexicalAnalysis lex;
		
		if (!lex.readInputFile(fileName))
			throw runtime_error("\nException! Nije uspeo da otvori input file!\n");
		
		lex.initialize();

		retVal = lex.Do();

		if (retVal)
		{
			cout << "Leksicka analiza je bila uspesna!" << endl;
			lex.printTokens();
		}
		else
		{
			lex.printLexError();
			throw runtime_error("\nException! Leksicka analiza nije uspela!\n");
		}

		// sintaksicka analiza
		SyntaxAnalysis syntax(lex.getTokenList());

		if (!syntax.Do()) {
			syntax.printSyntaxError();
			throw runtime_error("Sintaksna analiza nije uspela!");
		}

		cout << "Program je leksicki i sintaksno ispravan" << endl;

		/*
		// analiza zivotnog veka
		LivenessAnalysis liveness(syntax.getInstructions(), syntax.getFunctionName());
		
		if (!liveness.Do()) {
			liveness.printError();
			throw runtime_error("Analiza zivotnog veka nije uspela!");
		}

		liveness.printResults();

		// graf smetnje
		InterferenceGraph graph(syntax.getVariables(), syntax.getInstructions());

		if (!graph.Do()) {
			graph.printError();
			throw runtime_error("Formiranje grafa smetnji nije uspelo");
		}

		graph.printGraph();

		// dodela fizickih registara
		ResourceAllocation resourceAllocation(graph);
		
		if (!resourceAllocation.Do()) {
			resourceAllocation.printError();

			if (resourceAllocation.isSpillDetected()) {
				cout << "Program zahteva vise od 4 registra." << endl;

			}
		}
		
		resourceAllocation.printAssignments();
		*/

		// analiza zivotnog veka, graf smetnji i dodela registara
		bool registersAssigned = false;
		int spillCount = 0;
		while (!registersAssigned)
		{
			// analiza zivotnog veka
			LivenessAnalysis liveness(syntax.getInstructions(), syntax.getFunctionName());
			if (!liveness.Do())
			{
				liveness.printError();
				throw runtime_error("Analiza zivotnog veka nije uspela!");
			}
			liveness.printResults();

			// graf smetnji
			InterferenceGraph graph(syntax.getVariables(), syntax.getInstructions());
			if (!graph.Do())
			{
				graph.printError();
				throw runtime_error("Formiranje grafa smetnji nije uspelo!");
			}
			graph.printGraph();

			// dodela fizickih registara
			ResourceAllocation resourceAllocation(graph);
			if (resourceAllocation.Do())
			{
				resourceAllocation.printAssignments();
				registersAssigned = true;
			}
			else
			{
				resourceAllocation.printError();

				if (!resourceAllocation.isSpillDetected())
				{
					throw runtime_error("Dodela registara nije uspela!");
				}
				cout << "Program zahteva vise od 4 registra." << endl;
				cout << "Pokrece se prelivanje registarske promenljive." << endl;
				SpillCode spillCode(syntax.getVariables(), syntax.getInstructions(), graph);
				if (!spillCode.Do())
				{
					spillCode.printError();
					throw runtime_error("Prelivanje nije uspelo!");
				}
				++spillCount;
				if (spillCount >= 10)
				{
					throw runtime_error("Prekoracen je maksimalan broj prelivanja!");
				}
				cout << endl;
				cout << "Ponovo se pokrecu analiza zivotnog veka, " << "graf smetnji i dodela registara." << endl;
				cout << endl;
			}
		}

		// generisanje MIPS koda
		CodeGenerator generator(syntax.getVariables(), syntax.getInstructions(), syntax.getFunctionName(), outputFileName);

		if (!generator.Do()) {
			generator.printError();
			throw runtime_error("Generisanje MIPS koda nije uspelo!");
		}

		cout << "MAVN program je uspesno preveden" << endl;
	}
	catch (runtime_error e)
	{
		cout << e.what() << endl;
	}
}