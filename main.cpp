
#include <yaml-cpp/yaml.h>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/AST.h"
#include <clang/Sema/SemaConsumer.h>
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>

#include <iostream>

using namespace clang;

#include "Transforms/Transforms.h"

template<>
struct YAML::convert<YAML::Node> {
	static YAML::Node encode(const YAML::Node& rhs) {return rhs;}
	static bool decode(const YAML::Node& node, YAML::Node& rhs) {rhs=node; return true;}
};

int main(int argc, char **argv)
{	
	std::string errorMessage("Could not load compilation database");
	
	std::vector<YAML::Node> config = YAML::LoadAll(std::cin);
	for(auto configSectionIter = config.begin(); configSectionIter != config.end(); ++configSectionIter)
	{
		TransformRegistry::get().config.clear();
		//figure out which files we need to work on
		YAML::Node& configSection = *configSectionIter;
		std::vector<std::string> inputFiles;
		if(configSection["Files"])
			inputFiles = configSection["Files"].as<std::vector<std::string> >();
		else
		{
			llvm::errs() << "Warning: No files selected. Operating on all files.\n";
			
			YAML::Node compileCommands = YAML::LoadFile("compile_commands.json");
			for(auto iter = compileCommands.begin(); iter != compileCommands.end(); ++iter)
			{
				inputFiles.push_back((*iter)["file"].as<std::string>());
			}
		}
		if(!configSection["Transforms"])
		{
			llvm::errs() << "No transforms specified in this configuration section:\n";
			llvm::errs() << YAML::Dump(configSection) << "\n";
		}
		
		//load up the compilation database
		llvm::OwningPtr<tooling::CompilationDatabase> Compilations(tooling::CompilationDatabase::loadFromDirectory(".", errorMessage));
		tooling::ClangTool ct(*Compilations.take(), inputFiles);
		
		std::map<std::string,std::map<std::string, std::string> > transforms = configSection["Transforms"].as<decltype(transforms)>();
		//save the config for this run
		for(auto iter = transforms.begin(); iter != transforms.end(); ++iter)
		{
			TransformRegistry::get().config[iter->first + "Transform"].insert(iter->second.begin(), iter->second.end());
		}

		//finally, run
		for(auto iter = transforms.begin(); iter != transforms.end(); iter++)
			ct.run(new TransformFactory(TransformRegistry::get()[iter->first + "Transform"]));
		
	}
	return 0;
}
