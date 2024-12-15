#pragma once
#include "include.h"

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

int64_t align_up(int64_t addr, int64_t alignment){
    return (addr+(alignment-1)) & -alignment;
}

void init_vulkan_device(){
    auto const vulkan_lib = dlopen("libvulkan.so", RTLD_LAZY);
    if( vulkan_lib == nullptr ) fatal("could not load libvulkan.so");

    // load global functions
    auto const vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vulkan_lib, "vkGetInstanceProcAddr");
    if( vkGetInstanceProcAddr == nullptr ) fatal("could not load vkGetInstanceProcAddr");
    auto const vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(nullptr, "vkCreateInstance");
    auto const vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties");

    if(true){
        uint32_t properties_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        VkExtensionProperties properties[64];
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties);
        for(int i=0; i<(int)properties_count; i++){
            debug("instance extension: %s", properties[i].extensionName);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // create instance
    {
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
            .apiVersion         = VK_API_VERSION_1_0,
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
    }

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

    ////////////////////////////////////////////////////////////////////////////////
    // gpu
    uint32_t gpu_count = 1;
    VKCHECK(vkEnumeratePhysicalDevices(vk::instance, &gpu_count, &vk::gpu))

    vk::device_local_memidx = UINT32_MAX;
    vk::lazy_memidx         = UINT32_MAX;
    vk::uncached_memidx     = UINT32_MAX;
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(vk::gpu, &mem_properties);
    for(int i=0; i<(int)mem_properties.memoryTypeCount; i++){
        auto const type = mem_properties.memoryTypes[i];
        if(vk::device_local_memidx==UINT32_MAX && type.propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            vk::device_local_memidx = i;
        if(vk::lazy_memidx==UINT32_MAX && type.propertyFlags&VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
            vk::lazy_memidx = i;
        if(vk::uncached_memidx==UINT32_MAX && type.propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && ((type.propertyFlags&VK_MEMORY_PROPERTY_HOST_CACHED_BIT)==0))
            vk::uncached_memidx = i;
        debug("%2d %c%c%c%c%c%c %c%c%c %d %c%c %5luM\n", i, 
            type.propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT?        'D':'-',
            type.propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT?        'V':'-',
            type.propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT?       'C':'-',
            type.propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT?       '$':'-',
            type.propertyFlags&VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT?    'L':'-',
            type.propertyFlags&VK_MEMORY_PROPERTY_PROTECTED_BIT?           'P':'-',
            type.propertyFlags&VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD? 'C':'-',
            type.propertyFlags&VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD? 'U':'-',
            type.propertyFlags&VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV?     'R':'-',
            type.heapIndex,
            mem_properties.memoryHeaps[type.heapIndex].flags&VK_MEMORY_HEAP_DEVICE_LOCAL_BIT?   'D':'-',
            mem_properties.memoryHeaps[type.heapIndex].flags&VK_MEMORY_HEAP_MULTI_INSTANCE_BIT? 'M':'-',
            mem_properties.memoryHeaps[type.heapIndex].size/(1024*1024));
    }
    debug("device local: %d, lazy: %d, uncached: %d", vk::device_local_memidx, vk::lazy_memidx, vk::uncached_memidx);

    if(false){
        uint32_t prop_count = 0;
        VKCHECK(vkEnumerateDeviceExtensionProperties(vk::gpu, nullptr, &prop_count, nullptr));
        debug("%d device extensions", prop_count);
        VkExtensionProperties properties[256];
        VKCHECK(vkEnumerateDeviceExtensionProperties(vk::gpu, nullptr, &prop_count, properties));
        for(int i=0; i<(int)prop_count; i++){
            debug("device ext %3d: %s", i, properties[i].extensionName);
        }
    }

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

    ////////////////////////////////////////////////////////////////////////////////
    // device
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

    #define LOCAL_FN(name) auto const name = (PFN_##name)vkGetDeviceProcAddr(vk::device, #name);
    LOCAL_FN(vkGetDeviceQueue)
    #undef LOCAL_FN
    vkGetDeviceQueue(vk::device, vk::queue_family_index, 0, &vk::graphics_queue);
    
    ////////////////////////////////////////////////////////////////////////////////
    // shaders
    vk::vs       = create_shader_module(spv::default_vs, sizeof(spv::default_vs));
    vk::fs       = create_shader_module(spv::default_fs, sizeof(spv::default_fs));
    vk::gizmo_vs = create_shader_module(spv::gizmo_vs,   sizeof(spv::gizmo_vs));

    Mesh const& mesh = damaged_helmet;

    ////////////////////////////////////////////////////////////////////////////////
    // staging buffer
    {
        debug("allocating staging buffer");
        VkBufferCreateInfo const buffer_info{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = (VkDeviceSize)mesh.total_size(),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VKCHECK(vkCreateBuffer(vk::device, &buffer_info, nullptr, &vk::staging_buffer));
        vk_name((uint64_t)vk::staging_buffer, VK_OBJECT_TYPE_BUFFER, "staging-buffer");

        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(vk::device, vk::staging_buffer, &req);
        debug("staging type bits: %x", req.memoryTypeBits);

        VkMemoryAllocateInfo const staging_alloc_info {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = req.size,
            .memoryTypeIndex = vk::uncached_memidx,
        };
        VkDeviceMemory staging_memory;
        VKCHECK(vkAllocateMemory(vk::device, &staging_alloc_info, nullptr, &staging_memory));
        vk_name((uint64_t)vk::staging_buffer, VK_OBJECT_TYPE_BUFFER, "staging-memory");
        VKCHECK(vkBindBufferMemory(vk::device, vk::staging_buffer, staging_memory, 0));
        VKCHECK(vkMapMemory(vk::device, staging_memory, 0, req.size, 0, &vk::staging_buffer_ptr));

        // upload mesh
        AAsset_read(mesh.asset, vk::staging_buffer_ptr, mesh.total_size());
        VkMappedMemoryRange const flush_range{
            .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .memory = staging_memory,
            .offset = 0,
            .size   = req.size,
        };
        VKCHECK(vkFlushMappedMemoryRanges(vk::device, 1, &flush_range));
    }

    ////////////////////////////////////////////////////////////////////////////////
    // textures
    struct Config{
        char const* debug_name;
        VkExtent2D  extent;
        VkFormat    format;
    };
    Arr configs = {
        Config{ "albedo", mesh.albedo_extent, VK_FORMAT_R8G8B8A8_SRGB  },
        Config{ "normal", mesh.normal_extent, VK_FORMAT_R8G8B8A8_UNORM }
    };
    constexpr int const img_count = LEN(configs);
    {
        // create images
        for(int i=0; i<img_count; i++){
            VkImageCreateInfo image_info{
                .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType     = VK_IMAGE_TYPE_2D,
                .format        = configs[i].format,
                .extent        = {configs[i].extent.width, configs[i].extent.height, 1},
                .mipLevels     = 1,
                .arrayLayers   = 1,
                .samples       = VK_SAMPLE_COUNT_1_BIT,
                .tiling        = VK_IMAGE_TILING_OPTIMAL,
                .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                               | VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            VKCHECK(vkCreateImage(vk::device, &image_info, nullptr, &vk::images[i]));
            vk_name((uint64_t)vk::images[i],  VK_OBJECT_TYPE_IMAGE, configs[i].debug_name);
        }

        // alloc space for all images
        int64_t total_size = 0;
        int64_t img_offsets[img_count] = {};
        for(int i=0; i<img_count; i++){
            VkMemoryRequirements memreq = {};
            vkGetImageMemoryRequirements(vk::device, vk::images[i], &memreq);
            total_size = align_up(total_size, memreq.alignment);
            img_offsets[i] = total_size;
            total_size += memreq.size;
        }
        VkMemoryAllocateInfo albedo_alloc_info {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = (VkDeviceSize)total_size,
            .memoryTypeIndex = vk::device_local_memidx,
        };
        VKCHECK(vkAllocateMemory(vk::device, &albedo_alloc_info, nullptr, &vk::texture_memory));
        vk_name((uint64_t)vk::texture_memory,  VK_OBJECT_TYPE_DEVICE_MEMORY, "texture-mem");

        // bind memory and create views
        for(int i=0; i<img_count; i++){
            vkBindImageMemory(vk::device, vk::images[i], vk::texture_memory, img_offsets[i]);
            VkImageViewCreateInfo const albedo_view_info{
                .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image    = vk::images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format   = configs[i].format,
                .subresourceRange = {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1
                }
            };
            VKCHECK(vkCreateImageView(vk::device, &albedo_view_info, nullptr, &vk::views[i]));
        }
        debug("done creating image resources");
    }

    VkSamplerCreateInfo const sampler_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.f,
        .anisotropyEnable = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.f,
        .maxLod = 0.f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    VKCHECK(vkCreateSampler(vk::device, &sampler_info, nullptr, &vk::default_sampler));

    ////////////////////////////////////////////////////////////////////////////////
    // vertex+index buffer
    {
        VkBufferCreateInfo const buffer_info{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = (uint64_t)mesh.geometry_size(),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VKCHECK(vkCreateBuffer(vk::device, &buffer_info, nullptr, &vk::geometry_buffer));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(vk::device, vk::geometry_buffer, &mem_requirements);

        VkMemoryAllocateInfo const memalloc_info{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_requirements.size,
            .memoryTypeIndex = vk::device_local_memidx,
        };
        VKCHECK(vkAllocateMemory(vk::device, &memalloc_info, nullptr, &vk::geometry_memory));
        VKCHECK(vkBindBufferMemory(vk::device, vk::geometry_buffer, vk::geometry_memory, mesh.pos_offset()));
    }

    ////////////////////////////////////////////////////////////////////////////////
    // descriptor sets
    {
        auto const bindings = mapi<2>([](uint32_t i){
            return VkDescriptorSetLayoutBinding{
                .binding = i,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            };
        });
        VkDescriptorSetLayoutCreateInfo const layout_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = bindings.len(),
            .pBindings = bindings.begin(),
        };
        VKCHECK(vkCreateDescriptorSetLayout(vk::device, &layout_info, nullptr, &vk::descriptor_set_layout));

        VkDescriptorPoolSize const pool_sizes[] {{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = (uint32_t)img_count,
        }};
        VkDescriptorPoolCreateInfo const pool_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = LEN(pool_sizes),
            .pPoolSizes = pool_sizes,
        };
        VKCHECK(vkCreateDescriptorPool(vk::device, &pool_info, nullptr, &vk::descriptor_pool));

        VkDescriptorSetAllocateInfo const descriptor_set_alloc_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = vk::descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &vk::descriptor_set_layout,
        };
        VKCHECK(vkAllocateDescriptorSets(vk::device, &descriptor_set_alloc_info, &vk::descriptor_set));

        auto const img_descr_info = mapi<2>([](uint32_t i){
            return VkDescriptorImageInfo{
                .sampler   = vk::default_sampler,
                .imageView = vk::views[i],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
        });
        VkWriteDescriptorSet const write_descriptors[]{{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vk::descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = img_descr_info.len(),
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = img_descr_info.begin()
        }};
        vkUpdateDescriptorSets(vk::device, LEN(write_descriptors), write_descriptors, 0, nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // pipeline layout
    {
        VkPushConstantRange const push_constants{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size   = sizeof(PushConstants),
        };
        VkPipelineLayoutCreateInfo const layout_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &vk::descriptor_set_layout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &push_constants,
        };
        VKCHECK(vkCreatePipelineLayout(vk::device, &layout_info, nullptr, &vk::graphics_pipeline_layout));
    }

    ////////////////////////////////////////////////////////////////////////////////
    // command buffers
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


    ////////////////////////////////////////////////////////////////////////////////
    // upload images
    {
        VkCommandBufferBeginInfo const begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(vk::cmd, &begin_info);

        // transition layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier img_init_barriers[img_count];
        for(int i=0; i<img_count; i++){
            img_init_barriers[i] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = vk::images[i],
                .subresourceRange = {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            };
        };
        vkCmdPipelineBarrier(vk::cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            LEN(img_init_barriers), img_init_barriers);

        // copy staging buffer to image
        VkDeviceSize const buffer_offsets[img_count] = {
            mesh.albedo_offset(),
            mesh.normal_offset(),
        };
        for(int i=0; i<img_count; i++){
            VkBufferImageCopy region{
                .bufferOffset      = buffer_offsets[i],
                .bufferRowLength   = 0, // 0 means tightly packed
                .bufferImageHeight = 0, // 0 means tightly packed
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .imageOffset = {0,0,0},
                .imageExtent = { configs[i].extent.width, configs[i].extent.height, 1 }
            };
            vkCmdCopyBufferToImage(vk::cmd, vk::staging_buffer, vk::images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }

        // copy geometry
        VkBufferCopy const geometry_region{
            .srcOffset = 0,
            .dstOffset = 0,
            .size      = mesh.geometry_size(),
        };
        vkCmdCopyBuffer(vk::cmd, vk::staging_buffer, vk::geometry_buffer, 1, &geometry_region);

        // transition layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier img_copy_barriers[img_count];
        for(int i=0; i<img_count; i++){
            img_copy_barriers[i] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = vk::images[i],
                .subresourceRange = {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            };
        }
        vkCmdPipelineBarrier(vk::cmd, 
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            LEN(img_copy_barriers), img_copy_barriers);

        VKCHECK(vkEndCommandBuffer(vk::cmd));
        VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &vk::cmd,
        };
        VKCHECK(vkQueueSubmit(vk::graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    }
}
