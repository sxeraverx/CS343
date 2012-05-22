//
// MethodMoveTransform.cpp: Batch method move
//

#include "Transforms.h"

#include <set>
#include <clang/Basic/SourceManager.h>
#include <clang/Sema/Sema.h>
#include <llvm/Support/raw_ostream.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Rewriter.h>

using namespace clang;

class MethodMoveTransform : public Transform {
public:  
	virtual std::string getName() override
	{
    return "MethodMoveTransform";
	}

  virtual void HandleTranslationUnit(ASTContext &C) override;
	
  
protected:
  std::set<FileID> rewriteIDSet;
  void processDeclContext(DeclContext *DC);
  void processCXXRecordDecl(CXXRecordDecl *CRD);
  void collectNamespaceInfo(DeclContext *DC, SourceLocation& EL,
                            std::string& outHeader,
                            std::string& outFooter);
  std::string writeImplMethodDecl(CXXMethodDecl *MD);
  
protected:
  // TODO: move these to a utility
  std::string captureSourceText(SourceRange R)
  {
    return captureSourceText(R.getBegin(), R.getEnd());
  }

  std::string captureSourceText(SourceLocation B, SourceLocation E);
};

REGISTER_TRANSFORM(MethodMoveTransform);

  
void MethodMoveTransform::HandleTranslationUnit(ASTContext &C)
{
    auto TUD = C.getTranslationUnitDecl();
    processDeclContext(TUD);
    
    // output rewritten files
    // TODO: move this to a utility class
    for (auto I = rewriteIDSet.begin(), E = rewriteIDSet.end(); I != E; ++I) {
        auto F = sema->getSourceManager().getFileEntryForID(*I);
        
        std::string outName(F->getName());
        size_t ext = outName.rfind(".");
        if (ext == std::string::npos) {
            ext = outName.length();
        }
        
        outName.insert(ext, ".refactored");
        
        llvm::errs() << "Output to: " << outName << "\n";
        
        std::string outErrorInfo;
        
        llvm::raw_fd_ostream outFile(outName.c_str(), outErrorInfo, 0);
        
        if (outErrorInfo.empty()) {
            // Now output rewritten source code
            auto RB = rewriter.getRewriteBufferFor(*I);
            outFile << std::string(RB->begin(), RB->end());
        }
        else {
            llvm::errs() << "Cannot open " << outName << " for writing\n";
        }

        outFile.close();
    }
  }
  
void MethodMoveTransform::processDeclContext(DeclContext *DC)
{  
  for(auto I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I) {    
    if(auto CRD = dyn_cast<CXXRecordDecl>(*I)) {
      processCXXRecordDecl(CRD);
    }
    else if (auto innerDC = dyn_cast<DeclContext>(*I)) {
      // descend into the next level (namespace, etc.)
      processDeclContext(innerDC);
    }
  }
}

void MethodMoveTransform::processCXXRecordDecl(CXXRecordDecl *CRD)
{
  // // TODO: handle nested class
  // processDeclContext(CRD);

  if (CRD->getQualifiedNameAsString() != "A::MyNameSpace::Foo") {
    return;
  }

  llvm::errs() << "CXXRecordDecl " << CRD->getQualifiedNameAsString() << "\n";
  
  // the right brace location of this CXXRecordDecl
  // nothing beyond that location is taken into consideration
  auto CRDRBL = CRD->getRBraceLoc();
  
  std::string sourceHeader;
  std::string sourceFooter;
  collectNamespaceInfo(CRD->getParent(), CRDRBL, sourceHeader, sourceFooter);


  // llvm::errs() << "header:\n" << sourceHeader << "\n\n";
  // llvm::errs() << "footer:\n" << sourceFooter << "\n\n";


  // methods, this include all ctors and dtors
  for (auto MI = CRD->method_begin(), ME = CRD->method_end();
       MI != ME; ++MI) {
      auto DNI = MI->getNameInfo();
      auto MN = DNI.getName().getAsString();
    
      if (0) {
        continue;
      }
    
      // we're only interested in inline, defined-in-class-decl methods
      if (!MI->hasBody()) {
        continue;
      }
    
      // body is a Stmt (compound statement)
      auto S = MI->getBody();
    
      // is the inline def outside of the record body? if not, skip
      // this also requires the check whether both are from the same file
      auto SLS = S->getLocStart();
      if (!sema->getSourceManager().isFromSameFile(CRDRBL, SLS)) {
        continue;
      }
      
      if (CRDRBL < SLS) {
        continue;
      }
    
      llvm::errs() << "handle " << MN << "\n";
    
      std::string&& b = writeImplMethodDecl(&*MI);
      llvm::errs() << b;

  }  
}

void MethodMoveTransform::collectNamespaceInfo(DeclContext *DC,
                                               SourceLocation& EL,
                                               std::string& outHeader,
                                               std::string& outFooter)                                               
{
  if (!DC) {
    return;
  }

  collectNamespaceInfo(DC->getParent(), EL, outHeader, outFooter);
  
  if (auto NSD = dyn_cast<NamespaceDecl>(DC)) {
    outHeader += "namespace ";
    outHeader += NSD->getNameAsString();
    outHeader += " {\n";
    
    std::string footer = "}; // namespace ";
    footer += NSD->getNameAsString();    
    footer += "\n";
    outFooter.insert(0, footer);
  }
  
  // TODO: Insert ident
  
  // TODO: Discuss on the mailing list, these iterators only gives the *last*
  // instance of using, e.g.:
  //
  //   namespace A {
  //     using namespace B;
  //     class Foo { /* ... */ };
  //     using namespace B;  // only this is given
  //   };
  //
  // but apparently class Foo is already using namespace B; this will cause
  // some problem for our namespace replication
  
  for (auto UI = DC->using_directives_begin(),
       UE = DC->using_directives_end(); UI != UE; ++UI) {
  
    auto UDLS = (*UI)->getLocStart();
    if (!sema->getSourceManager().isFromSameFile(EL, UDLS)) {
      continue;
    }
    
    if (EL < UDLS) {
      continue;
    }
         
    auto ND = (*UI)->getNominatedNamespaceAsWritten();
    auto N = ND->getNameAsString();
    
    outHeader += "using namespace ";
    outHeader += N;
    outHeader += ";\n";
  }
}

std::string MethodMoveTransform::writeImplMethodDecl(CXXMethodDecl *MD)
{
  std::string src;
  llvm::raw_string_ostream sst(src);
  
  // return type  
  sst << MD->getResultType().getAsString();
  sst << " ";

  // method name
  sst << MD->getParent()->getDeclName().getAsString();
  sst << "::";
  sst << MD->getDeclName().getAsString();
  
  // parameter list
  auto PI = MD->param_begin();
  auto PE = MD->param_end();
  auto LPE = PI;

  sst << "(";
  while (PI != PE) {

    SourceRange SR = (*PI)->getSourceRange();
    sst << captureSourceText(SR);
    
    if ((*PI)->hasDefaultArg()) {
      llvm::errs() << captureSourceText((*PI)->getDefaultArgRange());
    }


    LPE = PI;

    ++PI;
    if (PI != PE) {
      sst << ", ";
    }
  }

  sst << ")";

  
  
  sst << "\n\n";
  
  return sst.str();
}

std::string MethodMoveTransform::captureSourceText(SourceLocation B,
                                                   SourceLocation E)
{
  const char *cdataBegin = sema->getSourceManager().getCharacterData(B);
  const char *cdataEnd = rewriter.getSourceMgr().getCharacterData(E);
  return std::string(cdataBegin, cdataEnd - cdataBegin + 1);
}
