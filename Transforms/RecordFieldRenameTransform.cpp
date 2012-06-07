//
// RecordFieldRenameTransform.cpp
//

#include "RenameTransforms.h"

using namespace clang;

class RecordFieldRenameTransform : public RenameTransform {
public:
  virtual void HandleTranslationUnit(ASTContext &) override;
  void collectAndRenameFieldDecl(DeclContext *DC);
  
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

