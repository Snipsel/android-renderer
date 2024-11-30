#pragma once
#include "include.h"

namespace vk{
    constexpr int      max_swapchain_images = 8;
    // early init
    VkInstance         instance;
    VkPhysicalDevice   gpu;
    VkDevice           device;
    uint32_t           queue_family_index;
    VkQueue            graphics_queue;
    VkDebugUtilsMessengerEXT debug_messenger;
    // surface and swapchain
    VkSurfaceKHR       surface;
    VkSurfaceFormatKHR surface_format;
    VkSwapchainKHR     swapchain;
    uint32_t           swapchain_image_count;
    VkExtent2D         swapchain_extent;
    VkImage            swapchain_images[max_swapchain_images];
    VkImageView        swapchain_views[max_swapchain_images];
    VkFramebuffer      swapchain_framebuffers[max_swapchain_images];
    // sync
    VkFence            ready_to_record;
    VkSemaphore        ready_to_submit;
    VkSemaphore        ready_to_present;
    // pipeline
    VkRenderPass       render_pass;
    VkPipeline         graphics_pipeline;
    VkCommandPool      cmd_pool;
    VkCommandBuffer    cmd;
}

namespace Android{
    bool has_focus;
    AInputQueue* queue;
}

namespace spv{
uint32_t const triangle_vs[] =
#include "../build/spv/triangle.vs.h"
;
uint32_t const white_fs[] =
#include "../build/spv/white.fs.h"
;
}

#define DECL(name) PFN_##name name;

// instance functions //////////////////////////////////////////////////////////
DECL(vkCreateAndroidSurfaceKHR)
DECL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
DECL(vkGetPhysicalDeviceSurfaceFormatsKHR)
void load_instance_funcs(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr){
#define FN(name) name = (PFN_##name)vkGetInstanceProcAddr(vk::instance, #name);
    FN(vkCreateAndroidSurfaceKHR)
    FN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    FN(vkGetPhysicalDeviceSurfaceFormatsKHR)
#undef FN
}

// device functions ////////////////////////////////////////////////////////////
DECL(vkCreateSwapchainKHR)
DECL(vkGetSwapchainImagesKHR)
DECL(vkCreateImageView)
DECL(vkSetDebugUtilsObjectNameEXT)
DECL(vkCreateSemaphore)
DECL(vkCreateFence)
DECL(vkWaitForFences)
DECL(vkResetFences)
DECL(vkCreateShaderModule)
DECL(vkCreatePipelineLayout)
DECL(vkCreateGraphicsPipelines)
DECL(vkCreateRenderPass)
DECL(vkCreateFramebuffer)
DECL(vkCreateCommandPool)
DECL(vkAllocateCommandBuffers)
DECL(vkBeginCommandBuffer)
DECL(vkEndCommandBuffer)
DECL(vkResetCommandBuffer)
DECL(vkCmdBeginRenderPass)
DECL(vkCmdEndRenderPass)
DECL(vkCmdBindPipeline)
DECL(vkCmdSetViewport)
DECL(vkCmdSetScissor)
DECL(vkCmdDraw);
DECL(vkAcquireNextImageKHR);
DECL(vkQueueSubmit);
DECL(vkQueuePresentKHR);
void load_device_funcs(PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr){
#define FN(name) name = (PFN_##name)vkGetDeviceProcAddr(vk::device, #name);
    FN(vkCreateSwapchainKHR)
    FN(vkGetSwapchainImagesKHR)
    FN(vkCreateImageView)
    FN(vkSetDebugUtilsObjectNameEXT)
    FN(vkCreateSemaphore)
    FN(vkCreateFence)
    FN(vkWaitForFences)
    FN(vkResetFences)
    FN(vkCreateShaderModule)
    FN(vkCreatePipelineLayout)
    FN(vkCreateGraphicsPipelines)
    FN(vkCreateRenderPass)
    FN(vkCreateFramebuffer)
    FN(vkCreateCommandPool)
    FN(vkAllocateCommandBuffers)
    FN(vkBeginCommandBuffer)
    FN(vkEndCommandBuffer)
    FN(vkResetCommandBuffer)
    FN(vkCmdBeginRenderPass)
    FN(vkCmdEndRenderPass)
    FN(vkCmdBindPipeline)
    FN(vkCmdSetViewport)
    FN(vkCmdSetScissor)
    FN(vkCmdDraw);
    FN(vkAcquireNextImageKHR);
    FN(vkQueueSubmit);
    FN(vkQueuePresentKHR);
#undef FN
}

#undef DECL
