#pragma once
#include "include.h"

struct Mesh{
    char const* filename;
    int32_t vertex_count;
    int32_t index_count;
    VkExtent2D albedo_extent;
    VkExtent2D normal_extent;
    AAsset* asset;

    struct Vertex{
        vec2 uv;
        vec3 normal;
        vec3 tangent;
        vec3 bitangent;
    };

    static constexpr VkFormat pos_format    = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr int32_t  pos_stride    = sizeof(vec3);
    static constexpr int32_t  index_stride  = sizeof(uint16_t);

    // vertex buffer
    static constexpr VkVertexInputBindingDescription const binding_descr[] =
    { { .binding=0, .stride= pos_stride,     .inputRate= VK_VERTEX_INPUT_RATE_VERTEX
    },{ .binding=1, .stride= sizeof(Vertex), .inputRate= VK_VERTEX_INPUT_RATE_VERTEX
    } };
    static constexpr VkVertexInputAttributeDescription const attr_descr[] =
    { { .location=0, .binding=0, .format=pos_format,                 .offset= 0,
    },{ .location=1, .binding=1, .format=VK_FORMAT_R32G32_SFLOAT,    .offset= offsetof(Vertex,uv),
    },{ .location=2, .binding=1, .format=VK_FORMAT_R32G32B32_SFLOAT, .offset= offsetof(Vertex,normal),
    },{ .location=3, .binding=1, .format=VK_FORMAT_R32G32B32_SFLOAT, .offset= offsetof(Vertex,tangent),
    },{ .location=4, .binding=1, .format=VK_FORMAT_R32G32B32_SFLOAT, .offset= offsetof(Vertex,bitangent),
    } };

    // geometry
    inline VkDeviceSize pos_offset()    const { return 0; }
    inline VkDeviceSize pos_size()      const { return vertex_count*pos_stride; }
    inline VkDeviceSize vertex_offset() const { return pos_offset()+pos_size(); }
    inline VkDeviceSize vertex_size()   const { return vertex_count*sizeof(Vertex); }
    inline VkDeviceSize index_offset()  const { return vertex_offset()+vertex_size(); }
    inline VkDeviceSize index_size()    const { return index_count*index_stride; }

    // textures
    inline VkDeviceSize albedo_offset() const { return index_offset()+index_size(); }
    inline VkDeviceSize albedo_size()   const { return 4*albedo_extent.width*albedo_extent.height; }
    inline VkDeviceSize normal_offset() const { return albedo_offset()+albedo_size(); }
    inline VkDeviceSize normal_size()   const { return 4*normal_extent.width*normal_extent.height; }

    // buffer offsets
    inline VkDeviceSize geometry_size()   const { return index_offset()+index_size(); }
    inline VkDeviceSize textures_offset() const { return albedo_offset(); }
    inline VkDeviceSize textures_size()   const { return normal_offset()+normal_size() - textures_offset(); }

    inline VkDeviceSize total_size() const { return geometry_size()+textures_size(); }
};

struct PushConstants{
    mat4 mvp;
    mat4 model;
};

struct WindowState{
    static constexpr int max_swapchain_images = 8;
    VkSurfaceKHR       surface;
    VkSurfaceFormatKHR surface_format;
    VkSurfaceTransformFlagBitsKHR transform;
    VkSwapchainKHR     swapchain;
    uint32_t           swapchain_image_count;
    VkExtent2D         swapchain_extent;
    VkImage            swapchain_images[max_swapchain_images];
    VkImageView        swapchain_views[max_swapchain_images];
    VkFramebuffer      swapchain_framebuffers[max_swapchain_images];
    VkFence            ready_to_record;
    VkSemaphore        ready_to_submit;
    VkSemaphore        ready_to_present;
    // depth
    VkImage            depth_image;
    VkImageView        depth_view;
    // intermediate render target
    // VkImage            render_image;
    // VkImageView        render_view;
    // VkFramebuffer      render_framebuffer;
    // surface-dependent state
    VkRenderPass       render_pass;
    VkPipeline         graphics_pipeline;
};

