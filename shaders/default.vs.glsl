#version 450

layout(location=0) in  vec3 pos;
layout(location=1) in  vec2 in_uv;
layout(location=0) out vec2 out_uv;

layout(push_constant) uniform _push {
    mat4 view;
} push;

void main(){
    out_uv = in_uv;
    //gl_Position = view*vec4(positions[gl_VertexIndex], 0.0, 1.0);
    //gl_Position = vec4(pos.xyz, 1.0)*push.view;
    gl_Position = push.view * vec4(pos.xyz, 1);
}
