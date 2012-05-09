#include <string>
#include <vector>

#include <clang/Sema/SemaConsumer.h>
#include <clang/Tooling/Tooling.h>

class Transform : public clang::SemaConsumer
{
public:
  virtual std::string getName() = 0;
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

#define REGISTER_TRANSFORM(transform) \
  TransformRegistration _transform_registration_ ## transform(&transform_factory<transform>);

class TransformFactory : public clang::tooling::FrontendActionFactory {
private:
  transform_creator tcreator;
public:
  TransformFactory(transform_creator creator);
  clang::FrontendAction *create() override;
};