namespace vk{
    // early init
    VkInstance         instance;
    VkPhysicalDevice   gpu;
    VkDevice           device;
    uint32_t           queue_family_index;
    VkQueue            graphics_queue;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkShaderModule     vs, fs, gizmo_vs;
    VkCommandPool      cmd_pool;
    VkCommandBuffer    cmd;
    // memory
    uint32_t  device_local_memidx;
    uint32_t  lazy_memidx;
    uint32_t  uncached_memidx;
    VkBuffer  staging_buffer;
    void*     staging_buffer_ptr;
    // geometry
    VkDeviceMemory geometry_memory;
    VkBuffer       geometry_buffer;
    // descriptor set
    VkPipelineLayout      graphics_pipeline_layout;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool      descriptor_pool;
    VkDescriptorSet       descriptor_set;
    // textures
    VkDeviceMemory texture_memory;
    VkSampler      default_sampler;
    VkImage        images[2];
    VkImageView    views[2];
    // window
    WindowState        win;
}

namespace Android{
    atomic_bool    request_quit;
    atomic_bool    has_focus;
    pthread_t      main_thread;
    ANativeWindow* window;
    AInputQueue*   queue;
}

namespace spv{
uint32_t const gizmo_vs[] =
#include "../build/spv/gizmo.vs.h"
;
uint32_t const default_vs[] =
#include "../build/spv/default.vs.h"
;
uint32_t const default_fs[] =
#include "../build/spv/default.fs.h"
;
}

#define DECL(name) PFN_##name name;

// instance functions //////////////////////////////////////////////////////////
DECL(vkCreateAndroidSurfaceKHR)
DECL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
DECL(vkGetPhysicalDeviceSurfaceFormatsKHR)
DECL(vkGetPhysicalDeviceMemoryProperties)
void load_instance_funcs(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr){
#define FN(name) name = (PFN_##name)vkGetInstanceProcAddr(vk::instance, #name);
    FN(vkCreateAndroidSurfaceKHR)
    FN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    FN(vkGetPhysicalDeviceSurfaceFormatsKHR)
    FN(vkGetPhysicalDeviceMemoryProperties)
#undef FN
}

// device functions ////////////////////////////////////////////////////////////
DECL(vkCreateSwapchainKHR)
DECL(vkGetSwapchainImagesKHR)
DECL(vkCreateImage)
DECL(vkCreateImageView)
DECL(vkSetDebugUtilsObjectNameEXT)
DECL(vkCreateSemaphore)
DECL(vkCreateFence)
DECL(vkWaitForFences)
DECL(vkResetFences)
DECL(vkBindImageMemory)
DECL(vkCreateShaderModule)
DECL(vkCreatePipelineLayout)
DECL(vkCreateGraphicsPipelines)
DECL(vkCreateRenderPass)
DECL(vkCreateFramebuffer)
DECL(vkCreateCommandPool)
DECL(vkCreateBuffer)
DECL(vkGetBufferMemoryRequirements)
DECL(vkGetImageMemoryRequirements)
DECL(vkAllocateCommandBuffers)
DECL(vkBeginCommandBuffer)
DECL(vkEndCommandBuffer)
DECL(vkResetCommandBuffer)
DECL(vkCreateSampler)
DECL(vkCreateDescriptorSetLayout)
DECL(vkCreateDescriptorPool)
DECL(vkAllocateDescriptorSets)
DECL(vkUpdateDescriptorSets)

DECL(vkCmdCopyBuffer)
DECL(vkCmdCopyBufferToImage)
DECL(vkCmdBeginRenderPass)
DECL(vkCmdEndRenderPass)
DECL(vkCmdBindPipeline)
DECL(vkCmdSetViewport)
DECL(vkCmdSetScissor)
DECL(vkCmdPushConstants)
DECL(vkCmdDraw)
DECL(vkCmdDrawIndexed)
DECL(vkCmdPipelineBarrier)
DECL(vkCmdBindDescriptorSets)

