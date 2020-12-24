#include <filesystem>
#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vw/config.h>
#include <vw/instance.h>
#include <vw/list_device.h>
#include <vw/is_capable.h>
#include <vw/context.h>
#include <vw/exceptions.h>
#include <vw/buffer.h>
#include <vw/render_pass.h>
#include <vw/shader.h>
#include <vw/pipeline.h>
#include <vw/framebuffer.h>
#include <vw/image.h>
#include <vw/buffer.h>
#include <vw/wait_for_idle.h>
#include <vw/command_buffer.h>

int main( int argc, const char *argv[] ) {
  const auto configs = vw::parse_configs( argc, argv );
  auto instance = vw::create_instance(
    configs,
    {},
    {}
  );
  if( configs.list ) {
    vw::list_devices( *instance, configs );
    return 0;
  }
  std::vector< const char* > dext{
    VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME
  };
  std::vector< const char* > dlayers{};
  if( !vw::is_capable(
    *instance,
    configs,
    dext,
    dlayers
  ) ) {
    std::cout << "指定された条件に合うデバイスは無かった" << std::endl;
    return 0;
  }
  vw::context_t context;
  create_surface( context, *instance, configs, dext, dlayers );

  const auto queue_props = context.physical_device.getQueueFamilyProperties();
  std::vector< VkBool32 > supported;
  supported.reserve( queue_props.size() );
  for( uint32_t i = 0; i < queue_props.size(); ++i )
    supported.emplace_back( context.physical_device.getSurfaceSupportKHR( i, *context.surface ) );
  context.set_graphics_queue_index( std::distance( queue_props.begin(), std::find_if( queue_props.begin(), queue_props.end(), []( const auto &v ) { return bool( v.queueFlags & vk::QueueFlagBits::eGraphics ); } ) ) );
  context.set_present_queue_index(
    ( context.graphics_queue_index != queue_props.size() && supported[ context.graphics_queue_index ] == VK_TRUE ) ?
    context.graphics_queue_index :
    std::distance( supported.begin(), std::find( supported.begin(), supported.end(), VK_TRUE ) )
  );
  if( context.graphics_queue_index == queue_props.size() || context.present_queue_index == queue_props.size() ) {
    std::cerr << "必要なキューが備わっていない " << std::endl;
    throw vw::unable_to_create_surface{};
  }
  const float priority = 0.0f;
  std::vector< vk::DeviceQueueCreateInfo > queues{
    vk::DeviceQueueCreateInfo()
      .setQueueFamilyIndex( context.graphics_queue_index ).setQueueCount( 1 ).setPQueuePriorities( &priority )
  };
  if ( context.graphics_queue_index != context.present_queue_index ) {
    queues.emplace_back(
      vk::DeviceQueueCreateInfo()
        .setQueueFamilyIndex( context.present_queue_index ).setQueueCount( 1 ).setPQueuePriorities( &priority )
    );
  }
  vk::PhysicalDeviceVulkan12Features vk12_features;
  vk12_features.timelineSemaphore = true;
  const auto features = context.physical_device.getFeatures();
  context.set_device( context.physical_device.createDeviceUnique(
    vk::DeviceCreateInfo()
      .setPNext( &vk12_features )
      .setQueueCreateInfoCount( queues.size() )
      .setPQueueCreateInfos( queues.data() )
      .setEnabledExtensionCount( dext.size() )
      .setPpEnabledExtensionNames( dext.data() )
      .setEnabledLayerCount( dlayers.size() )
      .setPpEnabledLayerNames( dlayers.data() )
      .setPEnabledFeatures( &features )
  ) );
  context.set_graphics_command_pool( context.device->createCommandPoolUnique(
    vk::CommandPoolCreateInfo()
      .setQueueFamilyIndex( context.graphics_queue_index )
      .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
  ) );
  context.set_present_command_pool( context.device->createCommandPoolUnique(
    vk::CommandPoolCreateInfo()
      .setQueueFamilyIndex( context.present_queue_index )
      .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
  ) );
  create_swapchain( context );
  create_descriptor_set( 
    context,
    {
      vk::DescriptorPoolSize().setType( vk::DescriptorType::eStorageBuffer ).setDescriptorCount( 400 )
    },
    {
      vk::DescriptorSetLayoutBinding()
        .setDescriptorType( vk::DescriptorType::eStorageBuffer )
        .setDescriptorCount( 1 )
        .setBinding( 1 )
        .setStageFlags( vk::ShaderStageFlagBits::eCompute )
        .setPImmutableSamplers( nullptr )
    }
  );
  create_allocator( context );
  create_pipeline_cache( context );

  const auto timeline_create_info =
    vk::SemaphoreTypeCreateInfo()
     .setSemaphoreType( vk::SemaphoreType::eTimeline );
  auto semaphore = 
    context.device->createSemaphoreUnique(
      vk::SemaphoreCreateInfo()
        .setPNext( &timeline_create_info )
    );

  const size_t buffer_size = 256 * sizeof( float );
  auto host_storage = vw::get_buffer(
    context,
    vk::BufferCreateInfo()
      .setSize( buffer_size )
      .setUsage( vk::BufferUsageFlagBits::eStorageBuffer ),
    VMA_MEMORY_USAGE_CPU_TO_GPU
  );

  const auto dbi = vk::DescriptorBufferInfo()
    .setBuffer( *host_storage.buffer )
    .setOffset( 0 )
    .setRange( buffer_size );
  std::array< float, 1 > spec_data{ 0.5f };
  std::array< vk::SpecializationMapEntry, 4 > spec_ent {
    vk::SpecializationMapEntry()
      .setConstantID( 3 )
      .setOffset( 0 )
      .setSize( 4 )
  };
  auto spec = vk::SpecializationInfo()
    .setMapEntryCount( spec_ent.size() )
    .setPMapEntries( spec_ent.data() )
    .setDataSize( spec_data.size() * sizeof( float ) )
    .setPData( spec_data.data() );
  auto cs = get_shader( context, std::filesystem::path( configs.shader ) / std::filesystem::path( "add.comp.spv" ) );
  const std::vector< vk::PushConstantRange > push_constant_range{};
  std::vector< vk::DescriptorSetLayout > raw_descriptor_set_layout;
  raw_descriptor_set_layout.reserve( context.descriptor_set_layout.size() );
  std::transform(
    context.descriptor_set_layout.begin(),
    context.descriptor_set_layout.end(),
    std::back_inserter( raw_descriptor_set_layout ),
    []( const auto &v ) { return *v; }
  );
  auto pipeline_layout = context.device->createPipelineLayoutUnique(
    vk::PipelineLayoutCreateInfo()
      .setSetLayoutCount( raw_descriptor_set_layout.size() )
      .setPSetLayouts( raw_descriptor_set_layout.data() )
      .setPushConstantRangeCount( push_constant_range.size() )
      .setPPushConstantRanges( push_constant_range.data() )
  );
  auto pipeline_create_info =
    vk::ComputePipelineCreateInfo()
      .setStage(
        vk::PipelineShaderStageCreateInfo()
          .setStage( vk::ShaderStageFlagBits::eCompute )
          .setModule( *cs )
          .setPName( "main" )
          .setPSpecializationInfo( &spec )
      )
      .setLayout( *pipeline_layout );
  auto raw_pipeline = context.device->createComputePipeline(
    *context.pipeline_cache, pipeline_create_info
  );
  vk::ObjectDestroy< vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > deleter( *context.device, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE () );
  auto pipeline = vk::UniqueHandle< vk::Pipeline, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE >( raw_pipeline, deleter );

  context.device->updateDescriptorSets(
    std::vector< vk::WriteDescriptorSet >{
       vk::WriteDescriptorSet()
         .setDstSet( *context.descriptor_set[ 0 ] )
         .setDstBinding( 0 )
         .setDescriptorType( vk::DescriptorType::eStorageBuffer )
         .setDescriptorCount( 1 )
         .setPBufferInfo( &dbi )
    },
    nullptr
  );

  std::vector< vk::BufferMemoryBarrier > fill_barrier;
  fill_barrier.emplace_back(
    vk::BufferMemoryBarrier()
      .setSrcAccessMask( vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite )
      .setDstAccessMask( vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite )
      .setBuffer( *host_storage.buffer )
      .setOffset( 0 )
      .setSize( buffer_size )
  );
  auto graphics_queue = context.device->getQueue( context.graphics_queue_index, 0 );
  std::vector< uint32_t > ds_offset{};
  auto command_buffer = vw::get_command_buffer( context, true, 1 );
  auto &commands = command_buffer[ 0 ];
  commands->reset( vk::CommandBufferResetFlags( 0 ) );
  commands->begin(
    vk::CommandBufferBeginInfo()
      .setFlags( vk::CommandBufferUsageFlagBits::eSimultaneousUse )
  );
  commands->fillBuffer( *host_storage.buffer, 0, buffer_size, 0 );
  commands->pipelineBarrier(
    vk::PipelineStageFlagBits::eComputeShader,
    vk::PipelineStageFlagBits::eComputeShader,
    vk::DependencyFlagBits::eDeviceGroup,
    std::vector< vk::MemoryBarrier >{},
    fill_barrier,
    std::vector< vk::ImageMemoryBarrier >{}
  );
  {
    std::vector< vk::DescriptorSet > descriptor_set{
      *context.descriptor_set[ 0 ]
    };
    commands->bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      *pipeline_layout,
      0,
      descriptor_set,
      {}
    );
  }
  commands->bindPipeline( vk::PipelineBindPoint::eCompute, *pipeline );
  commands->dispatch( 16, 16, 1 );
  commands->end();
  uint64_t current_semaphore_value = 0u;
  uint64_t next_semaphore_value = 1u;
  {
    const auto timeline_semaphore_submit_info =
      vk::TimelineSemaphoreSubmitInfo()
        .setWaitSemaphoreValueCount( 1 )
        .setPWaitSemaphoreValues( &current_semaphore_value )
        .setSignalSemaphoreValueCount( 1 )
        .setPSignalSemaphoreValues( &next_semaphore_value );
    graphics_queue.submit(
      vk::SubmitInfo()
        .setPNext( &timeline_semaphore_submit_info )
        .setCommandBufferCount( 1 )
        .setPCommandBuffers( &*commands )
        .setWaitSemaphoreCount( 1 )
        .setPWaitSemaphores( &*semaphore )
        .setSignalSemaphoreCount( 1 )
        .setPSignalSemaphores( &*semaphore ),
        vk::Fence()
    );
  }
  ++current_semaphore_value;
  ++next_semaphore_value;
  {
    const auto timeline_semaphore_submit_info =
      vk::TimelineSemaphoreSubmitInfo()
        .setWaitSemaphoreValueCount( 1 )
        .setPWaitSemaphoreValues( &current_semaphore_value )
        .setSignalSemaphoreValueCount( 1 )
        .setPSignalSemaphoreValues( &next_semaphore_value );
    graphics_queue.submit(
      vk::SubmitInfo()
        .setPNext( &timeline_semaphore_submit_info )
        .setCommandBufferCount( 1 )
        .setPCommandBuffers( &*commands )
        .setWaitSemaphoreCount( 1 )
        .setPWaitSemaphores( &*semaphore )
        .setSignalSemaphoreCount( 1 )
        .setPSignalSemaphores( &*semaphore ),
        vk::Fence()
    );
  }
  ++current_semaphore_value;
  ++next_semaphore_value;
  context.device->waitSemaphores(
    vk::SemaphoreWaitInfo()
      .setSemaphoreCount( 1 )
      .setPSemaphores( &*semaphore )
      .setPValues( &current_semaphore_value ),
    UINT64_MAX
  );
}

