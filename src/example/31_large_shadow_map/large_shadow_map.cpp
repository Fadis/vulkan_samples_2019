/*
 * Copyright (C) 2020 Naomasa Matsubayashi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <iostream>
#include <vector>
#include <filesystem>
#include <boost/scope_exit.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include <vk_mem_alloc.h>
#pragma GCC diagnostic pop
#include <fx/gltf.h>
#include <vw/config.h>
#include <vw/list_device.h>
#include <vw/is_capable.h>
#include <vw/instance.h>
#include <vw/context.h>
#include <vw/render_pass.h>
#include <vw/shader.h>
#include <vw/pipeline.h>
#include <vw/framebuffer.h>
#include <vw/image.h>
#include <vw/buffer.h>
#include <vw/wait_for_idle.h>
#include <vw/command_buffer.h>
#include <vw/projection.h>
#include <viewer/document.h>



int main( int argc, const char *argv[] ) {
  const auto config = vw::parse_configs( argc, argv );
  auto instance = vw::create_instance(
    config,
    {},
    {}
  );
  if( config.list ) {
    vw::list_devices( *instance, config );
    return 0;
  }
  if( !vw::is_capable(
    *instance,
    config,
    {
      VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME
    }, {}
  ) ) {
    std::cout << "指定された条件に合うデバイスは無かった" << std::endl;
    return 0;
  }
  {
    auto context = vw::create_context(
      *instance,
      config,
      {
        VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME
      },
      {},
      {
        vk::DescriptorPoolSize().setType( vk::DescriptorType::eUniformBuffer ).setDescriptorCount( 400 ),
        vk::DescriptorPoolSize().setType( vk::DescriptorType::eCombinedImageSampler ).setDescriptorCount( 400 )
      },
      {
        vk::DescriptorSetLayoutBinding()
          .setDescriptorType( vk::DescriptorType::eUniformBuffer )
          .setDescriptorCount( 1 )
          .setBinding( 0 )
          .setStageFlags( vk::ShaderStageFlagBits::eFragment )
          .setPImmutableSamplers( nullptr ),
        vk::DescriptorSetLayoutBinding() // base color
          .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
          .setDescriptorCount( 1 )
          .setBinding( 1 )
          .setStageFlags( vk::ShaderStageFlagBits::eFragment )
          .setPImmutableSamplers( nullptr ),
        vk::DescriptorSetLayoutBinding() // roughness metallness
          .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
          .setDescriptorCount( 1 )
          .setBinding( 2 )
          .setStageFlags( vk::ShaderStageFlagBits::eFragment )
          .setPImmutableSamplers( nullptr ),
        vk::DescriptorSetLayoutBinding() // normal
          .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
          .setDescriptorCount( 1 )
          .setBinding( 3 )
          .setStageFlags( vk::ShaderStageFlagBits::eFragment )
          .setPImmutableSamplers( nullptr ),
        vk::DescriptorSetLayoutBinding() // occlusion
          .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
          .setDescriptorCount( 1 )
          .setBinding( 4 )
          .setStageFlags( vk::ShaderStageFlagBits::eFragment )
          .setPImmutableSamplers( nullptr ),
        vk::DescriptorSetLayoutBinding() // emissive
          .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
          .setDescriptorCount( 1 )
          .setBinding( 5 )
          .setStageFlags( vk::ShaderStageFlagBits::eFragment )
          .setPImmutableSamplers( nullptr ),
        vk::DescriptorSetLayoutBinding() // shadow
          .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
          .setDescriptorCount( 1 )
          .setBinding( 6 )
          .setStageFlags( vk::ShaderStageFlagBits::eFragment )
          .setPImmutableSamplers( nullptr ),
        vk::DescriptorSetLayoutBinding() // dynamic uniform
          .setDescriptorType( vk::DescriptorType::eUniformBuffer )
          .setDescriptorCount( 1 )
          .setBinding( 7 )
          .setStageFlags( vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment )
          .setPImmutableSamplers( nullptr ),
      }
    );
    std::vector< vw::render_pass_t > render_pass;
    render_pass.emplace_back( vw::create_render_pass( context, true, true ) );
    render_pass.emplace_back( vw::create_render_pass( context, false, false ) );
    std::vector< std::vector< vw::framebuffer_t > > framebuffers;
    framebuffers.emplace_back( vw::create_framebuffer(
      context, render_pass[ 0 ], 1024u, 1024u
    ) );
    framebuffers.emplace_back( vw::create_framebuffer(
      context, render_pass[ 1 ]
    ) );
    auto fence = vw::create_framebuffer_fences(
      context, framebuffers[ 0 ].size(), framebuffers.size()
    );
    viewer::sampler_t shadow_sampler = viewer::create_nomip_sampler( context );
    std::vector< std::vector< viewer::texture_t > > extra_textures;
    for( size_t i = 0u; i != framebuffers[ 0 ].size(); ++i ) {
      extra_textures.emplace_back(
        std::vector< viewer::texture_t >{
          viewer::create_texture(
            framebuffers[ 0 ][ i ].color_image,
            shadow_sampler
          )
        }
      );
    }
    std::vector< viewer::buffer_t > dynamic_uniform_buffer;
    for( size_t i = 0u; i != framebuffers[ 0 ].size(); ++i )
      dynamic_uniform_buffer.emplace_back(
        viewer::create_uniform_buffer( context, sizeof( viewer::dynamic_uniforms_t ) )
      );
    std::vector< viewer::buffer_t > temporary_dynamic_uniform_buffer;
    for( size_t i = 0u; i != framebuffers[ 0 ].size(); ++i )
      temporary_dynamic_uniform_buffer.emplace_back(
        viewer::create_staging_buffer( context, sizeof( viewer::dynamic_uniforms_t ) )
      );
    viewer::document_t document = viewer::load_gltf(
      context,
      render_pass,
      std::filesystem::path( config.input ),
      framebuffers[ 0 ].size(),
      config.shader,
      config.shader_mask,
      extra_textures,
      dynamic_uniform_buffer,
      float( context.width )/float( context.height )
    );
    auto center = ( document.node.min + document.node.max ) / 2.f;
    auto scale = std::abs( glm::length( document.node.max - document.node.min ) );
    uint32_t current_frame = 0u;
    auto command_buffer = vw::get_command_buffer( context, true, framebuffers[ 0 ].size() * framebuffers.size() );
    auto graphics_queue = context.device->getQueue( context.graphics_queue_index, 0 );
    auto present_queue = context.device->getQueue( context.present_queue_index, 0 );
    const std::array< vk::ClearValue, 2 > clear_values{
      vk::ClearColorValue( std::array< float, 4u >{ config.purple ? 1.0f : 0.0f, 0.0f, config.purple ? 1.0f : 0.0f, 1.0f } ),
      vk::ClearDepthStencilValue( 1.f, 0 )
    };

    std::vector< vk::Viewport > viewport;
    std::vector< vk::Rect2D > scissor;
    for( const auto &fb: framebuffers ) {
      viewport.emplace_back(
        vk::Viewport()
          .setWidth( fb[ 0 ].width )
          .setHeight( fb[ 0 ].height )
          .setMinDepth( 0.0f )
          .setMaxDepth( 1.0f )
      );
      scissor.emplace_back( vk::Rect2D( vk::Offset2D(0, 0), vk::Extent2D( fb[ 0 ].width, fb[ 0 ].height ) ) );
    }
    auto lhrh = glm::mat4(-1,0,0,0,0,-1,0,0,0,0,1,0,0,0,0,1);
    std::vector< glm::mat4 > projection{
      lhrh * glm::perspective( 0.39959648408210363f , (float(framebuffers[ 0 ][ 0 ].width)/float(framebuffers[ 0 ][ 0 ].height)), std::min(0.1f*scale,0.5f), 10.f*scale ),
      glm::perspective( 0.39959648408210363f, (float(framebuffers[ 1 ][ 0 ].width)/float(framebuffers[ 1 ][ 0 ].height)), std::min(0.1f*scale,0.5f), 1.4f*scale )
    };
    auto camera_pos = center + glm::vec3{ 0.f, 0.f, 1.0f*scale };
    auto camera_dir = center + glm::vec3{ 0.f, 0.f, 0.f };
    const auto cameras = viewer::get_cameras(
      document.node,
      document.camera
    );
    if( !cameras.empty() ) {
      //projection[ 1 ] = cameras[ 0 ].projection_matrix;
      camera_pos = cameras[ 0 ].camera_pos;
      camera_dir = cameras[ 0 ].camera_direction + camera_pos;
    }
    float camera_angle = 0;//M_PI;
    auto speed = 0.01f*scale;
    auto light_pos = glm::vec3{ 0.0f*scale, 1.2f*scale, 0.0f*scale };
    float light_energy = 5.0f;
    const auto point_lights = viewer::get_point_lights(
      document.node,
      document.point_light
    );
    if( !point_lights.empty() ) {
      light_energy = point_lights[ 0 ].intensity / ( 4 * M_PI ) / 100;
      light_pos = point_lights[ 0 ].location;
    }
    bool snapped = false;
    uint32_t global_current_frame = 0u;
    bool light_space = config.light;
    float light_size = 0.1;
    while( !context.input_state->quit ) {
      if( context.input_state->a ) camera_angle += 0.01 * M_PI/2;
      if( context.input_state->d ) camera_angle -= 0.01 * M_PI/2;
      glm::vec3 camera_direction( std::sin( camera_angle ), 0, -std::cos( camera_angle ) );
      if( context.input_state->w ) camera_pos += camera_direction * glm::vec3( speed );
      if( context.input_state->s ) camera_pos -= camera_direction * glm::vec3( speed );
      if( context.input_state->e ) camera_pos[ 1 ] += speed;
      if( context.input_state->c ) camera_pos[ 1 ] -= speed;
      if( context.input_state->j ) light_energy += 0.05f;
      if( context.input_state->k ) light_energy -= 0.05f;
      if( context.input_state->up ) light_pos[ 2 ] += speed;
      if( context.input_state->down ) light_pos[ 2 ] -= speed;
      if( context.input_state->left ) light_pos[ 0 ] -= speed;
      if( context.input_state->right ) light_pos[ 0 ] += speed;
      if( context.input_state->n ) light_pos[ 1 ] -= speed;
      if( context.input_state->u ) light_pos[ 1 ] += speed;
      if( context.input_state->g ) light_space = !config.light;
      if( !context.input_state->g ) light_space = config.light;
      glm::mat4 lookat = glm::lookAt(
        camera_pos,
        camera_pos + camera_direction /*camera_pos - camera_dir*/,
        glm::vec3{ 0.f, camera_pos[ 1 ] + 100.f*scale, 0.f }
      );
      const auto begin_time = std::chrono::high_resolution_clock::now();
      auto &fe = fence[ current_frame ];
      for( size_t i = 0u; i != framebuffers.size(); ++i ) {
        auto wait_for_fences_result = context.device->waitForFences( 1, &*fe.fence[ i ], VK_TRUE, UINT64_MAX );
        if( wait_for_fences_result != vk::Result::eSuccess )
          vk::throwResultException( wait_for_fences_result, "waitForFences failed" );
        auto reset_fences_result = context.device->resetFences( 1, &*fe.fence[ i ] );
        if( reset_fences_result != vk::Result::eSuccess )
          vk::throwResultException( reset_fences_result, "waitForFences failed" );
      }
      if( global_current_frame == 4u && !snapped ) {
        snapped = true;
        dump_image( context, framebuffers[ 0 ][ current_frame % 3 ].color_image, "hoge.png", 0 );
      }
      auto image_index = context.device->acquireNextImageKHR( *context.swapchain, UINT64_MAX, *fe.image_acquired_semaphore, vk::Fence() );
      auto [light_projection_matrix,light_view_matrix,light_znear,light_zfar,light_frustum_width] = vw::get_light_matrix(
        projection[ 1 ],
        lookat,
        light_pos,
        camera_pos,
        1.0f
      );
      /*auto [light_projection_matrix,light_view_matrix,light_znear,light_zfar] = vw::get_aabb_light_matrix(
        document.node.min,
        document.node.max,
        light_pos
      );*/
      for( size_t i = 0u; i != framebuffers.size(); ++i ) {
        auto &fb = framebuffers[ i ][ image_index.value ];
        auto &gcb = command_buffer[ current_frame * framebuffers.size() + i ];
        gcb->reset( vk::CommandBufferResetFlags( 0 ) );
        gcb->begin(
          vk::CommandBufferBeginInfo()
            .setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
        );
        if( i == 0u ) {
          gcb->setDepthBias( 1.25f, 0.f, 1.75f );
        }
        auto dynamic_uniform = viewer::dynamic_uniforms_t()
          .set_projection_matrix( lhrh * ( light_space ? light_projection_matrix : projection[ 1 ] ) )
          .set_camera_matrix( light_space ? light_view_matrix : lookat )
          .set_light_vp_matrix0( lhrh*light_projection_matrix*light_view_matrix )
          .set_eye_pos( glm::vec4( camera_pos, 1.0 ) )
          .set_light_pos( glm::vec4( light_pos, 1.0 ) )
          .set_light_energy( light_energy )
          .set_light_znear( light_znear )
          .set_light_zfar( light_zfar )
          .set_light_frustum_width( light_frustum_width )
          .set_light_size( light_size );
        vw::transfer_buffer(
          context, 
          gcb,
          reinterpret_cast< uint8_t* >( &dynamic_uniform ),
          reinterpret_cast< uint8_t* >( &dynamic_uniform ) + sizeof( viewer::dynamic_uniforms_t ),
          temporary_dynamic_uniform_buffer[ current_frame ].buffer,
          dynamic_uniform_buffer[ current_frame ].buffer
        );
        vw::barrier_buffer( *gcb, dynamic_uniform_buffer[ i ].buffer );
        auto const pass_info = vk::RenderPassBeginInfo()
          .setRenderPass( *render_pass[ i ].render_pass )
          .setFramebuffer( *fb.framebuffer )
          .setRenderArea( vk::Rect2D( vk::Offset2D(0, 0), vk::Extent2D((uint32_t)fb.width, (uint32_t)fb.height) ) )
          .setClearValueCount( clear_values.size() )
          .setPClearValues( clear_values.data() );
        gcb->beginRenderPass( &pass_info, vk::SubpassContents::eInline );
        gcb->setViewport( 0, 1, &viewport[ i ] );
        gcb->setScissor( 0, 1, &scissor[ i ] );
        viewer::draw_node(
          context,
          *gcb,
          document.node,
          document.mesh,
          document.buffer,
          current_frame,
          i
        );
        gcb->endRenderPass();
        gcb->end();
        vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        graphics_queue.submit(
          vk::SubmitInfo()
            .setPWaitDstStageMask( &pipe_stage_flags )
            .setCommandBufferCount( 1 )
            .setPCommandBuffers( &*gcb )
            .setWaitSemaphoreCount( 1 )
            .setPWaitSemaphores( i ? &*fe.draw_complete_semaphore[ 0 ] : &*fe.image_acquired_semaphore )
            .setSignalSemaphoreCount( 1 )
            .setPSignalSemaphores( i ? &*fe.draw_complete_semaphore[ 1 ] : &*fe.draw_complete_semaphore[ 0 ] ),
            *fe.fence[ i ]
        );
      }
      auto const present_info = vk::PresentInfoKHR()
        .setWaitSemaphoreCount( 1 )
        .setPWaitSemaphores( &*fe.draw_complete_semaphore[ 1 ] )
        .setSwapchainCount( 1 )
        .setPSwapchains( &*context.swapchain )
        .setPImageIndices( &image_index.value );
      auto present_result = present_queue.presentKHR( &present_info );
      if( present_result != vk::Result::eSuccess )
        vk::throwResultException( present_result, "presentKHR failed" );
      glfwPollEvents();
      ++current_frame;
      ++global_current_frame;
      current_frame %= framebuffers[ 0 ].size();
      vw::wait_for_sync( begin_time );
    }
    vw::wait_for_idle( context );
  }
}


