#pragma once
#include<android/native_window.h>
#include<jni.h>
#include<android/native_activity.h>
#include<signal.h>
#include<android/log.h>
#include<cstdlib>
#include<dlfcn.h>
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_ANDROID_KHR
#include<vulkan/vulkan.h>
#include"vkutil.h"
#include"global.h"

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
