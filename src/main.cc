#include "include.h"

void vk_name(uint64_t handle, VkObjectType obj_type, char const * name){
    auto const info = VkDebugUtilsObjectNameInfoEXT{
        .sType=VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType=obj_type,
        .objectHandle=handle,
        .pObjectName=name,
    };
    vkSetDebugUtilsObjectNameEXT(vk::device, &info);
}

VkShaderModule create_shader_module(uint32_t const* spv, size_t size_bytes){
    auto const info = VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size_bytes,
        .pCode    = spv,
    };
    VkShaderModule ret;
    VKCHECK(vkCreateShaderModule(vk::device, &info, nullptr, &ret));
    return ret;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                            VkDebugUtilsMessageTypeFlagsEXT type,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                            void*){
    const int level =
        (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0   ? ANDROID_LOG_ERROR:
        (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0 ? ANDROID_LOG_WARN :
                                                                            ANDROID_LOG_INFO ;
    char const general     = (type&VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)==0?'g':'-';
    char const validation  = (type&VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)==0?'v':'-';
    char const performance = (type&VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)==0?'p':'-';
    __android_log_print(level, "snipsel", "%c%c%c %s", general, validation, performance, data->pMessage);
    return VK_FALSE;
  }

void chain_pnext(VkBaseOutStructure* main_struct, VkBaseOutStructure* new_struct){
    new_struct->pNext  = main_struct->pNext;
    main_struct->pNext = new_struct;
}

// vulkan early initialization. We expect there to be only one gpu, so we don't need
// to wait for the window surface to see which one has display capabilities.
void vulkan_early_init(){
    auto const vulkan_lib = dlopen("libvulkan.so", RTLD_LAZY);
    if( vulkan_lib == nullptr ) fatal("could not load libvulkan.so");

    // load global functions
    auto const vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vulkan_lib, "vkGetInstanceProcAddr");
    if( vkGetInstanceProcAddr == nullptr ) fatal("could not load vkGetInstanceProcAddr");
    auto const vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(nullptr, "vkCreateInstance");
    auto const vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties");

    {
        uint32_t properties_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        VkExtensionProperties properties[64];
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties);
        for(int i=0; i<(int)properties_count; i++){
            debug("instance extension: %s", properties[i].extensionName);
        }
    }

    // create instance
    char const* const layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    char const* const instance_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };
    auto const applicationInfo = VkApplicationInfo{
        .pApplicationName   = "mini",
        .applicationVersion = 1,
        .pEngineName        = "mini",
        .engineVersion      = 1,
        .apiVersion         = VK_API_VERSION_1_3,
    };
    auto const instance_info = VkInstanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = LEN(layers),
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = LEN(instance_extensions),
        .ppEnabledExtensionNames = instance_extensions,
    };
    VKCHECK(vkCreateInstance(&instance_info, nullptr, &vk::instance))
    load_instance_funcs(vkGetInstanceProcAddr);

    #define LOCAL_FN(name) auto const name = (PFN_##name)vkGetInstanceProcAddr(vk::instance, #name);
    LOCAL_FN(vkCreateDebugUtilsMessengerEXT)
    LOCAL_FN(vkEnumeratePhysicalDevices)
    LOCAL_FN(vkGetDeviceProcAddr)
    LOCAL_FN(vkGetPhysicalDeviceQueueFamilyProperties)
    LOCAL_FN(vkCreateDevice)
    LOCAL_FN(vkEnumerateDeviceExtensionProperties)
    #undef LOCAL_FN

    {
        auto const info = VkDebugUtilsMessengerCreateInfoEXT{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = vulkan_debug_callback
        };
        VKCHECK(vkCreateDebugUtilsMessengerEXT(vk::instance, &info, nullptr, &vk::debug_messenger));
    }

    // just grab the first gpu
    uint32_t gpu_count = 1;
    VKCHECK(vkEnumeratePhysicalDevices(vk::instance, &gpu_count, &vk::gpu))

    {
        uint32_t prop_count = 0;
        VKCHECK(vkEnumerateDeviceExtensionProperties(vk::gpu, nullptr, &prop_count, nullptr));
        debug("%d device extensions", prop_count);
        VkExtensionProperties properties[256];
        VKCHECK(vkEnumerateDeviceExtensionProperties(vk::gpu, nullptr, &prop_count, properties));
        for(int i=0; i<(int)prop_count; i++){
            debug("device ext %3d: %s", i, properties[i].extensionName);
        }
    }

    // find graphics queue family
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk::gpu, &queue_family_count, nullptr);
    debug("queue family count: %u", queue_family_count);
    VkQueueFamilyProperties queue_families[8]; // 8 families should be enough
    vkGetPhysicalDeviceQueueFamilyProperties(vk::gpu, &queue_family_count, queue_families);
    vk::queue_family_index = UINT32_MAX;
    for(uint32_t i=0; i<queue_family_count; i++){
        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            vk::queue_family_index = i;
            break;
        }
    }
    if(vk::queue_family_index == UINT32_MAX) fatal("no graphics queue families found");

    // create device
    float queue_priority = 1.f;
    auto const queue_create_info = VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk::queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    auto const device_features = VkPhysicalDeviceFeatures{};
    char const * device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    auto const device_create_info = VkDeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = LEN(device_extensions),
        .ppEnabledExtensionNames = device_extensions,
        .pEnabledFeatures = &device_features,
    };
    VKCHECK(vkCreateDevice(vk::gpu, &device_create_info, nullptr, &vk::device))
    load_device_funcs(vkGetDeviceProcAddr);

    // cheeky local functions
    #define LOCAL_FN(name) auto const name = (PFN_##name)vkGetDeviceProcAddr(vk::device, #name);
    LOCAL_FN(vkGetDeviceQueue)
    #undef LOCAL_FN
    vkGetDeviceQueue(vk::device, vk::queue_family_index, 0, &vk::graphics_queue);
}

