//
// TypeRenameTransform.cpp: Rename C/C++/Objective-C/Objective-C++ Types
//

#include "Transforms.h"
#include <clang/Lex/Preprocessor.h>

using namespace clang;

class TypeRenameTransform : public Transform {
public:
  TypeRenameTransform() : indentLevel(0) {}
  
	virtual std::string getName() override { return "TypeRenameTransform"; }

  virtual void HandleTranslationUnit(ASTContext &C) override;
  
protected:
  std::string fromTypeQualifiedName;
  std::string toTypeName;

  void processDeclContext(DeclContext *DC);  
  void processStmt(Stmt *S);
  void processTypeLoc(TypeLoc TL);
  void processTypeLoc(TypeLoc TL, TypeLoc &headTL);
  void writeOutput();
  
  // TODO: Move these to a utility
private:
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
  
  // a quick way to get the whole TypeLocClass tree
  std::string typeLocClassName(TypeLoc::TypeLocClass C) {
    std::string src;
    llvm::raw_string_ostream sst(src);
    

// will be undefined by clang/AST/TypeNodes.def
#define ABSTRACT_TYPE(Class, Base)
#define TYPE(Class, Base) case TypeLoc::TypeLocClass::Class: \
    sst << #Class; \
    break;

    switch(C) {
      #include <clang/AST/TypeNodes.def>
      case TypeLoc::TypeLocClass::Qualified:
        sst << "Qualified";
        break;
    }
    
    sst << "(" << C << ")";
    
    return sst.str();
  }
};

REGISTER_TRANSFORM(TypeRenameTransform);


void TypeRenameTransform::HandleTranslationUnit(ASTContext &C)
{
  // TODO: Temporary measure of config
  const char* f = getenv("FROM_TYPE_QUALIFIED_NAME");
  const char* t = getenv("TO_TYPE_NAME");
  
  if (!f || !t) {
    llvm::errs() << "No FROM_TYPE_QUALIFIED_NAME or TO_CLASS_NAME specified"
      " -- skipping ClassRenameTransform\n";
    return;
  }
  
  fromTypeQualifiedName = f;
  toTypeName = t;
  
  
  llvm::errs() << "TypeRenameTransform, from: " << fromTypeQualifiedName
    << ", to: "<< toTypeName << "\n";

  auto TUD = C.getTranslationUnitDecl();
  processDeclContext(TUD);
  writeOutput();
}


