//
// FunctionRenameTransform.cpp
//

#include "Transforms.h"

using namespace clang;

class FunctionRenameTransform : public Transform {
public:
  virtual void HandleTranslationUnit(ASTContext &) override;
  void processDeclContext(DeclContext *DC);  
  void processStmt(Stmt *S);
  
protected:
  int indentLevel;
  
  std::string indent() {
    return std::string(indentLevel, '\t');
  }
  
  void pushIndent() {
    indentLevel++;
  }
  
  void popIndent() {
    indentLevel--;
  }
  
  std::string loc(SourceLocation L) {
    std::string src;
    llvm::raw_string_ostream sst(src);
    L.print(sst, sema->getSourceManager());
    return sst.str();
  }
};

REGISTER_TRANSFORM(FunctionRenameTransform);

void FunctionRenameTransform::HandleTranslationUnit(ASTContext &C)
{
  auto TUD = C.getTranslationUnitDecl();
  processDeclContext(TUD);
}

void FunctionRenameTransform::processDeclContext(DeclContext *DC)
{  
  // TODO: ignore system headers (/usr, /opt, /System and /Library)
  // TODO: Skip globally touched locations
  //
  // if a.cpp and b.cpp both include c.h, then once a.cpp is processed,
  // we cas skip any location that is not in b.cpp
  //

  pushIndent();
  
  for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {
    if (auto D = dyn_cast<FunctionDecl>(*I)) {
      // TODO: If it's a ctor/dtor, it's an error
      llvm::errs() << "Function " << loc(D->getLocation()) << "\n";

      // handle ctor name initializers
      if (auto CD = dyn_cast<CXXConstructorDecl>(D)) {
        auto BL = CD->getLocation();
        for (auto II = CD->init_begin(), IE = CD->init_end(); II != IE; ++II) {
          if (auto X = (*II)->getInit()) {
            processStmt(X);
          }
        }
      }
      
      // rename the params' types
      for (auto PI = D->param_begin(), PE = D->param_end(); PI != PE; ++PI) {
        if ((*PI)->hasInit()) {
          processStmt((*PI)->getInit());
        }  
      }
      
      // handle body
      if (D->hasBody()) {
        processStmt(D->getBody());
      }
    }
    else if (auto D = dyn_cast<VarDecl>(*I)) {
      // handle initialization
      if (D->hasInit()) {
        processStmt(D->getInit());
      }
    }
    else if (auto D = dyn_cast<ObjCMethodDecl>(*I)) {
      // handle body
      auto B = D->getBody();
      if (B) {
        processStmt(B);
      }
    }
    
    // TODO: Handle ObjC interface/impl, inheritance, protocol
    // TODO: Whether we should support category rename?

    // descend into the next level (namespace, etc.)    
    if (auto innerDC = dyn_cast<DeclContext>(*I)) {
      processDeclContext(innerDC);
    }
  }
  popIndent();
}

void FunctionRenameTransform::processStmt(Stmt *S)
{
  if (!S) {
    return;
  }
  
  pushIndent();
  llvm::errs() << indent() << "Stmt: " << S->getStmtClassName() << ", at: "<< loc(S->getLocStart()) << "\n";
  
  for (auto I = S->child_begin(), E = S->child_end(); I != E; ++I) {
    processStmt(*I);
  }
  
  popIndent();
}
