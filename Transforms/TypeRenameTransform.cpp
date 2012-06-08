//
// TypeRenameTransform.cpp: Rename C/C++/Objective-C/Objective-C++ Types
//

#include "Transforms.h"
#include "RenameTransforms.h"

using namespace clang;

// TODO: Instead of using string match, use a two-pass approach
// pass 1: find the Decl that needs to be renamed
// pass 2: use TypeLoc's type info to find if it's the original Decl
// reason: need a way to handle in-function classes
// (the reas name of those classes do not show up in the TypeLoc's
// qualified name)
// issue: what about forward-decl'd types (class A; A* b;) ? 

class TypeRenameTransform : public RenameTransform {
public:
  virtual void HandleTranslationUnit(ASTContext &C) override;
  
protected:
  void collectRenameDecls(DeclContext *DC, bool topLevel = false);
  void processDeclContext(DeclContext *DC, bool topLevel = false);  
  void processStmt(Stmt *S);
  
  // forceRewriteMacro is needed to handle expressions like VAArgExpr
  // TODO: be smart, if TL is not within a marco, it's do-able
  void processTypeLoc(TypeLoc TL, bool forceRewriteMacro = false);
  
  void processQualifierLoc(NestedNameSpecifierLoc NNSL,
                           bool forceRewriteMacro = false);
  void processParmVarDecl(ParmVarDecl *P);
  bool tagNameMatches(TagDecl *T);
  
private:
  // a quick way to get the whole TypeLocClass tree
  static std::string typeLocClassName(TypeLoc::TypeLocClass C) {
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
  auto I = loadConfig("TypeRename", "Types");
  if (!I) {
    return;
  }

  auto TUD = C.getTranslationUnitDecl();
  collectRenameDecls(TUD, true);
  processDeclContext(TUD, true);
}

void TypeRenameTransform::collectRenameDecls(DeclContext *DC, bool topLevel)
{
  // TODO: Skip globally touched locations
  //
  // if a.cpp and b.cpp both include c.h, then once a.cpp is processed,
  // we cas skip any location that is not in b.cpp
  //
  
  pushIndent();
  
  for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {
    auto L = (*I)->getLocation();
    if (topLevel && shouldIgnore(L)) {
      continue;
    }
    
    if (auto TD = dyn_cast<TagDecl>(*I)) {
      std::string newName;
      if (nameMatches(TD, newName)) {
        renameLocation(L, newName);
      }
    }
    else if (auto D = dyn_cast<TypedefDecl>(*I)) {
      // typedef T n -- we want to see first if it's n that needs renaming
      std::string newName;
      if (nameMatches(D, newName)) {
        renameLocation(L, newName);
      }
    }

    // descend into the next level (namespace, etc.)    
    if (auto innerDC = dyn_cast<DeclContext>(*I)) {
      collectRenameDecls(innerDC);
    }
  }
  popIndent();  
}

// TODO: A special case for top-level decl context
// (because we can quickly skip system/refactored nodes there)
void TypeRenameTransform::processDeclContext(DeclContext *DC, bool topLevel)
{  
  // TODO: Skip globally touched locations
  //
  // if a.cpp and b.cpp both include c.h, then once a.cpp is processed,
  // we cas skip any location that is not in b.cpp
  //

  pushIndent();
  
  for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {
    auto L = (*I)->getLocation();
    if (topLevel && shouldIgnore(L)) {
      continue;
    }

    if (auto TD = dyn_cast<TagDecl>(*I)) {
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

      // handle ctor name initializers
      if (auto CD = dyn_cast<CXXConstructorDecl>(D)) {
        auto BL = CD->getLocation();
        std::string newName;
        if (BL.isValid() && CD->getParent()->getLocation() != BL && 
            nameMatches(CD->getParent(), newName, true)) {
          renameLocation(BL, newName);
        }
        
        for (auto II = CD->init_begin(), IE = CD->init_end(); II != IE; ++II) {
          auto X = (*II)->getInit();
          if (X) {
            processStmt(X);
          }
        }
      }
      
      // dtor
      if (auto DD = dyn_cast<CXXDestructorDecl>(D)) {
        // if parent matches
        auto BL = DD->getLocation();
        std::string newName;
        if (BL.isValid() && DD->getParent()->getLocation() != BL &&
            nameMatches(DD->getParent(), newName, true)) {
        
          // can't use renameLocation since this is a tricy case        
        
          // need to use raw_identifier because Lexer::findLocationAfterToken
          // performs a raw lexing
          SourceLocation EL =
            Lexer::findLocationAfterToken(BL, tok::raw_identifier,
                                          sema->getSourceManager(),
                                          sema->getLangOpts(), false);


          // TODO: Find the right way to do this -- consider this a hack

          if (EL.isValid()) {
            // EL is 1 char after the dtor name ~Foo, so -1 == pos of 'o'
            SourceLocation NE = EL.getLocWithOffset(-1);
            
            // "~Foo" has length of 4, we want -3 == pos of 'F'
            SourceLocation NB =
              EL.getLocWithOffset(-(int)DD->getNameAsString().size() + 1);

            if (NB.isMacroID()) {
              // TODO: emit error
              llvm::errs() << "Warning: Token is resulted from macro expansion"
                " and is therefore not renamed, at: " << loc(NB) << "\n";
            }
            else {
            // TODO: Determine if it's a wrtiable file
        
            // TODO: Determine if the location has already been touched or
            // needs skipping (such as in refactoring API user's code, then
            // the API headers need no changing since later the new API will be
            // in place)              
              rewriter.ReplaceText(SourceRange(NB, NE), newName);        
            }
          }
        }
      }
      
      // rename the params' types
      for (auto PI = D->param_begin(), PE = D->param_end(); PI != PE; ++PI) {
        processParmVarDecl(*PI);
      }
      
      // the name itself
      processQualifierLoc(D->getQualifierLoc());
      
      // handle body
      if (D->hasBody()) {
        processStmt(D->getBody());
      }
    }
    else if (auto D = dyn_cast<VarDecl>(*I)) {
      if (auto TSI = D->getTypeSourceInfo()) {
        processTypeLoc(TSI->getTypeLoc());
      }
      
      processQualifierLoc(D->getQualifierLoc());      
      
      if (D->hasInit()) {
        processStmt(D->getInit());
      }
    }
    else if (auto D = dyn_cast<FieldDecl>(*I)) {
      if (auto TSI = D->getTypeSourceInfo()) {
        processTypeLoc(TSI->getTypeLoc());
      }
      
      processQualifierLoc(D->getQualifierLoc());      
    }
    else if (auto D = dyn_cast<TypedefDecl>(*I)) {
      // typedef T n, handle the case of T
      if (auto TSI = D->getTypeSourceInfo()) {
        processTypeLoc(TSI->getTypeLoc());
      }
    }
    else if (auto D = dyn_cast<ObjCMethodDecl>(*I)) {
      // if no type source info, it's a void f(void) function
      auto TSI = D->getResultTypeSourceInfo();
      if (TSI) {      
        processTypeLoc(TSI->getTypeLoc());
      }

      for (auto PI = D->param_begin(), PE = D->param_end(); PI != PE; ++PI) {        
        processParmVarDecl(*PI);
      }
      
      // handle body
      auto B = D->getBody();
      if (B) {
        processStmt(B);
      }
    }
    else if (auto D = dyn_cast<ObjCPropertyDecl>(*I)) {
      if (auto TSI = D->getTypeSourceInfo()) {
        processTypeLoc(TSI->getTypeLoc());
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
  else if (auto E = dyn_cast<CXXNewExpr>(S)) {
    processTypeLoc(E->getAllocatedTypeSourceInfo()->getTypeLoc());  
  }
  else if (auto E = dyn_cast<ExplicitCastExpr>(S)) {
    processTypeLoc(E->getTypeInfoAsWritten()->getTypeLoc());  
  }
  else if (auto E = dyn_cast<CXXTemporaryObjectExpr>(S)) {
    auto TSI = E->getTypeSourceInfo();
    if (TSI) {
      processTypeLoc(TSI->getTypeLoc());  
    }
  }
  else if (auto E = dyn_cast<VAArgExpr>(S)) {
    // TODO: This will be a problem if the arg is also a macro expansion...
    processTypeLoc(E->getWrittenTypeInfo()->getTypeLoc(), true);  
  }
  else if (auto E = dyn_cast<UnaryExprOrTypeTraitExpr>(S)) {
    // sizeof etc.
    if (E->isArgumentType()) {
      processTypeLoc(E->getArgumentTypeInfo()->getTypeLoc());  
    }
  }
  else {
    // TODO: Objective-C method call
    // TODO: Objective-C @encode, @protocol etc.
    // TODO: Fill in other Stmt/Expr that has type info
    // TODO: Verify correctness and furnish test cases
  }
  
  for (auto I = S->child_begin(), E = S->child_end(); I != E; ++I) {
    processStmt(*I);
  }
  
  popIndent();
}

void TypeRenameTransform::processTypeLoc(TypeLoc TL, bool forceRewriteMacro)
{
  if (TL.isNull()) {
    return;
  }
  
  auto BL = TL.getBeginLoc();
  
  // TODO: ignore system headers
  
  // is a result from macro expansion? sorry...
  if (BL.isMacroID() && !forceRewriteMacro) {
    
    // TODO: emit error diagnostics
    
    // llvm::errs() << "Cannot rename type from macro expansion at: " << loc(BL) << "\n";
    return;
  }
  
  // TODO: Take care of spelling loc finesses
  BL = sema->getSourceManager().getSpellingLoc(BL);

  pushIndent();
  auto QT = TL.getType();
    
  // llvm::errs() << indent()
  //   << "TypeLoc"
  //   << ", typeLocClass: " << typeLocClassName(TL.getTypeLocClass())
  //   << "\n" << indent() << "qualType as str: " << QT.getAsString()
  //   << "\n" << indent() << "beginLoc: " << loc(TL.getBeginLoc())
  //   << "\n";
    
  switch(TL.getTypeLocClass()) {
    
    // an elaborated type loc captures the "prefix" of a type
    // for example, the elaborated type loc of "A::B::C" is A::B
    // we need to know if A::B and A are types we are renaming
    // (so that we can handle nested classes, in-class typedefs, etc.)
    case TypeLoc::TypeLocClass::Elaborated:
    {
      if (auto ETL = dyn_cast<ElaboratedTypeLoc>(&TL)) {
        processQualifierLoc(ETL->getQualifierLoc(), forceRewriteMacro);
      }
      break;
    }
    
    case TypeLoc::TypeLocClass::FunctionProto:
    {
      if (auto FTL = dyn_cast<FunctionTypeLoc>(&TL)) {
        for (unsigned I = 0, E = FTL->getNumArgs(); I != E; ++I) {
          processParmVarDecl(FTL->getArg(I));
        }
      }
      break;
    }
    
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
          
          if (auto TSI = AL.getTypeSourceInfo()) {
            processTypeLoc(TSI->getTypeLoc(), forceRewriteMacro);
          }
        }
      }
      break;
    }
    
    // typedef is tricky
    case TypeLoc::TypeLocClass::Typedef:
    {
      auto T = TL.getTypePtr();
      if (auto TDT = dyn_cast<TypedefType>(T)) {
        auto TDD = TDT->getDecl();
        std::string newName;
        if (nameMatches(TDD, newName, true)) {
          renameLocation(BL, newName);
        }
      }
      
      break;
    }
    
    // leaf types
    // TODO: verify correctness, need test cases for each    
    // TODO: Check if Builtin works
    case TypeLoc::TypeLocClass::Builtin:
    case TypeLoc::TypeLocClass::Enum:    
    case TypeLoc::TypeLocClass::Record:
    case TypeLoc::TypeLocClass::InjectedClassName:
    case TypeLoc::TypeLocClass::ObjCInterface:
    case TypeLoc::TypeLocClass::TemplateTypeParm:
    {
      // skip if it's an anonymous type
      // read Clang`s definition (in RecordDecl) -- not exactly what you think
      // so we use the length of name

      std::string newName;      
      if (auto TT = dyn_cast<TagType>(TL.getTypePtr())) {
        auto TD = TT->getDecl();
        if (nameMatches(TD, newName, true)) {
          renameLocation(BL, newName);          
        }
      }
      else {
        if (stringMatches(QT.getAsString(), newName)) {
          renameLocation(BL, newName);
        }
      }
      break;
    }
      
      
    default:
      break;
  }
  
  processTypeLoc(TL.getNextTypeLoc(), forceRewriteMacro);
  popIndent();
}

// fix the "B" part of names like A::B::C if B is our target
void TypeRenameTransform::processQualifierLoc(NestedNameSpecifierLoc NNSL,
                                              bool forceRewriteMacro)
{
  while (NNSL) {
    auto NNS = NNSL.getNestedNameSpecifier();
    auto NNSK = NNS->getKind();
    if (NNSK == NestedNameSpecifier::TypeSpec || NNSK == NestedNameSpecifier::TypeSpecWithTemplate) {
      processTypeLoc(NNSL.getTypeLoc(), forceRewriteMacro);
    }
    
    NNSL = NNSL.getPrefix();
  }  
}

void TypeRenameTransform::processParmVarDecl(ParmVarDecl *P)
{
  auto PTSI = P->getTypeSourceInfo();
  if (PTSI) {
    processTypeLoc(PTSI->getTypeLoc());
  }
  
  // then the default vars
  if (P->hasInit()) {
    processStmt(P->getInit());
  }  
}
