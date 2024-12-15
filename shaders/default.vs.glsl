#version 450
layout(location=0) in  vec3 pos;
layout(location=1) in  vec2 in_uv;
layout(location=2) in  vec3 in_normal;
layout(location=3) in  vec3 in_tangent;

layout(location=0) out vec2 out_uv;
layout(location=1) out vec3 out_normal;
layout(location=2) out vec3 out_tangent;

layout(push_constant) uniform _push {
    mat4 mvp;
    mat4 mv;
} push;

void main(){
    mat3 adjugate = mat3(
        cross(push.mv[1].xyz, push.mv[2].xyz),
        cross(push.mv[2].xyz, push.mv[0].xyz),
        cross(push.mv[0].xyz, push.mv[1].xyz));
    out_uv      = in_uv;
    out_normal  = normalize(adjugate*in_normal );
    out_tangent = normalize(adjugate*in_tangent);
    gl_Position = push.mvp * vec4(pos.xyz, 1);
}