DECL(vkAcquireNextImageKHR)
DECL(vkQueueSubmit)
DECL(vkQueuePresentKHR)
DECL(vkQueueWaitIdle)
DECL(vkDeviceWaitIdle)
DECL(vkAllocateMemory)
DECL(vkBindBufferMemory)
DECL(vkMapMemory)
DECL(vkFlushMappedMemoryRanges)
DECL(vkCmdBindVertexBuffers)
DECL(vkCmdBindIndexBuffer)

DECL(vkDestroyCommandPool)
DECL(vkDestroyPipeline)
DECL(vkDestroyPipelineLayout)
DECL(vkDestroyRenderPass)
DECL(vkDestroySemaphore)
DECL(vkDestroyFence)
DECL(vkDestroyFramebuffer)
DECL(vkDestroyImage)
DECL(vkDestroyImageView)
DECL(vkDestroySwapchainKHR)
DECL(vkDestroySurfaceKHR)
void load_device_funcs(PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr){
#define FN(name) name = (PFN_##name)vkGetDeviceProcAddr(vk::device, #name);
    FN(vkCreateSwapchainKHR)
    FN(vkGetSwapchainImagesKHR)
    FN(vkCreateImage)
    FN(vkCreateImageView)
    FN(vkSetDebugUtilsObjectNameEXT)
    FN(vkCreateSemaphore)
    FN(vkCreateFence)
    FN(vkWaitForFences)
    FN(vkResetFences)
    FN(vkBindImageMemory)
    FN(vkCreateShaderModule)
    FN(vkCreatePipelineLayout)
    FN(vkCreateGraphicsPipelines)
    FN(vkCreateRenderPass)
    FN(vkCreateFramebuffer)
    FN(vkCreateCommandPool)
    FN(vkCreateBuffer)
    FN(vkGetBufferMemoryRequirements)
    FN(vkGetImageMemoryRequirements)
    FN(vkAllocateCommandBuffers)
    FN(vkBeginCommandBuffer)
    FN(vkEndCommandBuffer)
    FN(vkResetCommandBuffer)
    FN(vkCreateSampler)
    FN(vkCreateDescriptorSetLayout)
    FN(vkCreateDescriptorPool)
    FN(vkAllocateDescriptorSets)
    FN(vkUpdateDescriptorSets)

    FN(vkCmdCopyBuffer)
    FN(vkCmdCopyBufferToImage)
    FN(vkCmdBeginRenderPass)
    FN(vkCmdEndRenderPass)
    FN(vkCmdBindPipeline)
    FN(vkCmdSetViewport)
    FN(vkCmdSetScissor)
    FN(vkCmdPushConstants)
    FN(vkCmdDraw)
    FN(vkCmdDrawIndexed)
    FN(vkCmdCopyBufferToImage)
    FN(vkCmdPipelineBarrier)
    FN(vkCmdBindDescriptorSets)

    FN(vkAcquireNextImageKHR)
    FN(vkQueueSubmit)
    FN(vkQueuePresentKHR)
    FN(vkQueueWaitIdle)
    FN(vkDeviceWaitIdle)
    FN(vkAllocateMemory)
    FN(vkBindBufferMemory)
    FN(vkMapMemory)
    FN(vkFlushMappedMemoryRanges)
    FN(vkCmdBindVertexBuffers)
    FN(vkCmdBindIndexBuffer)

    FN(vkDestroyCommandPool)
    FN(vkDestroyPipeline)
    FN(vkDestroyPipelineLayout)
    FN(vkDestroyRenderPass)
    FN(vkDestroySemaphore)
    FN(vkDestroyFence)
    FN(vkDestroyFramebuffer)
    FN(vkDestroyImage)
    FN(vkDestroyImageView)
    FN(vkDestroySwapchainKHR)
    FN(vkDestroySurfaceKHR)
#undef FN
}

#undef DECL
