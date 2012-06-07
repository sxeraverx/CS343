//
// RecordFieldRenameTransform.cpp
//

#include "RenameTransforms.h"

using namespace clang;

class RecordFieldRenameTransform : public RenameTransform {
public:
  virtual void HandleTranslationUnit(ASTContext &) override;
  void collectAndRenameFieldDecl(DeclContext *DC);
  void processDeclContext(DeclContext *DC);  
  void processStmt(Stmt *S);
  
protected:
};

REGISTER_TRANSFORM(RecordFieldRenameTransform);

void RecordFieldRenameTransform::HandleTranslationUnit(ASTContext &C)
{
  auto I = loadConfig("RecordFieldRename", "Fields");
  if (!I) {
    return;
  }
  
  auto TUD = C.getTranslationUnitDecl();  
  collectAndRenameFieldDecl(TUD);
  processDeclContext(TUD);
}

void RecordFieldRenameTransform::collectAndRenameFieldDecl(DeclContext *DC)
{
  pushIndent();
  
  for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {
    auto L = (*I)->getLocation();
    if (shouldIgnore(L)) {
      continue;
    }
    
    if (auto D = dyn_cast<FieldDecl>(*I)) {
      llvm::errs() << indent() << "Field: " << D->getQualifiedNameAsString() << "\n";
      
      std::string newName;
      if (nameMatches(D, newName)) {
        llvm::errs() << indent() << "Rename to: " << newName << "\n";
        renameLocation(D->getLocation(), newName);
      }
    }
    
    // descend into the next level (namespace, etc.)    
    if (auto innerDC = dyn_cast<DeclContext>(*I)) {
      collectAndRenameFieldDecl(innerDC);
    }
  }
  popIndent();  
}

void RecordFieldRenameTransform::processDeclContext(DeclContext *DC)
{  
  // TODO: Skip globally touched locations
  // if a.cpp and b.cpp both include c.h, then once a.cpp is processed,
  // we cas skip any location that is not in b.cpp

  pushIndent();
  
  for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {
    auto L = (*I)->getLocation();
    if (shouldIgnore(L)) {
      continue;
    }
  
    if (auto D = dyn_cast<FunctionDecl>(*I)) {
      // handle ctor name initializers
      if (auto CD = dyn_cast<CXXConstructorDecl>(D)) {
        auto BL = CD->getLocation();
        for (auto II = CD->init_begin(), IE = CD->init_end(); II != IE; ++II) {
          if (auto M = (*II)->getAnyMember()) {
            // rename the referenced member
            
            llvm::errs() << indent() << "Init'er: " << M->getQualifiedNameAsString()
              << ", at: " << loc(M->getLocation()) << "\n";
            
            std::string newName;
            if (nameMatches(M, newName)) {
              llvm::errs() << indent() << "Rename to: " << newName << "\n";
              renameLocation((*II)->getMemberLocation(), newName);
            }            
          }
          
          if (auto X = (*II)->getInit()) {
            processStmt(X);
          }
        }
      }
      
      // rename the params' initializers
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

    // descend into the next level (namespace, etc.)    
    if (auto innerDC = dyn_cast<DeclContext>(*I)) {
      processDeclContext(innerDC);
    }
  }
  popIndent();
}

void RecordFieldRenameTransform::processStmt(Stmt *S)
{
  if (!S) {
    return;
  }

  pushIndent();
  llvm::errs() << indent() << "Stmt: " << S->getStmtClassName() << ", at: "<< loc(S->getLocStart()) << "\n";

  if (auto E = dyn_cast<MemberExpr>(S)) {
    if (auto D = E->getMemberDecl()) {
      std::string newName;
      if (nameMatches(D, newName)) {
        renameLocation(E->getMemberLoc(), newName);
      }
    }
  }
  else if (auto E = dyn_cast<DeclRefExpr>(S)) {
    if (auto D = E->getDecl()) {
      std::string newName;
      if (nameMatches(D, newName)) {
        renameLocation(E->getLocation(), newName);
      }
    }
  }

  for (auto I = S->child_begin(), E = S->child_end(); I != E; ++I) {
    processStmt(*I);
  }
  
  popIndent();
}
