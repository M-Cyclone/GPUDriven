#define BINDING_UBO     0
#define BINDING_SAMPLER 1

struct UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
};
