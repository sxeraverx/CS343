//
// FunctionRenameTransform.cpp
//

#include "Transforms.h"
#include <clang/Lex/Preprocessor.h>

using namespace clang;

class FunctionRenameTransform : public Transform {
public:
  FunctionRenameTransform() : indentLevel(0) {}  
  virtual void HandleTranslationUnit(ASTContext &) override;
  
  void collectAndRenameFunctionDecl(DeclContext *DC);
  void processDeclContext(DeclContext *DC);  
  void processStmt(Stmt *S);
  
protected:
  std::string fromFunctionQualifiedName;
  std::string toFunctionName;
  
  std::map<Decl *, std::string> functionNameMap;
  bool functionMatches(NamedDecl *N, std::string &outNewName);
  void renameLocation(SourceLocation L, std::string& N);

  
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
	fromFunctionQualifiedName =
	  TransformRegistry::get().config["FunctionRenameTransform"].begin()->first;
  toFunctionName =
    TransformRegistry::get().config["FunctionRenameTransform"].begin()->second;
  
  llvm::errs() << "FunctionRenameTransform, from: " << fromFunctionQualifiedName
    << ", to: "<< toFunctionName << "\n";
  
  auto TUD = C.getTranslationUnitDecl();
  collectAndRenameFunctionDecl(TUD);
  processDeclContext(TUD);
}

void FunctionRenameTransform::collectAndRenameFunctionDecl(DeclContext *DC)
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
      llvm::errs() << indent() << "Function " << D->getQualifiedNameAsString() << ", at: "<< loc(D->getLocation()) << "\n";
      
      std::string newName;
      if (functionMatches(D, newName)) {
        llvm::errs() << indent() << "matches: " << newName << "\n";
        renameLocation(D->getLocation(), newName);
      }
    }
    
    // descend into the next level (namespace, etc.)    
    if (auto innerDC = dyn_cast<DeclContext>(*I)) {
      collectAndRenameFunctionDecl(innerDC);
    }
  }
  popIndent();  
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

  if (auto E = dyn_cast<MemberExpr>(S)) {
    if (auto D = E->getMemberDecl()) {
      std::string newName;
      if (functionMatches(D, newName)) {
        llvm::errs() << indent() << "match: " << newName << ", at: " << loc(E->getMemberLoc()) << "\n";
        renameLocation(E->getMemberLoc(), newName);
      }
    }
  }
  else if (auto E = dyn_cast<DeclRefExpr>(S)) {
    if (auto D = E->getDecl()) {
      std::string newName;
      if (functionMatches(D, newName)) {
        llvm::errs() << indent() << "match: " << newName << ", at: " << loc(E->getLocation()) << "\n";
        renameLocation(E->getLocation(), newName);
      }
    }
  }
  

  // if (auto CE = dyn_cast<CallExpr>(S)) {
  //   if (auto FD = CE->getDirectCallee()) {
  //     std::string newName;
  //     if (functionMatches(FD, newName)) {
  //       llvm::errs() << indent() << "match: " << newName << "\n";
  //
  //       auto BL = S->getLocStart();
  //       if (BL.isValid()) {
  //         if (BL.isMacroID()) {
  //           // TODO: emit error
  //         }
  //         else {
  //           Preprocessor &P = sema->getPreprocessor();
  //           auto BLE = P.getLocForEndOfToken(BL);
  //           if (BLE.isValid()) {
  //             auto EL = BLE.getLocWithOffset(-1);
  //             rewriter.ReplaceText(SourceRange(BL, EL), newName);
  //           }
  //         }
  //       }
  //     }
  //   }
  // }

  for (auto I = S->child_begin(), E = S->child_end(); I != E; ++I) {
    processStmt(*I);
  }
  
  popIndent();
}


bool FunctionRenameTransform::functionMatches(NamedDecl *N,
                                              std::string &outNewName)
{
  // TODO: Replace with regex
  auto I = functionNameMap.find(N);
  if (I != functionNameMap.end()) {
    outNewName = (*I).second;
    return true;
  }
  
  auto QN = N->getQualifiedNameAsString();
  if (QN != fromFunctionQualifiedName) {
    return false;
  }
  
  functionNameMap[N] = toFunctionName;
  outNewName = toFunctionName;
  return true;
}

void FunctionRenameTransform::renameLocation(SourceLocation L, std::string &N)
{
  if (L.isValid()) {
    if (L.isMacroID()) {
      // TODO: emit error
    }
    else {
      Preprocessor &P = sema->getPreprocessor();      
      auto LE = P.getLocForEndOfToken(L);
      if (LE.isValid()) {
        auto E = LE.getLocWithOffset(-1);
        rewriter.ReplaceText(SourceRange(L, E), N);
      }
    }
  }
}
