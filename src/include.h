#pragma once
#include<android/native_window.h>
#include<jni.h>
#include<android/native_activity.h>
#include<signal.h>
#include<android/log.h>
#include<cstdlib>
#include<dlfcn.h>
#include<signal.h>
#include<sys/mman.h>
#include<sched.h>
#include<unistd.h>
#include<time.h>
#include<sys/wait.h>
#include<stdatomic.h>
#include<pthread.h>
#include<fcntl.h>
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_ANDROID_KHR
#include<vulkan/vulkan.h>

template<typename T, int64_t N>
struct Arr{
    T elems[N];
    operator T*() const { return elems; }
    consteval int64_t len() const { return N; }
    constexpr T const* begin() const { return elems; }
    constexpr T      * begin()       { return elems; }
    constexpr T const* end()   const { return elems+N; }
    constexpr T      * end()         { return elems+N; }
    constexpr T const& operator[](int64_t i) const { return elems[i]; }
    constexpr T      & operator[](int64_t i)       { return elems[i]; }
};
// deduction guide
template<typename T, typename... Ts>
Arr(T, Ts...) -> Arr<T, 1 + sizeof...(Ts)>;

template<int64_t N, typename F>
auto mapi(F fn){
    Arr<decltype(fn(0)),N> ret;
    for(int64_t i=0; i<N; i++){
        ret[i] = fn(i);
    }
    return ret;
}

template<int64_t N, typename A, typename F>
auto map(Arr<A,N> const& a, F fn){
    using T = decltype(fn(*a.begin()));
    Arr<T,N> ret;
    for(int64_t i=0; i<N; i++){
        ret[i] = fn(a[i]);
    }
    return ret;
}

#include"mat.h"
#include"vkutil.h"
#include"global.h"
#include"../build/assets.h"


#define debug(fmt, ...) \
    __android_log_print(ANDROID_LOG_DEBUG, "snipsel", \
                        __FILE__ ":%d %s " fmt, __LINE__, __func__ \
                        __VA_OPT__(,) __VA_ARGS__)

#define fatal(fmt, ...) do{ \
    __android_log_print(ANDROID_LOG_FATAL, "snipsel", \
                        __FILE__ ":%d %s " fmt, __LINE__, __func__ \
                        __VA_OPT__(,) __VA_ARGS__); \
    raise(SIGTRAP); }while(0)

#define LEN(X) (sizeof(X)/sizeof(X[0]))
#define VKCHECK(fn) if(auto status = fn; status!=VK_SUCCESS) fatal("vulkan error(%d) " #fn, status);
#define EQCHECK(val,exp) if((val)!=(exp)) fatal("expected " #exp "(=%d), got %d", exp, val);

void vk_name(uint64_t handle, VkObjectType obj_type, char const * name){
    auto const info = VkDebugUtilsObjectNameInfoEXT{
        .sType=VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType=obj_type,
        .objectHandle=handle,
        .pObjectName=name,
    };
    vkSetDebugUtilsObjectNameEXT(vk::device, &info);
}

#include"window.h"
#include"loop.h"
#include"instance.h"
