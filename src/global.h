#pragma once
#include "include.h"

struct Mesh{
    char const* filename;
    int32_t vertex_count;
    int32_t index_count;
    char const* albedo;
    int32_t albedo_width;
    int32_t albedo_height;
    AAsset* geometry_asset;
    AAsset* albedo_asset;

    static constexpr VkFormat pos_format   = VK_FORMAT_R32G32B32_SFLOAT;
    static constexpr int32_t  pos_stride   = 3*sizeof(float);
    static constexpr VkFormat uv_format    = VK_FORMAT_R32G32_SFLOAT;
    static constexpr int32_t  uv_stride    = 2*sizeof(float);
    static constexpr int32_t  index_stride = sizeof(uint16_t);

    int32_t pos_offset()   const { return 0; }
    int32_t pos_size()     const { return vertex_count*pos_stride; }
    int32_t uv_offset()    const { return pos_offset()+pos_size(); }
    int32_t uv_size()      const { return vertex_count*uv_stride; }
    int32_t index_offset() const { return uv_offset()+uv_size(); }
    int32_t index_size()   const { return index_count*index_stride; }

    int32_t total_size()   const {return index_offset()+index_size(); }
};

struct PushConstants{
    mat4 view;
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
    VkBuffer  vertex_buffer;
    // descriptor set
    VkPipelineLayout      graphics_pipeline_layout;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool      descriptor_pool;
    VkDescriptorSet       descriptor_set;
    // images
    VkSampler   default_sampler;
    VkImage     albedo_image;
    VkImageView albedo_view;
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
