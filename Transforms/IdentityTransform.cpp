//
// IdentityTransform.cpp
//

#include "Transforms.h"

using namespace clang;

class IdentityTransform : public Transform {
public:  
	virtual std::string getName() override { return "IdentityTransform"; }
	
  virtual void HandleTranslationUnit(ASTContext &) override;
};

REGISTER_TRANSFORM(IdentityTransform);

  
void IdentityTransform::HandleTranslationUnit(ASTContext &)
{
}
