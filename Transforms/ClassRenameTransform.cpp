//
// ClassRenameTransform.cpp
//

#include "Transforms.h"

#include <set>
#include <clang/Basic/SourceManager.h>
#include <clang/Sema/Sema.h>
#include <llvm/Support/raw_ostream.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Rewriter.h>

using namespace clang;

class ClassRenameTransform : public Transform {
private:
  std::set<FileID> rewriteIDSet;
  
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
    Preprocessor &P = sema->getPreprocessor();
    
    SourceLocation E = P.getLocForEndOfToken(TL.getEndLoc());
    const char* cdataBegin = sourceMgr->getCharacterData(TL.getBeginLoc());
    const char* cdataEnd = sourceMgr->getCharacterData(E);
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
  
  void HandleTranslationUnit(ASTContext &C) override {
    if (!enabled) {
      return;
    }
    
    sourceMgr = &sema->getSourceManager();
    
    const TranslationUnitDecl *TUD = C.getTranslationUnitDecl();
    renameClass(TUD);
    
    // output rewritten files
    for (std::set<FileID>::iterator i = rewriteIDSet.begin(), e = rewriteIDSet.end(); i != e; ++i) {
        const FileEntry* F = sema->getSourceManager().getFileEntryForID(*i);
        
        std::string outName(F->getName());
        size_t ext = outName.rfind(".");
        if (ext == std::string::npos) {
            ext = outName.length();
        }
        
        outName.insert(ext, "_out");
        
        llvm::errs() << "Output to: " << outName << "\n";
        std::string OutErrorInfo;
        
        llvm::raw_fd_ostream outFile(outName.c_str(), OutErrorInfo, 0);

        
        if (OutErrorInfo.empty())
        {
            // Now output rewritten source code
            const RewriteBuffer *RB = rewriter.getRewriteBufferFor(*i);
            outFile << std::string(RB->begin(), RB->end());
        }
        else
        {
            llvm::errs() << "Cannot open " << outName << " for writing\n";
        }

        outFile.close();
    }
  }
  
  void renameClass(const DeclContext *DC)
  {
    for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {
      // FullSourceLoc FSL((*I)->getLocStart(), sema->getSourceManager());
      // auto F = sema->getSourceManager().getFileEntryForID(FSL.getFileID());
      // std::string fn(F->getName());
      // if (fn.find("/usr") != std::string::npos) {
      //   continue;
      // }
      // 
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
        llvm::errs() << indent() << "DeclaratorDecl, type: " << DD->getType().getAsString() << ", name: " << DD->getQualifiedNameAsString() << "\n";        

        // refactor value type
        // TODO: need to do type inference and rewrite the type correctly
        if (DD->getType().getAsString() == fromClassFullName) {
          auto TSI = DD->getTypeSourceInfo();
          auto TL = TSI->getTypeLoc();
            
          pushIndent();
          std::string speltType = captureTypeLocInfo(TL);
          llvm::errs() << indent() << "===> need rename, spelling: " << speltType << "\n";
          popIndent();
          
          Preprocessor &P = sema->getPreprocessor();
          SourceLocation B = TL.getBeginLoc();
          SourceLocation E = P.getLocForEndOfToken(TL.getEndLoc());
          rewriter.ReplaceText(SourceRange(B, E), toClassName);
          
          FullSourceLoc FSL(B, *sourceMgr);
          rewriteIDSet.insert(FSL.getFileID());
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
