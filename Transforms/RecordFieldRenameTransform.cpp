//
// RecordFieldRenameTransform.cpp
//

#include "RenameTransforms.h"

using namespace clang;

class RecordFieldRenameTransform : public RenameTransform {
public:
  virtual void HandleTranslationUnit(ASTContext &) override;
};

REGISTER_TRANSFORM(RecordFieldRenameTransform);

void RecordFieldRenameTransform::HandleTranslationUnit(ASTContext &C)
{
}