void record_frame(VkCommandBuffer cmd, uint32_t swapchain_idx){
    debug("beginning renderpass");
    VkClearValue const clear_value = {
        .color={
            .float32={0.3f,0.2f,0.4f,1.f}
        }
    };
    VkRenderPassBeginInfo const render_pass_begin_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = vk::render_pass,
        .framebuffer = vk::swapchain_framebuffers[swapchain_idx],
        .renderArea  = { .offset = {0,0}, .extent = vk::swapchain_extent, },
        .clearValueCount = 1,
        .pClearValues  = &clear_value,
    };
    vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    debug("binding pipeline");
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk::graphics_pipeline);

    debug("setting viewport");
    VkViewport const viewport{
        .x=0.f,
        .y=0.f,
        .width=(float)vk::swapchain_extent.width,
        .height=(float)vk::swapchain_extent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    debug("setting scissor");
    VkRect2D const scissor {
        .offset = {0,0},
        .extent = vk::swapchain_extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    debug("drawing");
    vkCmdDraw(cmd, 3, 1, 0, 0);

    debug("ending renderpass");
    vkCmdEndRenderPass(cmd);
}

// got all three: window, input and active
void finalize_init(ANativeActivity*){
    debug("we're live!");

    for(int frame=0; frame<5*60; frame++){
        AInputEvent* event;
        while(AInputQueue_getEvent(Android::queue, &event)>=0){
            AInputQueue_finishEvent(Android::queue, event, 1);
        }

        uint64_t const timeout = 1'000'000'000;

        VKCHECK(vkWaitForFences(vk::device, 1, &vk::ready_to_record, true, timeout));
        VKCHECK(vkResetFences(vk::device, 1, &vk::ready_to_record));

        uint32_t swapchain_idx = 0;
        while(true){
            VkResult status = vkAcquireNextImageKHR(vk::device, vk::swapchain, timeout, vk::ready_to_submit, nullptr, &swapchain_idx);
            if(status == VK_SUCCESS) break;
            else if(status == VK_ERROR_OUT_OF_DATE_KHR){
                debug("out of date swapchain, retrying");
                continue;
            }else fatal("failed to acquire next image");
        }
        debug("swapchain idx: %d", swapchain_idx);
        VKCHECK(vkResetCommandBuffer(vk::cmd, 0));
        VkCommandBufferBeginInfo const cmd_begin_info{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, };
        VKCHECK(vkBeginCommandBuffer(vk::cmd, &cmd_begin_info));

        record_frame(vk::cmd, swapchain_idx);

        VKCHECK(vkEndCommandBuffer(vk::cmd));

        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo const submit_info {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &vk::ready_to_submit,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &vk::cmd,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores  = &vk::ready_to_present,
        };
        VKCHECK(vkQueueSubmit(vk::graphics_queue, 1, &submit_info, vk::ready_to_record));

        VkPresentInfoKHR const present_info {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &vk::ready_to_present,
            .swapchainCount = 1,
            .pSwapchains   = &vk::swapchain,
            .pImageIndices = &swapchain_idx,
        };

        VKCHECK(vkQueuePresentKHR(vk::graphics_queue, &present_info));
        debug("submitted!");
    }

    fatal("done");

}

VkSurfaceFormatKHR choose_surface_format(){
    uint32_t format_count = 0;
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk::gpu, vk::surface, &format_count, nullptr));
    if(format_count==0) fatal("no surface formats");
    if(format_count>64) fatal("too many formats");
    VkSurfaceFormatKHR formats[64];
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk::gpu, vk::surface, &format_count, formats));
    int max_score = 0;
    int max_score_index = -1;
    for(int i=0; i<(int)format_count; i++){
        int score = 64-i;
        if(formats[i].colorSpace==VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT) score += 64;
        if(formats[i].format==VK_FORMAT_A2B10G10R10_UNORM_PACK32 || formats[i].format==VK_FORMAT_A2R10G10B10_UNORM_PACK32) score += 2*64;
        if(formats[i].format==VK_FORMAT_R16G16B16A16_SFLOAT) score += 1*64;
        if(score>max_score){
            max_score = score;
            max_score_index = i;
        }
        debug("fmt %2d: %3u %-24s %s", i, score, str_vkformat(formats[i].format), str_vkcolorspace(formats[i].colorSpace));
    }
    VkSurfaceFormatKHR ret = formats[max_score_index];
    debug("chosen format: %2d: %s %s (score: %d)\n", max_score_index, str_vkformat(ret.format), str_vkcolorspace(ret.colorSpace), max_score);
    return ret;
}

