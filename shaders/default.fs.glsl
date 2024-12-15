#version 450
layout(location=0) in  vec2 in_uv;
layout(location=1) in  vec3 in_normal;
layout(location=2) in  vec3 in_tangent;
layout(location=3) in  vec3 in_bitangent;

layout(location=0) out vec4 out_color;

layout(binding=0) uniform sampler2D tex_albedo;
layout(binding=1) uniform sampler2D tex_normal;

void main() {
    vec2 screen = gl_FragCoord.xy/vec2(1440.0,3120.0);
    if(screen.y>0.5){
        out_color = vec4(0.5*in_normal+0.5, 1.0);
    }else{
        mat3 tbn = mat3(
            normalize(in_tangent),
            normalize(in_bitangent),
            normalize(in_normal));
        vec3 tangent_space_normal = normalize(texture(tex_normal, in_uv).xyz*2.0-1.0);
        vec3 normal = normalize(tbn*tangent_space_normal);
        out_color = vec4(0.5*normal+0.5, 1.0);
    }
}
