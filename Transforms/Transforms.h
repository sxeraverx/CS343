#include <string>
#include <vector>

#include <clang/Rewrite/Rewriter.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/SemaConsumer.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/CompilerInstance.h>

#include <yaml-cpp/yaml.h>

class Transform : public clang::SemaConsumer
{
public:
	~Transform();
protected:
	clang::Sema *sema;
	clang::Rewriter rewriter;
	virtual void InitializeSema(clang::Sema &s) override;
	friend class TransformFactory;
};

template <typename T> Transform* transform_factory()
{
	return new T;
}

typedef Transform* (*transform_creator)(void);

class TransformRegistry
{
 private:
	std::map<std::string,transform_creator> m_transforms;
 public:
	YAML::Node config;
	static TransformRegistry& get();
	void add(const std::string &, transform_creator);
	const transform_creator operator[](const std::string &name) const;
};

class TransformRegistration
{
public:
	TransformRegistration(const std::string& name, transform_creator creator) {
		TransformRegistry::get().add(name, creator);
	}
};

#define REGISTER_TRANSFORM(transform)	  \
	TransformRegistration _transform_registration_ \
	## transform(#transform, &transform_factory<transform>)

class TransformFactory : public clang::tooling::FrontendActionFactory {
private:
	transform_creator tcreator;
public:
	TransformFactory(transform_creator creator);
	clang::FrontendAction *create() override;
};
