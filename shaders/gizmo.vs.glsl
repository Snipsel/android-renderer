#version 450

layout(location=0) out vec3 color;

layout(push_constant) uniform _push {
    mat4 view;
} push;


#define O vec3(0,0,0)
#define X vec3(0,0,1)
#define Y vec3(0,1,0)
#define Z vec3(1,0,0)

const vec3 positions[] = {
    O, Y, X, // bottom face
    O, X, Z, // back face
    O, Z, Y, // left face
    X, Y, Z  // diagonal face
};

void main(){
    vec3 vertex = positions[gl_VertexIndex];
    color = vertex;
    gl_Position = push.view*vec4(vertex, 1.0);
}
