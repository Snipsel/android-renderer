#version 450
layout(location=0) in  vec3 in_pos;
layout(location=1) in  vec2 in_uv;
layout(location=2) in  vec3 in_normal;
layout(location=3) in  vec3 in_tangent;
layout(location=4) in  vec3 in_bitangent;

layout(location=0) out vec2 out_uv;
layout(location=1) out vec3 out_normal;
layout(location=2) out vec3 out_tangent;
layout(location=3) out vec3 out_bitangent;
layout(location=4) out vec3 out_view;

layout(push_constant) uniform _push {
    mat4 mvp;
    mat4 model;
    vec4 eye;
} push;

void main(){
    gl_Position = push.mvp * vec4(in_pos.xyz, 1);
    // mat3 adjugate = mat3(
    //     cross(push.mv[1].xyz, push.mv[2].xyz),
    //     cross(push.mv[2].xyz, push.mv[0].xyz),
    //     cross(push.mv[0].xyz, push.mv[1].xyz));
    out_uv         = in_uv;
    out_normal     = mat3(push.model)*in_normal;
    out_tangent    = mat3(push.model)*in_tangent;
    out_bitangent  = mat3(push.model)*in_bitangent;
    out_view = push.eye.xyz - (push.model*vec4(in_pos,1)).xyz;
}
