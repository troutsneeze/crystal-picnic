#ifndef SHADERS_H
#define SHADERS_H

#include <string>

namespace Wrap {
	class Shader;
}

namespace Shader
{

void use(Wrap::Shader *shader);
Wrap::Shader *get(std::string name);
void destroy(Wrap::Shader *shader);

}

#endif
