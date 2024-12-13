#pragma once
#include "include.h"

mat4 win_transform(){
    switch(vk::win.transform){
        case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:
            return mat4(
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
        case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
            return mat4(
                0, -1, 0, 0,
                1,  0, 0, 0,
                0,  0, 1, 0,
                0,  0, 0, 1);
        case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
            return mat4(
               -1,  0, 0, 0,
                0, -1, 0, 0,
                0,  0, 1, 0,
                0,  0, 0, 1);
        case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
            return mat4(
                0,  1, 0, 0,
               -1,  0, 0, 0,
                0,  0, 1, 0,
                0,  0, 0, 1);
        default:
            fatal("unsupported orientation");
            return {};
    }
}

void record_frame(VkCommandBuffer cmd, uint32_t swapchain_idx){
    VkClearValue const clear_value[] = {
        {   .color={ .float32={0.3f,0.2f,0.4f,1.f} } },
        {   .depthStencil={ .depth=0.f, .stencil=0 } }
    };
    VkRenderPassBeginInfo const render_pass_begin_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = vk::win.render_pass,
        .framebuffer = vk::win.swapchain_framebuffers[swapchain_idx],
        .renderArea  = { .offset = {0,0}, .extent = vk::win.swapchain_extent, },
        .clearValueCount = LEN(clear_value),
        .pClearValues  = clear_value,
    };
    vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk::win.graphics_pipeline);

    VkViewport const viewport{
        .x=0.f,
        .y=0.f,
        .width=(float)vk::win.swapchain_extent.width,
        .height=(float)vk::win.swapchain_extent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D const scissor {
        .offset = {0,0},
        .extent = vk::win.swapchain_extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk::graphics_pipeline_layout, 0, 1, &vk::descriptor_set, 0, nullptr);

    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t const t3 = int64_t(ts.tv_sec%10)*int64_t(1'000'000'000) + int64_t(ts.tv_nsec);
    float   const t =  M_PI*2.f*float(t3)/10e9f;
    float   const st = sinf(t);
    float   const ct = cosf(t);
    mat4 const rotz = mat4(
        ct, -st,  0, 0,
        st,  ct,  0, 0,
        0,   0,   1, 0,
        0,   0,   0, 1);
    mat4 const roty = mat4(
        ct,  0,   st,  0, 
        0,   1,   0,   0,
       -st,  0,   ct,  0,
        0,   0,   0,   1);
    mat4 const rotx = mat4(
        1,   0,  0,  0, 
        0,  ct,-st,  0,
        0,  st, ct,  0,
        0,   0,  0,  1);

    // reverse-z puts us in LH coordinate, flip y to get back to RH coorditates
    mat4 const flip_y = mat4(
        1, 0, 0, 0,
        0,-1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);

    float const aspect = float(vk::win.swapchain_extent.width)/float(vk::win.swapchain_extent.height);
    //float const r = 1.5;
    PushConstants const push {
        .view = win_transform()
              * flip_y 
              * perspective(1.f/aspect, 1.f, 0.1)
              * translate({0,0,-3})
              * roty
    };
    vkCmdPushConstants(cmd, vk::graphics_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

#if 1
    Mesh& mesh = damaged_helmet;
    VkBuffer     buffers[2] = {vk::vertex_buffer, vk::vertex_buffer};
    VkDeviceSize offset [2] = {(VkDeviceSize)mesh.pos_offset(), (VkDeviceSize)mesh.uv_offset()};
    vkCmdBindVertexBuffers(vk::cmd, 0, LEN(offset), buffers, offset);
    vkCmdBindIndexBuffer(vk::cmd, vk::vertex_buffer, mesh.index_offset(), VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cmd, mesh.index_count, 1, 0, 0, 0);
#else
    vkCmdDraw(cmd, 3*4, 1, 0, 0);
#endif

    vkCmdEndRenderPass(cmd);
}

void loop(bool has_focus){
    (void)has_focus;
    AInputEvent* event;
    while(AInputQueue_getEvent(Android::queue, &event)>=0){
        debug("input: %d\n", AInputEvent_getType(event));
        AInputQueue_finishEvent(Android::queue, event, 1);
    }

    uint64_t const timeout = 1'000'000'000;

    VKCHECK(vkWaitForFences(vk::device, 1, &vk::win.ready_to_record, true, timeout));
    VKCHECK(vkResetFences(vk::device, 1, &vk::win.ready_to_record));

    uint32_t swapchain_idx = 0;
    while(true){
        VkResult status = vkAcquireNextImageKHR(vk::device, vk::win.swapchain, timeout, vk::win.ready_to_submit, nullptr, &swapchain_idx);
        if(status == VK_SUCCESS) break;
        else if(status == VK_ERROR_OUT_OF_DATE_KHR){
            debug("out of date swapchain, retrying");
            continue;
        }else if(status == VK_SUBOPTIMAL_KHR){
            debug("acquire next image: suboptimal");
            continue;
        }else fatal("failed to acquire next image");
    }
    //debug("acquire: %d", swapchain_idx);

    VKCHECK(vkResetCommandBuffer(vk::cmd, 0));
    VkCommandBufferBeginInfo const cmd_begin_info{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, };
    VKCHECK(vkBeginCommandBuffer(vk::cmd, &cmd_begin_info));
    record_frame(vk::cmd, swapchain_idx);
    VKCHECK(vkEndCommandBuffer(vk::cmd));

    VkPipelineStageFlags const wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo const submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &vk::win.ready_to_submit,
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &vk::cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &vk::win.ready_to_present,
    };
    VKCHECK(vkQueueSubmit(vk::graphics_queue, 1, &submit_info, vk::win.ready_to_record));

    VkPresentInfoKHR const present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &vk::win.ready_to_present,
        .swapchainCount     = 1,
        .pSwapchains        = &vk::win.swapchain,
        .pImageIndices      = &swapchain_idx,
    };
    VkResult status = vkQueuePresentKHR(vk::graphics_queue, &present_info);
    if(status==VK_SUCCESS){
        return;
    }else if(status==VK_ERROR_OUT_OF_DATE_KHR){
        debug("queue present: out of date");
    }else if(status==VK_SUBOPTIMAL_KHR){
        debug("queue present: suboptimal");
    }else{
        fatal("status: %d (0x%x)", status, status);
    }
}

void* application_thread(void*){
    debug("thread: go time!");
    // wait for initialization to be done
    vkQueueWaitIdle(vk::graphics_queue);
    while(!atomic_load(&Android::request_quit)){
        loop(atomic_load(&Android::has_focus));
    }
    debug("bye bye!");
    return NULL;
}

