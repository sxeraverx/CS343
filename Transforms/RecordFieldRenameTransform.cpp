//
// RecordFieldRenameTransform.cpp
//

#include "RenameTransforms.h"
#include "pcrecpp.h"

using namespace clang;

class RecordFieldRenameTransform : public RenameTransform {
public:
  virtual void HandleTranslationUnit(ASTContext &) override;
  
protected:
  typedef std::pair<pcrecpp::RE, std::string> REStringPair;
  std::vector<REStringPair> renameList;
};

REGISTER_TRANSFORM(RecordFieldRenameTransform);

void RecordFieldRenameTransform::HandleTranslationUnit(ASTContext &C)
{
  auto S = TransformRegistry::get().config["RecordFieldRename"];
  if (!S.IsMap()) {
    llvm::errs() << "Error: Cannot find config entry \"RecordFieldRename\""
      " or entry is not a map\n";
    return;
  }
  
  auto IG = S["Ignore"];
  
  if (IG && !IG.IsSequence()) {
    llvm::errs() << "Error: Config key \"Ignore\" must be a sequence\n";
    return;
  }
  
  for (auto I = IG.begin(), E = IG.end(); I != E; ++I) {
    if (I->IsScalar()) {
      llvm::errs() << "Ignoring: " << I->as<std::string>() << "\n";
    }
  }
  
  auto RN = S["Fields"];
  if (!RN.IsSequence()) {
    llvm::errs() << "No fields specified or is not a sequence\n";
    return;
  }
  
  for (auto I = RN.begin(), E = RN.end(); I != E; ++I) {
    if (!I->IsMap()) {
      llvm::errs() << "Error: \"Fields\" contains non-map items\n";
      break;
    }
    
    for (auto MI = I->begin(), ME = I->end(); MI != ME; ++MI) {
      auto F = MI->first.as<std::string>();
      auto T = MI->second.as<std::string>();      
      pcrecpp::RE re(F);
      renameList.push_back(REStringPair(re, T));
      
      llvm::errs() << "renames: " << F << " -> " << T << "\n";
    }
  }
  
  
}
