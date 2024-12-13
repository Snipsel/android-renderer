#include "include.h"

int64_t timediff(struct timespec a, struct timespec b){
    return (b.tv_sec-a.tv_sec)*1'000'000'000 + (int64_t(b.tv_nsec)-int64_t(a.tv_nsec));
}

static bool loop_running;
void start_loop(ANativeActivity*){
    debug("we're live!");
    Android::request_quit = false;
    loop_running = true;
    pthread_create(&Android::main_thread, NULL, application_thread, (void*)nullptr);
}

void stop_loop(){
    debug("requesting quit");
    atomic_store(&Android::request_quit, true);
    pthread_join(Android::main_thread, NULL);
    loop_running = false;
    debug("child stopped");
}

void init_window_callback(ANativeActivity* activity, ANativeWindow* window){
    debug("");
    struct timespec ts_begin;
    clock_gettime(CLOCK_REALTIME, &ts_begin);

    create_window(vk::win, window);

    struct timespec ts_mid;
    clock_gettime(CLOCK_REALTIME, &ts_mid);
    int64_t dt1 = timediff(ts_begin, ts_mid);
    debug("window      init time: %ld.%06ld ms", dt1/1'000'000, dt1%1'000'000);

    Android::window = window;
    if(Android::queue)
        start_loop(activity);
}

void destroy_window_callback(ANativeActivity*, ANativeWindow*){
    debug("");
    if(loop_running)
        stop_loop();
    vkDeviceWaitIdle(vk::device);
    destroy_window(vk::win);
    Android::window = nullptr;
}

void init_input_callback(ANativeActivity* activity, AInputQueue* queue){
    debug("");
    Android::queue = queue;
    if(vk::win.surface)
        start_loop(activity);
}

void destroy_input_callback(ANativeActivity*, AInputQueue*){
    debug("");
    if(loop_running)
        stop_loop();
    Android::queue = nullptr;
}

extern "C" JNIEXPORT
void ANativeActivity_onCreate(ANativeActivity* activity, 
                              void*  /*savedState*/, 
                              size_t /*savedStateSize*/){
    struct timespec ts_on_create;
    clock_gettime(CLOCK_REALTIME, &ts_on_create);
    debug("============================================================");

    damaged_helmet.geometry_asset = AAssetManager_open(activity->assetManager, damaged_helmet.filename, AASSET_MODE_BUFFER);
    damaged_helmet.albedo_asset   = AAssetManager_open(activity->assetManager, damaged_helmet.albedo,   AASSET_MODE_BUFFER);
    //gizmo.asset          = AAssetManager_open(activity->assetManager, gizmo.filename, AASSET_MODE_BUFFER);

    if(vk::device == VK_NULL_HANDLE)
        init_vulkan_device();

    activity->callbacks->onNativeWindowCreated   = init_window_callback;
    activity->callbacks->onNativeWindowDestroyed = destroy_window_callback;
    activity->callbacks->onInputQueueCreated     = init_input_callback;
    activity->callbacks->onInputQueueDestroyed   = destroy_input_callback;

    activity->callbacks->onWindowFocusChanged    = [](ANativeActivity*, int has_focus){
        if(has_focus){
            debug("focus gained");
        }else{
            debug("focus lost");
            atomic_store(&Android::request_quit, true);
        }
        atomic_store(&Android::has_focus, (bool)has_focus);
    };
    activity->callbacks->onStart   = [](ANativeActivity*){ debug("start");   };
    activity->callbacks->onResume  = [](ANativeActivity*){ debug("resume");  };
    activity->callbacks->onPause   = [](ANativeActivity*){ debug("pause");   atomic_store(&Android::has_focus, false);};
    activity->callbacks->onStop    = [](ANativeActivity*){ debug("stop");    };
    activity->callbacks->onDestroy = [](ANativeActivity*){ debug("destroy"); };
    activity->callbacks->onConfigurationChanged = [](ANativeActivity*){ debug("config change"); };
    activity->callbacks->onNativeWindowResized  = [](ANativeActivity*, ANativeWindow*){ debug("window resize"); };
    // activity->callbacks->onSaveInstanceState        = on_save_instance_state;
    // activity->callbacks->onNativeWindowRedrawNeeded = on_native_window_redraw_needed;
    // activity->callbacks->onContentRectChanged       = on_content_rect_changed;
    // activity->callbacks->onLowMemory                = on_low_memory;
    debug("onCreate done");
}

