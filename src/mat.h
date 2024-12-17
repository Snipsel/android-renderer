#pragma once
#include<cmath>

struct vec2{
    float x,y;
};

struct vec3{
    float x,y,z;
};
constexpr vec3 operator+(vec3 a, vec3 b){ return {a.x+b.x, a.y+b.y, a.z+b.z}; }
constexpr vec3 operator-(vec3 a, vec3 b){ return {a.x-b.x, a.y-b.y, a.z-b.z}; }
constexpr vec3 operator/(vec3 a, float o){ return {a.x/o, a.y/o, a.z/o}; }
constexpr float dot(vec3 a, vec3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
constexpr float length_sq(vec3 a){ return dot(a,a); }
constexpr float length(vec3 a){ return sqrtf(length_sq(a)); }
constexpr vec3  normalize(vec3 a){ return a/length(a); }
constexpr vec3  cross(vec3 a, vec3 b){ 
    return { a.y*b.z - b.y*a.z,
             a.z*b.x - b.z*a.x,
             a.x*b.y - b.x*a.y };
}

struct vec4{ 
    float x,y,z,w;
    constexpr vec4() = default;
    constexpr vec4(float x, float y, float z, float w):x(x), y(y), z(z), w(w){}
    constexpr vec4(vec3 v, float w):x(v.x), y(v.y), z(v.z), w(w){}
};
constexpr float dot(vec4 a, vec4 b){ return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

struct mat2{
    float elems[2*2];
    constexpr mat2(float ax, float bx,
                   float ay, float by)
        :elems{ax, ay,
               bx, by}{}
};

struct mat4{
    float elems[4*4];
    constexpr mat4() = default;
    constexpr mat4(float ax, float bx, float cx, float dx,
                   float ay, float by, float cy, float dy,
                   float az, float bz, float cz, float dz,
                   float aw, float bw, float cw, float dw)
        :elems{ax, ay, az, aw,
               bx, by, bz, bw,
               cx, cy, cz, cw,
               dx, dy, dz, dw}{};
    constexpr explicit mat4(mat2 m)
        :elems{m.elems[0], m.elems[1], 0, 0,
               m.elems[2], m.elems[3], 0, 0,
               0,          0,          1, 0,
               0,          0,          0, 1}{}
    constexpr float& operator()(int col, int row)       { return elems[4*col+row]; }
    constexpr float  operator()(int col, int row) const { return elems[4*col+row]; }
    constexpr vec4&  operator()(int col)       { return *(vec4*)(elems+4*col); }
    constexpr vec4   operator()(int col) const { return *(vec4*)(elems+4*col); }
};

constexpr mat4 operator*(mat4 const& a, mat4 const& b){
    mat4 ret;
    for(int i=0; i<4; i++){
        for(int j=0; j<4; j++){
            ret.elems[4*i+j] = 0;
            for(int k=0; k<4; k++){
                ret.elems[4*i+j] += b.elems[4*i+k] * a.elems[4*k+j];
            }
        }
    }
    return ret;
}


constexpr vec4 operator*(mat4 const& l, vec4 const& r){
    return vec4{ dot(l(0), r), dot(l(1), r), dot(l(2), r), dot(l(3), r) };
}

constexpr mat4 transpose(mat4 const& m){
    return mat4(m.elems[ 0], m.elems[ 4], m.elems[ 8], m.elems[12],
                m.elems[ 1], m.elems[ 5], m.elems[ 9], m.elems[13],
                m.elems[ 2], m.elems[ 6], m.elems[10], m.elems[14],
                m.elems[ 3], m.elems[ 7], m.elems[11], m.elems[15]);
}

mat4 translate(vec3 p){
    return mat4(
        1, 0, 0, p.x,
        0, 1, 0, p.y,
        0, 0, 1, p.z,
        0, 0, 0, 1);
}

mat4 look_at(vec3 eye, vec3 target, vec3 global_up={0,1,0}){
    vec3 const b = normalize(eye-target);
    vec3 const r = normalize(cross(global_up, b));
    vec3 const u = cross(b,r);
    return mat4(
        r.x, r.y, r.z, -dot(eye,r),
        u.x, u.y, u.z, -dot(eye,u),
        b.x, b.y, b.z, -dot(eye,b),
        0,   0,   0,   1);
};

mat4 ortho(float left, float right, float bottom, float top, float near, float far){
    float const sx = 2.f/(right-left);
    float const sy = 2.f/(bottom-top);
    float const sz = 1.f/(near-far);
    float const tx = -(right+left)/(right-left);
    float const ty = -(bottom+top)/(bottom-top);
    float const tz =  near/(near-far);
    return mat4(
        sx, 0,  0,  tx,
        0,  sy, 0,  ty,
        0,  0,  sz, tz,
        0,  0,  0,  1);
}

mat4 perspective(float aspect, float fovy, float near){
    float const sy = 1.f/tanf(fovy/2.f); // focal length
    float const sx = sy/aspect;
    return mat4(
        sx, 0,  0,  0,
        0,  sy, 0,  0,
        0,  0,  0,  near,
        0,  0, -1,  0
    );
}

struct mat3{
    float elems[3*3];
    constexpr mat3() = default;
    constexpr mat3(mat4 m)
        :elems{m(0,0), m(0,1), m(0,2),
               m(1,0), m(1,1), m(1,2),
               m(2,0), m(2,1), m(2,2)}{};
};
