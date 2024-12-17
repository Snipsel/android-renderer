#version 450
layout(location=0) in  vec2 in_uv;
layout(location=1) in  vec3 world_normal;
layout(location=2) in  vec3 world_tangent;
layout(location=3) in  vec3 world_bitangent;
layout(location=4) in  vec3 in_view;

layout(location=0) out vec4 out_color;

layout(binding=0) uniform sampler2D tex_albedo_roughness;
layout(binding=1) uniform sampler2D tex_normal_metal;

const float PI = 3.14159265359;

// Normal mapping
vec3 normal(vec3 normal_texel){
    mat3 tbn = mat3(
        normalize(world_tangent),
        normalize(world_bitangent),
        normalize(world_normal));
    vec3 tangent_space_normal = normalize(normal_texel*2.0-1.0);
    return normalize(tbn*tangent_space_normal);
}

float D_TrowbridgeReitz(float roughness2, float NdotH){
    float a2  = roughness2*roughness2;
	float tmp = 1.0+NdotH*NdotH*(a2-1);
	return a2/max(1e-3, PI*tmp*tmp);
}

float G_SchlickSmithGGX(float roughness2, float LdotN, float VdotN){
	float k = 0.5*roughness2;
	float GL = LdotN/max(1e-3, LdotN*(1-k)+k);
	float GV = VdotN/max(1e-3, VdotN*(1-k)+k);
	return GL*GV;
}

vec3 F_Schlick(vec3 F0, float VdotH){
    float p = 1.0-VdotH;
	return F0 + (1.0-F0)*(p*p*p*p*p);
}

vec3 BRDF(vec3 L, vec3 N, vec3 V, vec3 albedo, float metalic, float roughness){

    float VdotN = clamp(dot(V, N), 0.0, 1.0);
	float LdotN = clamp(dot(L, N), 0.0, 1.0);

    vec3  H = normalize(V+L); // halfway vector between view and light
    float LdotH = clamp(dot(L, H), 0.0, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float VdotH = clamp(dot(V, H), 0.0, 1.0);

    float roughness2 = max(1e-3, roughness*roughness);

    float D = D_TrowbridgeReitz(roughness2, NdotH);
    float G = G_SchlickSmithGGX(roughness2, LdotN, VdotN);
    float spec = (D*G)/(4.0*VdotN*LdotN);

    vec3  diff = albedo/PI;

	vec3 F0 = mix(vec3(0.04), albedo, metalic);
    vec3 F  = F_Schlick(F0, VdotH);
    return F*spec + (1.0-F)*diff;
}

void main() {
    vec4 albedo_roughness = texture(tex_albedo_roughness, in_uv);
    vec4 normal_metal     = texture(tex_normal_metal,     in_uv);
    vec3 N = normal(normal_metal.xyz);
    vec3 V = normalize(in_view);
    vec3 L = normalize(vec3(1,1,1));

    vec3 color = vec3(0);
    if(gl_FragCoord.y<((3120.0-1)/2.0)){

        float LdotN = clamp(dot(L,N), 0.0, 1.0);
        if(LdotN<=0){
            color = vec3(0);
        }else{
            vec3 brdf = BRDF(L, N, V, albedo_roughness.rgb, normal_metal.a, albedo_roughness.a);
            vec3 light_color = vec3(2.0,1.9,1.7);
            color = light_color*brdf*LdotN;
        }

    }else if(gl_FragCoord.y>((3120.0+1)/2.0)){

        float lambert = max(dot(N,L), 0.01);
        color = lambert*albedo_roughness.xyz;

    }else{
        color = vec3(1);
    }
    out_color = vec4(color, 1);
}
