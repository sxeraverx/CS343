//
// IdentityTransform.cpp
//

#include "Transforms.h"

using namespace clang;

class IdentityTransform : public Transform {
public:
	
  virtual void HandleTranslationUnit(ASTContext &) override;
};

REGISTER_TRANSFORM(IdentityTransform);

  
void IdentityTransform::HandleTranslationUnit(ASTContext &)
{
}
