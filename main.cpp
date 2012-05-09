//===- PrintFunctionNames.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which simply prints the names of all the top-level decls
// in the input file.
//
//===----------------------------------------------------------------------===//

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/AST.h"
#include <clang/Sema/SemaConsumer.h>
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
using namespace clang;

namespace {

class PrintFunctionsConsumer : public ASTConsumer {
public:
  void HandleTranslationUnit(ASTContext &Ctx) override {
    const TranslationUnitDecl *topLevel = Ctx.getTranslationUnitDecl();
    
    printNamespaceFunctions(topLevel);
  }
  void printNamespaceFunctions(const DeclContext *ns_decl) const
  {
    for(auto subdecl = ns_decl->decls_begin(); subdecl != ns_decl->decls_end(); subdecl++) {
      if(const RecordDecl *rc_decl = dyn_cast<RecordDecl>(*subdecl)) {
	llvm::errs() << rc_decl->getQualifiedNameAsString() << "\n";
	for(auto member = rc_decl->field_begin(); member != rc_decl->field_end(); member++) {
	  llvm::errs() << "\t" << member->getType().getAsString() << " " << member->getQualifiedNameAsString() << "\n";
	}
      }
      //recurse into inner contexts
      if(const DeclContext *inner_dc_decl = dyn_cast<DeclContext>(*subdecl))
	printNamespaceFunctions(inner_dc_decl);
    }
  }
};

class PrintFunctionNamesAction : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
    return new PrintFunctionsConsumer();
  }

  virtual bool BeginInvocation(CompilerInstance &CI) override {
    CI.getHeaderSearchOpts().AddPath("/usr/local/lib/clang/3.2/include", clang::frontend::System, false, false, false);
    return true;
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) override {
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      llvm::errs() << "PrintFunctionNames arg = " << args[i] << "\n";

      // Example error handling.
      if (args[i] == "-an-error") {
        DiagnosticsEngine &D = CI.getDiagnostics();
        unsigned DiagID = D.getCustomDiagID(
          DiagnosticsEngine::Error, "invalid argument '" + args[i] + "'");
        D.Report(DiagID);
        return false;
      }
    }
    if (args.size() && args[0] == "help")
      PrintHelp(llvm::errs());

    return true;
  }
  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help for PrintFunctionNames plugin goes here\n";
  }

};

class PrintFunctionsFactory : public tooling::FrontendActionFactory {
public:
  PrintFunctionNamesAction *create() override {return new PrintFunctionNamesAction;}
};

}

int main(int argc, char **argv)
{
  std::string errorMessage("Could not load compilation database");
  llvm::OwningPtr<tooling::CompilationDatabase> Compilations(tooling::CompilationDatabase::loadFromDirectory(".", errorMessage));
  tooling::ClangTool ct(*Compilations.take(), std::vector<std::string>(argv+1, argv+argc));
  ct.run(new PrintFunctionsFactory());
  return 0;
}