// TODO: A special case for top-level decl context
// (because we can quickly skip system/refactored nodes there)
void TypeRenameTransform::processDeclContext(DeclContext *DC)
{  
  // TODO: ignore system headers (/usr, /opt, /System and /Library)
  // TODO: Skip globally touched locations
  //
  // if a.cpp and b.cpp both include c.h, then once a.cpp is processed,
  // we cas skip any location that is not in b.cpp
  //

  pushIndent();
  
  for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {
    if (auto TD = dyn_cast<TagDecl>(*I)) {
      // handle tag decls (enum, struct, union, class)
      Preprocessor &P = sema->getPreprocessor();
      auto BL = TD->getLocation();
      
      if (BL.isValid()) {    
        auto EL = P.getLocForEndOfToken(BL).getLocWithOffset(-1);
        auto QTNS = TD->getQualifiedNameAsString();

        // rename
        if (QTNS == fromTypeQualifiedName) {
          rewriter.ReplaceText(SourceRange(BL, EL), toTypeName);        
        }
      }

      if (auto CRD = dyn_cast<CXXRecordDecl>(TD)) {
        // can't call bases_begin() if there's no definition
        if (CRD->hasDefinition()) {        
          for (auto BI = CRD->bases_begin(), BE = CRD->bases_end(); BI != BE; ++BI) {
            if (auto TSI = BI->getTypeSourceInfo()) {
              processTypeLoc(TSI->getTypeLoc());
            }
          }
        }
      } // if a CXXRecordDecl
    }
    else if (auto D = dyn_cast<FunctionDecl>(*I)) {
      // if no type source info, it's a void f(void) function
      auto TSI = D->getTypeSourceInfo();
      if (TSI) {      
        processTypeLoc(TSI->getTypeLoc());
      }

      // handle ctor initializers
      if (auto CD = dyn_cast<CXXConstructorDecl>(D)) {
        for (auto II = CD->init_begin(), IE = CD->init_end(); II != IE; ++II) {
          auto X = (*II)->getInit();
          if (X) {
            processStmt(X);
          }
        }
      }
      
      for (auto PI = D->param_begin(), PE = D->param_end(); PI != PE; ++PI) {
        
        // need to take care of params with no name
        // (param with name is handled by the function's type loc)
        // TODO: understand why it works?
        
        auto PN = (*PI)->getName();
        if (PN.empty()) {
          auto PTSI = (*PI)->getTypeSourceInfo();
          if (PTSI) {
            processTypeLoc(PTSI->getTypeLoc());
          }
        }
        
        // then the default vars
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
      processTypeLoc(D->getTypeSourceInfo()->getTypeLoc());
      
      if (D->hasInit()) {
        processStmt(D->getInit());
      }
    }
    else if (auto D = dyn_cast<FieldDecl>(*I)) {
      processTypeLoc(D->getTypeSourceInfo()->getTypeLoc());
    }
    else if (auto D = dyn_cast<TypedefDecl>(*I)) {
      // typedef T n -- we want to see first if it's n that needs renaming
      Preprocessor &P = sema->getPreprocessor();      
      auto BL = D->getLocation();
      if (BL.isValid()) {
        auto EL = P.getLocForEndOfToken(BL).getLocWithOffset(-1);
        auto QTNS = D->getQualifiedNameAsString();

        if (QTNS == fromTypeQualifiedName) {
          rewriter.ReplaceText(SourceRange(BL, EL), toTypeName);        
        }
      }

      // then we handle the case of T
      processTypeLoc(D->getTypeSourceInfo()->getTypeLoc());
    }
    else {
      // TODO: handle Objective-C types
    }

    // descend into the next level (namespace, etc.)    
    if (auto innerDC = dyn_cast<DeclContext>(*I)) {
      processDeclContext(innerDC);
    }
  }
  popIndent();
}

void TypeRenameTransform::processStmt(Stmt *S)
{
  if (!S) {
    return;
  }
  
  pushIndent();
  // llvm::errs() << indent() << "Stmt: " << S->getStmtClassName() << ", at: "<< loc(S->getLocStart()) << "\n";
  
  if (auto CE = dyn_cast<CallExpr>(S)) {
    auto D = CE->getCalleeDecl();
    if (D) {
      // llvm::errs() << indent() << D->getDeclKindName() << "\n";
    }
  }
  else if (auto NE = dyn_cast<CXXNewExpr>(S)) {
    processTypeLoc(NE->getAllocatedTypeSourceInfo()->getTypeLoc());  
  }
  else if (auto NE = dyn_cast<ExplicitCastExpr>(S)) {
    processTypeLoc(NE->getTypeInfoAsWritten()->getTypeLoc());  
  }
  else {
    // TODO: Fill in other Stmt/Expr that has type info
    // TODO: Verify correctness and furnish test cases
  }
  
  for (auto I = S->child_begin(), E = S->child_end(); I != E; ++I) {
    processStmt(*I);
  }
  
  popIndent();
}

void TypeRenameTransform::processTypeLoc(TypeLoc TL)
{
  processTypeLoc(TL, TL);
}

void TypeRenameTransform::processTypeLoc(TypeLoc TL, TypeLoc &headTL)
{
  if (TL.isNull()) {
    return;
  }
  
  auto BL = TL.getBeginLoc();
  
  // TODO: ignore system headers
  
  // is a result from macro expansion? sorry...
  if (BL.isMacroID()) {
    
    // TODO: emit error diagnostics
    
    // llvm::errs() << "Cannot rename type from macro expansion at: " << loc(BL) << "\n";
    return;
  }

  pushIndent();
  auto QT = TL.getType();
    
  // llvm::errs() << indent()
  //   << "TypeLoc"
  //   << ", typeLocClass: " << typeLocClassName(TL.getTypeLocClass())
  //   << "\n" << indent() << "qualType as str: " << QT.getAsString()
  //   << "\n" << indent() << "beginLoc: " << loc(TL.getBeginLoc())
  //   << "\n";
    
  switch(TL.getTypeLocClass()) {
    case TypeLoc::TypeLocClass::TemplateSpecialization:
    {
      if (auto TSTL = dyn_cast<TemplateSpecializationTypeLoc>(&TL)) {
        
        // TODO: See if it's the template name that needs renaming

        // iterate through the args

        for (unsigned I = 0, E = TSTL->getNumArgs(); I != E; ++I) {
          
          // need to see if the template argument is also a type
          // (we skip things like Foo<1> )
          auto AL = TSTL->getArgLoc(I);
          auto A = AL.getArgument();
          if (A.getKind() != TemplateArgument::ArgKind::Type) {
            continue;
          }
          processTypeLoc(AL.getTypeSourceInfo()->getTypeLoc());
        }
      }
      break;
    }
    
    // non-leaf types
    case TypeLoc::TypeLocClass::Typedef:
    // all leaf types
    case TypeLoc::TypeLocClass::Enum:    
    {
      // Typedef/Enum needs special handling because the leaf node
      // in not a full name (i.e. no namespace info)
      // so we have to do this
      TypeLoc ITL = headTL;
      auto ITLQT = headTL.getType();
      do {
        llvm::errs() << indent()
          << "TypeLoc"
          << ", typeLocClass: " << typeLocClassName(ITL.getTypeLocClass())
          << "\n" << indent() << "qualType as str: " << ITLQT.getAsString()
          << "\n" << indent() << "beginLoc: " << loc(ITL.getBeginLoc())
          << "\n";
        
        if (ITLQT.getAsString() == fromTypeQualifiedName) {
          Preprocessor &P = sema->getPreprocessor();
          auto EL = P.getLocForEndOfToken(BL).getLocWithOffset(-1);
          rewriter.ReplaceText(SourceRange(BL, EL), toTypeName);

          llvm::errs() << "renamed: " << loc(BL) << "\n";
          break;          
        }
        ITL = ITL.getNextTypeLoc();
      } while (ITL != TL && !ITL.isNull());
      
      break; // case
    }
    
    // leaf types
    // TODO: verify correctness, need test cases for each    
    // TODO: Check if Builtin works
    case TypeLoc::TypeLocClass::Builtin:
    case TypeLoc::TypeLocClass::Record:
    case TypeLoc::TypeLocClass::InjectedClassName:
    case TypeLoc::TypeLocClass::ObjCInterface:
    case TypeLoc::TypeLocClass::TemplateTypeParm:
    {
      // TODO: Correct way of comparing type?
      if (QT.getAsString() == fromTypeQualifiedName) {
        
        Preprocessor &P = sema->getPreprocessor();
        auto EL = P.getLocForEndOfToken(BL).getLocWithOffset(-1);
        rewriter.ReplaceText(SourceRange(BL, EL), toTypeName);

        llvm::errs() << "renamed: " << loc(BL) << "\n";
      }
      break;
    }
      
      
    default:
      break;
  }
  
  processTypeLoc(TL.getNextTypeLoc(), headTL);

  popIndent();
}

void TypeRenameTransform::writeOutput()
{
  // output rewritten files
  // TODO: move this to a utility class
  for (auto I = rewriter.buffer_begin(), E = rewriter.buffer_end();
       I != E; ++I) {

    auto F = rewriter.getSourceMgr().getFileEntryForID(I->first);
    std::string outName(F->getName());
    size_t ext = outName.rfind(".");
    if (ext == std::string::npos) {
      ext = outName.length();
    }

    outName.insert(ext, ".refactored");

    // TODO: Use diagnostics for error report?
    llvm::errs() << "Output to: " << outName << "\n";
    std::string outErrInfo;

    llvm::raw_fd_ostream outFile(outName.c_str(), outErrInfo, 0);

    if (outErrInfo.empty()) {
      // output rewritten source code
      auto RB = &I->second;
      outFile << std::string(RB->begin(), RB->end());
    }
    else {
      llvm::errs() << "Cannot open " << outName << " for writing\n";
    }

    outFile.close();
  }  
}
