//
// FunctionRenameTransform.cpp
//

#include "Transforms.h"
#include "RenameTransforms.h"

using namespace clang;

class FunctionRenameTransform : public RenameTransform {
public:
  virtual void HandleTranslationUnit(ASTContext &) override;
  
  
protected:
  void collectAndRenameFunctionDecl(DeclContext *DC, bool topLevel = false);
  void processDeclContext(DeclContext *DC, bool topLevel = false);  
  void processStmt(Stmt *S);
};

REGISTER_TRANSFORM(FunctionRenameTransform);

void FunctionRenameTransform::HandleTranslationUnit(ASTContext &C)
{
  auto I = loadConfig("FunctionRename", "Functions");
  if (!I) {
    return;
  }

  auto TUD = C.getTranslationUnitDecl();
  collectAndRenameFunctionDecl(TUD, true);
  processDeclContext(TUD, true);
}

void FunctionRenameTransform::collectAndRenameFunctionDecl(DeclContext *DC,
                                                           bool topLevel)
{
  // TODO: Skip globally touched locations
  // if a.cpp and b.cpp both include c.h, then once a.cpp is processed,
  // we cas skip any location that is not in b.cpp

  pushIndent();
  
  for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {
    auto L = (*I)->getLocation();
    if (topLevel && shouldIgnore(L)) {
      continue;
    }
    
    if (auto D = dyn_cast<FunctionDecl>(*I)) {
      // TODO: If it's a ctor/dtor, it's an error
      
      std::string newName;
      if (nameMatches(D, newName)) {
        renameLocation(D->getLocation(), newName);
      }

      // see if it overrides a known method
      pushIndent();
      if (auto M = dyn_cast<CXXMethodDecl>(D)) {
        for (auto MI = M->begin_overridden_methods(),
             ME = M->end_overridden_methods(); MI != ME; ++MI) {
          if (nameMatches(*MI, newName, true)) {
            renameLocation(D->getLocation(), newName);
          }
        }
      }
      popIndent();
    }
    
    // descend into the next level (namespace, etc.)    
    if (auto innerDC = dyn_cast<DeclContext>(*I)) {
      collectAndRenameFunctionDecl(innerDC);
    }
  }
  popIndent();  
}


void FunctionRenameTransform::processDeclContext(DeclContext *DC,
                                                 bool topLevel)
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
  // llvm::errs() << indent() << "Stmt: " << S->getStmtClassName() << ", at: "<< loc(S->getLocStart()) << "\n";

  if (auto E = dyn_cast<MemberExpr>(S)) {
    // handle the case for member references (e.g. calling foo(), A::foo())
    if (auto D = E->getMemberDecl()) {
      if (dyn_cast<FunctionDecl>(D)) {
        std::string newName;
        if (nameMatches(D, newName, true)) {
          renameLocation(E->getMemberLoc(), newName);
        }
      }
    }
  }
  else if (auto E = dyn_cast<DeclRefExpr>(S)) {
    // handle C function calls
    if (auto D = E->getDecl()) {
      if (dyn_cast<FunctionDecl>(D)) {
        std::string newName;
        if (nameMatches(D, newName, true)) {
          renameLocation(E->getLocation(), newName);
        }
      }
    }
  }

  for (auto I = S->child_begin(), E = S->child_end(); I != E; ++I) {
    processStmt(*I);
  }
  
  popIndent();
}
