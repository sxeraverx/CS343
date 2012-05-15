//
// ClassRenameTransform.cpp
//

#include "Transforms.h"

#include <clang/Basic/SourceManager.h>
#include <clang/Sema/Sema.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;

class ClassRenameTransform : public Transform {
private:
  Sema *sema;
  SourceManager *sourceMgr;
  std::string fromClassFullName;
  std::string toClassName;
  bool enabled;
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
  
  const std::string captureTypeLocInfo(TypeLoc& TL) {
    const char* cdataBegin = sourceMgr->getCharacterData(TL.getBeginLoc());
    const char* cdataEnd = sourceMgr->getCharacterData(TL.getEndLoc());
    
    return std::string(cdataBegin, cdataEnd - cdataBegin + 1);    
  }

public:
  ClassRenameTransform() : enabled(false), indentLevel(0) {
    // TODO: Temporary measure of config
    const char* f = getenv("FROM_CLASS_NAME");
    const char* t = getenv("TO_CLASS_NAME");
    
    if (!f || !t) {
      llvm::errs() << "No FROM_CLASS_NAME or TO_CLASS_NAME specified"
        " -- skipping ClassRenameTransform\n";
      return;
    }
    
    enabled = true;
    fromClassFullName = f;
    toClassName = t;
    
    llvm::errs() << "ClassRenameTransform, from: " << fromClassFullName
      << ", to: "<< toClassName << "\n";
  }
  
  std::string getName() override {
    return "ClassRenameTransform";
  }
  
  void InitializeSema(Sema &S) override {
    this->sema = &S;
    this->sourceMgr = &(S.getSourceManager());
  }

  void HandleTranslationUnit(ASTContext &C) override {
    if (!enabled) {
      return;
    }
    
    const TranslationUnitDecl *TUD = C.getTranslationUnitDecl();
    renameClass(TUD);
  }
  
  void renameClass(const DeclContext *DC)
  {
    for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {
      // records
      if(const RecordDecl *RD = dyn_cast<RecordDecl>(*I)) {
        llvm::errs() << indent() << "RecordDecl " << RD->getQualifiedNameAsString() << "\n";
        pushIndent();
        
        // fields
        for(auto FI = RD->field_begin(), FE = RD->field_end();
            FI != FE; ++FI) {
          llvm::errs() << indent() << "FieldDecl " << FI->getType().getAsString() << " " << FI->getQualifiedNameAsString() << "\n";
        }
        
        // subtype: C++ records -- grab all its methods
        if (const CXXRecordDecl *CRD = dyn_cast<CXXRecordDecl>(RD)) {
          
          // methods, this include all ctors and dtors
          for(auto MI = CRD->method_begin(), ME = CRD->method_end();
              MI != ME; ++MI) {
            llvm::errs() << indent() << "CXXMethodDecl " << MI->getType().getAsString() << " " << MI->getQualifiedNameAsString() << "\n";

            if (MI->hasBody()) {
              CXXMethodDecl *M = &(*MI);
              
              if(DeclContext *innerDC = dyn_cast<DeclContext>(M)) {
                pushIndent();
                renameClass(innerDC);
                popIndent();
              }          
            }
          }
          
          // TODO: static members
        }
        
        popIndent();
      }
      else if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(*I)) {
        llvm::errs() << indent() << "FunctionDecl " << FD->getType().getAsString() << " " << FD->getQualifiedNameAsString() << "\n";        
        
        if (FD->hasBody()) {
          // refactor return type
          if (FD->getResultType().getAsString() == fromClassFullName) {
            auto TSI = FD->getTypeSourceInfo();
            auto TL = TSI->getTypeLoc();

            pushIndent();
            std::string speltType = captureTypeLocInfo(TL);
            llvm::errs() << indent() << "===> rename, spelling: " << speltType << "\n";
            popIndent();
          }
          
          
          if(const DeclContext *innerDC = dyn_cast<DeclContext>(*I)) {
            pushIndent();
            renameClass(innerDC);
            popIndent();
          }          
        }
      }
      else if (const DeclaratorDecl *DD = dyn_cast<DeclaratorDecl>(*I)) {
        llvm::errs() << indent() << "DeclaratorDecl " << DD->getType().getAsString() << " " << DD->getQualifiedNameAsString() << "\n";        

        // refactor value type
        if (DD->getType().getAsString() == fromClassFullName) {
          auto TSI = DD->getTypeSourceInfo();
          auto TL = TSI->getTypeLoc();
            
          pushIndent();
          std::string speltType = captureTypeLocInfo(TL);
          llvm::errs() << indent() << "===> need rename, spelling: " << speltType << "\n";
          popIndent();
        }
      }
      else if (isa<NamespaceDecl>(*I)) {
        NamespaceDecl *ND = dyn_cast<NamespaceDecl>(*I);
        llvm::errs() << indent() << "NamespaceDecl " << ND->getQualifiedNameAsString() << "\n";        
        //recurse into inner contexts
        if(const DeclContext *innerDC = dyn_cast<DeclContext>(*I)) {
          pushIndent();
          renameClass(innerDC);
          popIndent();
        }
      }
    }
  }
};

REGISTER_TRANSFORM(ClassRenameTransform);
