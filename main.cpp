
#include <yaml-cpp/yaml.h>
#include "yaml-util.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/AST.h"
#include <clang/Sema/SemaConsumer.h>
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include "Refactoring.h"

#include <iostream>
#include <fstream>

#include <unistd.h>

using namespace clang;
using namespace std;

#include "Transforms/Transforms.h"

int main(int argc, char **argv)
{	
	string errorMessage("Could not load compilation database");

	YAML::Node compileCommands = YAML::LoadFile("compile_commands.json");
	
	vector<YAML::Node> config = YAML::LoadAll(cin);
	for(auto configSectionIter = config.begin(); configSectionIter != config.end(); ++configSectionIter)
	{
		TransformRegistry::get().config = YAML::Node();
		//figure out which files we need to work on
		YAML::Node& configSection = *configSectionIter;
		vector<string> inputFiles;
		if(configSection["Files"])
			inputFiles = configSection["Files"].as<vector<string> >();
		else
		{
			llvm::errs() << "Warning: No files selected. Operating on all files.\n";
			
			for(auto iter = compileCommands.begin(); iter != compileCommands.end(); ++iter)
			{
				inputFiles.push_back((*iter)["file"].as<string>());
			}
		}
		if(!configSection["Transforms"])
		{
			llvm::errs() << "No transforms specified in this configuration section:\n";
			llvm::errs() << YAML::Dump(configSection) << "\n";
		}
		
		//load up the compilation database
		llvm::OwningPtr<tooling::CompilationDatabase> Compilations(tooling::CompilationDatabase::loadFromDirectory(".", errorMessage));
		RefactoringTool rt(*Compilations.take(), inputFiles);
		
		TransformRegistry::get().config = configSection["Transforms"];
		TransformRegistry::get().replacements = &rt.getReplacements();
		
		//finally, run
		for(auto iter = configSection["Transforms"].begin(); iter != configSection["Transforms"].end(); iter++)
		{
			
			llvm::errs() << iter->first.as<string>() +"Transform" << "\n";
			rt.run(new TransformFactory(TransformRegistry::get()[iter->first.as<string>() + "Transform"]));
		}
	}
	return 0;
}