VkRenderPass create_render_pass(){
    VkAttachmentDescription const color_attachment{
        .format  = vk::surface_format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference const color_attachment_ref{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription const subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
    };
    VkRenderPassCreateInfo const render_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VkRenderPass render_pass;
    VKCHECK(vkCreateRenderPass(vk::device, &render_pass_info, nullptr, &render_pass));
    return render_pass;
}

VkPipeline create_pipeline(VkShaderModule vs, VkShaderModule fs, VkRenderPass render_pass){
    VkPipelineShaderStageCreateInfo const stages[] = {{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vs,
            .pName = "main",
        }, {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fs, 
            .pName  = "main",
        }
    };

    VkPipelineLayoutCreateInfo const layout_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    VkPipelineLayout layout;
    VKCHECK(vkCreatePipelineLayout(vk::device, &layout_info, nullptr, &layout));

    VkPipelineVertexInputStateCreateInfo const vertex_input_info{ .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VkPipelineInputAssemblyStateCreateInfo const input_assembly_info{
        .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineRasterizationStateCreateInfo const rasterization_info{
        .sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };
    VkPipelineDynamicStateCreateInfo const dynamic_info{
        .sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = LEN(dynamic_states),
        .pDynamicStates = dynamic_states
    };
    VkPipelineMultisampleStateCreateInfo const multisampling_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };
    VkPipelineColorBlendAttachmentState const blend_attachment{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                        | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT
                        | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo const blend_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment,
    };

    VkPipelineViewportStateCreateInfo const viewport_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkGraphicsPipelineCreateInfo const pipeline_info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = LEN(stages),
        .pStages = stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisampling_info,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &blend_info,
        .pDynamicState = &dynamic_info,
        .layout = layout,
        .renderPass = render_pass,
        .subpass = 0,
    };

    VkPipeline graphics_pipeline;
    VKCHECK(vkCreateGraphicsPipelines(vk::device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline));
    return graphics_pipeline;
}

void init_window_callback(ANativeActivity* activity, ANativeWindow* window){
    debug("initializing surface");
    {
        auto const info = VkAndroidSurfaceCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .window = window,
        };
        VKCHECK(vkCreateAndroidSurfaceKHR(vk::instance, &info, nullptr, &vk::surface));
    }

    vk::surface_format = choose_surface_format();
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk::gpu, vk::surface, &capabilities);
    debug("current extent: %dx%d", capabilities.currentExtent.width, capabilities.currentExtent.height);
    if(capabilities.currentExtent.width == UINT32_MAX) fatal("could not determine window size");
    vk::swapchain_extent = capabilities.currentExtent;

    debug("initializing swapchain");
    VkSwapchainCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk::surface,
        .minImageCount = 3,
        .imageFormat = vk::surface_format.format,
        .imageColorSpace = vk::surface_format.colorSpace,
        .imageExtent = vk::swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, // same queue for rendering as for presenting
        .preTransform = capabilities.currentTransform,
        // On android, we can't sepecify VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR here (we have to set that in our AndroidManifest?)
        .compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR, 
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
    };
    VKCHECK(vkCreateSwapchainKHR(vk::device, &info, nullptr, &vk::swapchain));

    vkGetSwapchainImagesKHR(vk::device, vk::swapchain, &vk::swapchain_image_count, nullptr);
    debug("swapchain image count: %d", vk::swapchain_image_count);
    if(vk::swapchain_image_count>LEN(vk::swapchain_images)) fatal("unexpected number of swapchain images, expected %d, got %d", info.minImageCount, vk::swapchain_image_count);
    vkGetSwapchainImagesKHR(vk::device, vk::swapchain, &vk::swapchain_image_count, vk::swapchain_images);
    
    vk::render_pass = create_render_pass();
    VkImageViewCreateInfo view_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = vk::surface_format.format,
        .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},
    };
    VkFramebufferCreateInfo framebuffer_info{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = vk::render_pass,
        .attachmentCount = 1,
        .width  = vk::swapchain_extent.width,
        .height = vk::swapchain_extent.height,
        .layers = 1,
    };
    VkFenceCreateInfo const fence_info = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags=VK_FENCE_CREATE_SIGNALED_BIT };
    VKCHECK(vkCreateFence(    vk::device, &fence_info,     nullptr, &vk::ready_to_record))
    VkSemaphoreCreateInfo const semaphore_info = { .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VKCHECK(vkCreateSemaphore(vk::device, &semaphore_info, nullptr, &vk::ready_to_submit))
    VKCHECK(vkCreateSemaphore(vk::device, &semaphore_info, nullptr, &vk::ready_to_present))

    vk_name((uint64_t)vk::ready_to_record,  VK_OBJECT_TYPE_FENCE,     "ready-to-record");
    vk_name((uint64_t)vk::ready_to_present, VK_OBJECT_TYPE_SEMAPHORE, "ready-to-present");
    vk_name((uint64_t)vk::ready_to_submit,  VK_OBJECT_TYPE_SEMAPHORE, "ready-to-submit");

    for(int i=0; i<(int)vk::swapchain_image_count; i++){
        view_info.image = vk::swapchain_images[i];
        VKCHECK(vkCreateImageView(vk::device, &view_info,      nullptr, &vk::swapchain_views[i]))
        VkImageView attachments[] = { vk::swapchain_views[i] };
        framebuffer_info.pAttachments = attachments;
        VKCHECK(vkCreateFramebuffer(vk::device, &framebuffer_info, nullptr, &vk::swapchain_framebuffers[i]))

        char scratch[32];
        snprintf(scratch, sizeof(scratch), "swap-image#%d", i);
        vk_name((uint64_t)vk::swapchain_images[i], VK_OBJECT_TYPE_IMAGE, scratch);

        snprintf(scratch, sizeof(scratch), "swap-view#%d", i);
        vk_name((uint64_t)vk::swapchain_views[i], VK_OBJECT_TYPE_IMAGE_VIEW, scratch);

        snprintf(scratch, sizeof(scratch), "swap-fb#%d", i);
        vk_name((uint64_t)vk::swapchain_framebuffers[i], VK_OBJECT_TYPE_FRAMEBUFFER, scratch);
    }

    auto const vs = create_shader_module(spv::triangle_vs, sizeof(spv::triangle_vs));
    auto const fs = create_shader_module(spv::white_fs,    sizeof(spv::white_fs));
    vk::graphics_pipeline = create_pipeline(vs, fs, vk::render_pass);
    debug("graphics pipeline successfully created");

    VkCommandPoolCreateInfo const cmd_pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vk::queue_family_index,
    };
    VKCHECK(vkCreateCommandPool(vk::device, &cmd_pool_info, nullptr, &vk::cmd_pool));

    VkCommandBufferAllocateInfo const alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk::cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VKCHECK(vkAllocateCommandBuffers(vk::device, &alloc_info, &vk::cmd));

    if(Android::queue && Android::has_focus)
        finalize_init(activity);
}

