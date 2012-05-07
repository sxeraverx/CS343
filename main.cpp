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

template <typename T>
class DAG {
public:
  class Node {
  private:
    T *data;
  public:
    Node() = delete;
    Node(T *t) : data(t) {}
    std::vector<Node> children;
    void sortChildren() {
      std::sort(children);
      for(auto child = children.begin(); child != children.end(); child++) {
	child->sortChildren();
      }
    }
    bool operator==(Node &n) {return *data==*n.data;}
    bool operator<(Node &n) {return *data<*n.data;}
  };
  
  std::vector<Node> root;
};

class PrintFunctionsConsumer : public ASTConsumer {
public:
  void HandleTranslationUnit(ASTContext &Ctx) override {
    const TranslationUnitDecl *topLevel = Ctx.getTranslationUnitDecl();
    
    DAG<FunctionDecl> *depTree = new DAG<FunctionDecl>();
    printNamespaceFunctions(topLevel, depTree);
    delete depTree;
  }
  void addDependencies(const Stmt *body, DAG<FunctionDecl> *depTree, const FunctionDecl *fn_decl) const {
    if(body) {
      llvm::errs() << "\t";
      body->getLocStart().print(llvm::errs(), fn_decl->getASTContext().getSourceManager());
      llvm::errs() << " - ";
      body->getLocEnd().print(llvm::errs(), fn_decl->getASTContext().getSourceManager());
      llvm::errs() << "\n";
      llvm::errs() << "\t" << fn_decl->hasPrototype() << "/" << fn_decl->hasWrittenPrototype() << "/" << fn_decl->isThisDeclarationADefinition() << "\n";
    }
  }
  void printNamespaceFunctions(const DeclContext *ns_decl, DAG<FunctionDecl> *depTree) const
  {
    for(auto subdecl = ns_decl->decls_begin(); subdecl != ns_decl->decls_end(); subdecl++) {
      if(const FunctionDecl *fn_decl = dyn_cast<FunctionDecl>(*subdecl)) {
	if(fn_decl->getASTContext().getSourceManager().isFromMainFile(fn_decl->getSourceRange().getBegin())) {
	  if(const CXXMethodDecl *cxxm_decl = dyn_cast<CXXMethodDecl>(fn_decl)) {
	    //only print the type if it would have to be printed in source
	    if(cxxm_decl->isCopyAssignmentOperator())
	      llvm::errs() << "[copy] ";
	    else if(cxxm_decl->isMoveAssignmentOperator())
	      llvm::errs() << "[move] ";
	    else if(isa<CXXConstructorDecl>(cxxm_decl))
	      llvm::errs() << "[ctor] ";
	    else if(isa<CXXDestructorDecl>(cxxm_decl))
	      llvm::errs() << "[dtor] ";
	    else if(isa<CXXConversionDecl>(cxxm_decl))
	      llvm::errs() << "[conv] ";
	    else
	      llvm::errs() << fn_decl->getResultType().getAsString() << " ";
	  }
	  else
	    llvm::errs() << fn_decl->getResultType().getAsString() << " ";
	  llvm::errs() << fn_decl->getQualifiedNameAsString();
	  
	  llvm::errs() << "(";
	  for(auto param = fn_decl->param_begin(); param != fn_decl->param_end(); param++) {
	    llvm::errs() << (*param)->getType().getAsString();
	    (*param)->printName(llvm::errs());
	    llvm::errs() << " ";
	    if(param+1!=fn_decl->param_end()) {
	      llvm::errs() << ", ";
	    }
	  }
	  llvm::errs() << ")\n";
	  
	  addDependencies(fn_decl->getBody(), depTree, fn_decl);
	}
      }
      //functions are both functions and contexts themselves
      if(const DeclContext *inner_dc_decl = dyn_cast<DeclContext>(*subdecl))
	printNamespaceFunctions(inner_dc_decl, depTree);
    }
  }
};


class PrintFunctionNamesAction : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
    return new PrintFunctionsConsumer();
  }

  virtual bool BeginInvocation(CompilerInstance &CI) override {
    CI.getHeaderSearchOpts().AddPath("/usr/local/lib/clang/3.1/include", clang::frontend::System, false, false, false);
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
