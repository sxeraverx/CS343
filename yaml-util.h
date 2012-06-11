#include <yaml-cpp/yaml.h>

#ifndef YAML_UTIL
#define YAML_UTIL

template<>
struct YAML::convert<YAML::Node> {
	static YAML::Node encode(const YAML::Node& rhs) {return rhs;}
	static bool decode(const YAML::Node& node, YAML::Node& rhs) {rhs=node; return true;}
};

#endif //YAML_UTIL