void init_input_callback(ANativeActivity* activity, AInputQueue* queue){
    debug("");
    Android::queue = queue;
    if(vk::surface && Android::has_focus)
        finalize_init(activity);
}

extern "C" JNIEXPORT
void ANativeActivity_onCreate(ANativeActivity* activity, 
                              void*  /*savedState*/, 
                              size_t /*savedStateSize*/){
    debug("============================================================");
    vulkan_early_init();

    // initialization: we have to wait for the window and input queue to be ready
    activity->callbacks->onNativeWindowCreated = init_window_callback;
    activity->callbacks->onInputQueueCreated   = init_input_callback;

    activity->callbacks->onStart   = [](ANativeActivity*){
        debug("start");
    };
    activity->callbacks->onResume  = [](ANativeActivity* activity){
        debug("resume");
    };
    activity->callbacks->onWindowFocusChanged = [](ANativeActivity* activity, int has_focus){
        Android::has_focus = has_focus;
        if(Android::has_focus && vk::surface && Android::queue)
            finalize_init(activity);
    };
    activity->callbacks->onPause   = [](ANativeActivity*){
        debug("pause"); 
    };
    activity->callbacks->onStop    = [](ANativeActivity*){
        debug("stop");
    };
    activity->callbacks->onDestroy = [](ANativeActivity*){
        debug("destroy");
    };

    /*
    activity->callbacks->onSaveInstanceState = on_save_instance_state;
    activity->callbacks->onNativeWindowRedrawNeeded = on_native_window_redraw_needed;
    activity->callbacks->onNativeWindowDestroyed = on_native_window_destroyed;
    activity->callbacks->onContentRectChanged = on_content_rect_changed;
    activity->callbacks->onConfigurationChanged = on_configuration_changed;
    activity->callbacks->onLowMemory = on_low_memory;
    */
    debug("onCreate done");
}

