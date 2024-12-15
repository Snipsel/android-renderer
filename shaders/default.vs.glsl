#version 450
layout(location=0) in  vec3 pos;
layout(location=1) in  vec2 in_uv;
layout(location=2) in  vec3 in_normal;
layout(location=3) in  vec3 in_tangent;
layout(location=4) in  vec3 in_bitangent;

layout(location=0) out vec2 out_uv;
layout(location=1) out vec3 out_normal;
layout(location=2) out vec3 out_tangent;
layout(location=3) out vec3 out_bitangent;

layout(push_constant) uniform _push {
    mat4 mvp;
    mat4 model;
} push;

void main(){
    out_uv        = in_uv;
    out_normal    = mat3(push.model)*in_normal;
    out_tangent   = mat3(push.model)*in_tangent;
    out_bitangent = mat3(push.model)*in_bitangent;
    gl_Position = push.mvp * vec4(pos.xyz, 1);
}
