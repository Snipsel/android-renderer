#version 450
layout(location=0) in  vec2 uv;
layout(location=1) in  vec3 normal;
layout(location=2) in  vec3 tangent;

layout(location=0) out vec4 out_color;

layout(binding=0) uniform sampler2D albedo;

void main() {
#if 1
    out_color = vec4(0.5*normal+0.5,1);
#elif 0
    out_color = vec4(0.5*tangent+0.5,1);
#else
    out_color = texture(albedo, uv);
#endif
}
