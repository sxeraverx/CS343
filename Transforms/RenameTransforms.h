#ifndef RENAME_TRANSFORMS_H
#define RENAME_TRANSFORMS_H

#include "Transforms.h"
#include <clang/Lex/Preprocessor.h>

class RenameTransform : public Transform {
public:
  RenameTransform() : indentLevel(0) {}
protected:
  // utility functions shared by all rename transforms
  
  void renameLocation(clang::SourceLocation L, std::string& N) {
    if (L.isValid()) {
      if (L.isMacroID()) {        
        // TODO: emit error using diagnostics
        llvm::errs() << "Warning: Token is resulted from macro expansion"
          " and is therefore not renamed, at: " << loc(L) << "\n";
      }
      else {
        clang::Preprocessor &P = sema->getPreprocessor();      
        auto LE = P.getLocForEndOfToken(L);
        if (LE.isValid()) {
          // getLocWithOffset returns the location *past* the token, hence -1
          auto E = LE.getLocWithOffset(-1);
          rewriter.ReplaceText(clang::SourceRange(L, E), N);
        }
      }
    }    
  }
    
  const std::string& indent() {    
    return indentString;
  }
  
  void pushIndent() {
    indentLevel++;
    indentString = std::string(indentLevel * 2, ' ');
  }
  
  void popIndent() {
    assert(indentLevel >= 0 && "indentLevel must be >= 0");
    indentLevel--;
    indentString = std::string(indentLevel * 2, ' ');
  }
  
  std::string loc(clang::SourceLocation L) {
    std::string src;
    llvm::raw_string_ostream sst(src);
    L.print(sst, sema->getSourceManager());
    return sst.str();
  }
  
private:
  int indentLevel;
  std::string indentString;
};

#endif
