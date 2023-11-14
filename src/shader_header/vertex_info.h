#define LOCATION_VERTEX_IN_POSITION 0
#define LOCATION_VERTEX_IN_COLOR    1

#define LOCATION_VERTEX_OUT_COLOR 0

#define BINDING_UBO 0

struct UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
};
