#include <string>
#include <vector>

#include <clang/Rewrite/Rewriter.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/SemaConsumer.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/CompilerInstance.h>

class clang::CompilerInstance;

class Transform : public clang::SemaConsumer
{
protected:
	clang::Sema *sema;
	clang::Rewriter rewriter;
	virtual std::string getName() = 0;
	virtual void InitializeSema(clang::Sema &s) override {sema = &s; rewriter.setSourceMgr(this->sema->getSourceManager(), sema->getLangOpts());}

};

template <typename T> Transform* transform_factory()
{
	return new T;
}

typedef Transform* (*transform_creator)(void);

class TransformRegistry
{
 private:
	std::vector<transform_creator> m_transforms;
 public:
	typedef std::vector<transform_creator>::iterator iterator;
	static TransformRegistry& get();
	void add(transform_creator);
	iterator begin();
	iterator end();
};

class TransformRegistration
{
 public:
	TransformRegistration(transform_creator);
};

#define REGISTER_TRANSFORM(transform)	  \
	TransformRegistration _transform_registration_ \
	## transform(&transform_factory<transform>)

#define DEFINE_TRANSFORM_BEGIN(transform)	  \
	class transform##Transform : public Transform { \
public:	  \
 std::string getName() override {return #transform "Transform"; }

#define DEFINE_TRANSFORM_END(transform)	  \
	}; \
	REGISTER_TRANSFORM(transform##Transform)

class TransformFactory : public clang::tooling::FrontendActionFactory {
 private:
	transform_creator tcreator;
 public:
	TransformFactory(transform_creator creator);
	clang::FrontendAction *create() override;
};
