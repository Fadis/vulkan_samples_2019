#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vw/config.h>
#include <vw/instance.h>
#include <vw/list_device.h>
#include <vw/is_capable.h>
#include <vw/context.h>
#include <vw/exceptions.h>

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
    VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
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
  std::vector< vk::DescriptorPoolSize > descriptor_pool_size{
    vk::DescriptorPoolSize().setType( vk::DescriptorType::eUniformBuffer ).setDescriptorCount( 400 ),
    vk::DescriptorPoolSize().setType( vk::DescriptorType::eCombinedImageSampler ).setDescriptorCount( 400 )
  };
  std::vector< vk::DescriptorSetLayoutBinding > descriptor_set_layout_bindings{
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
      .setPImmutableSamplers( nullptr )
  };
  vw::context_t context;
  create_surface( context, *instance, configs, dext, dlayers );

  uint32_t queue_props_count = 0u;
  vkGetPhysicalDeviceQueueFamilyProperties( context.physical_device, &queue_props_count, nullptr );
  std::vector< VkQueueFamilyProperties  > queue_props( queue_props_count );
  vkGetPhysicalDeviceQueueFamilyProperties( context.physical_device, &queue_props_count, queue_props.data() );
  std::vector< VkBool32 > supported;
  supported.reserve( queue_props.size() );
  for( uint32_t i = 0; i < queue_props.size(); ++i ) {
    VkBool32 s = VK_FALSE;
    if( vkGetPhysicalDeviceSurfaceSupportKHR( context.physical_device, i, *context.surface, &s ) != VK_SUCCESS )
      return 1;
    supported.emplace_back( s );
  }
  context.set_graphics_queue_index( std::distance( queue_props.begin(), std::find_if( queue_props.begin(), queue_props.end(), []( const auto &v ) { return bool( v.queueFlags & VK_QUEUE_GRAPHICS_BIT ); } ) ) );
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
  std::vector< VkDeviceQueueCreateInfo > queues;
  VkDeviceQueueCreateInfo graphics_queue_create_info;
  graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  graphics_queue_create_info.pNext = nullptr;
  graphics_queue_create_info.flags = 0;
  graphics_queue_create_info.queueFamilyIndex = context.graphics_queue_index;
  graphics_queue_create_info.queueCount = 1;
  graphics_queue_create_info.pQueuePriorities = &priority;
  queues.emplace_back( graphics_queue_create_info );
  if ( context.graphics_queue_index != context.present_queue_index ) {
    VkDeviceQueueCreateInfo present_queue_create_info;
    present_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    present_queue_create_info.pNext = nullptr;
    present_queue_create_info.flags = 0;
    present_queue_create_info.queueFamilyIndex = context.present_queue_index;
    present_queue_create_info.queueCount = 1;
    present_queue_create_info.pQueuePriorities = &priority;
    queues.emplace_back( present_queue_create_info );
  }
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( context.physical_device, &features );
  VkPhysicalDeviceVulkan12Features vk12_features;
  vk12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  vk12_features.pNext = nullptr;
  vk12_features.samplerMirrorClampToEdge = false;
  vk12_features.drawIndirectCount = false;
  vk12_features.storageBuffer8BitAccess = false;
  vk12_features.uniformAndStorageBuffer8BitAccess = false;
  vk12_features.storagePushConstant8 = false;
  vk12_features.shaderBufferInt64Atomics = false;
  vk12_features.shaderSharedInt64Atomics = false;
  vk12_features.shaderFloat16 = false;
  vk12_features.shaderInt8 = false;
  vk12_features.descriptorIndexing = false;
  vk12_features.shaderInputAttachmentArrayDynamicIndexing = false;
  vk12_features.shaderUniformTexelBufferArrayDynamicIndexing = false;
  vk12_features.shaderStorageTexelBufferArrayDynamicIndexing = false;
  vk12_features.shaderUniformBufferArrayNonUniformIndexing = false;
  vk12_features.shaderSampledImageArrayNonUniformIndexing = false;
  vk12_features.shaderStorageBufferArrayNonUniformIndexing = false;
  vk12_features.shaderStorageImageArrayNonUniformIndexing = false;
  vk12_features.shaderInputAttachmentArrayNonUniformIndexing = false;
  vk12_features.shaderUniformTexelBufferArrayNonUniformIndexing = false;
  vk12_features.shaderStorageTexelBufferArrayNonUniformIndexing = false;
  vk12_features.descriptorBindingUniformBufferUpdateAfterBind = false;
  vk12_features.descriptorBindingSampledImageUpdateAfterBind = false;
  vk12_features.descriptorBindingStorageImageUpdateAfterBind = false;
  vk12_features.descriptorBindingStorageBufferUpdateAfterBind = false;
  vk12_features.descriptorBindingUniformTexelBufferUpdateAfterBind = false;
  vk12_features.descriptorBindingStorageTexelBufferUpdateAfterBind = false;
  vk12_features.descriptorBindingUpdateUnusedWhilePending = false;
  vk12_features.descriptorBindingPartiallyBound = false;
  vk12_features.descriptorBindingVariableDescriptorCount = false;
  vk12_features.runtimeDescriptorArray = false;
  vk12_features.samplerFilterMinmax = false;
  vk12_features.scalarBlockLayout = false;
  vk12_features.imagelessFramebuffer = false;
  vk12_features.uniformBufferStandardLayout = false;
  vk12_features.shaderSubgroupExtendedTypes = false;
  vk12_features.separateDepthStencilLayouts = false;
  vk12_features.hostQueryReset = false;
  vk12_features.timelineSemaphore = true;
  vk12_features.bufferDeviceAddress = false;
  vk12_features.bufferDeviceAddressCaptureReplay = false;
  vk12_features.bufferDeviceAddressMultiDevice = false;
  vk12_features.vulkanMemoryModel = false;
  vk12_features.vulkanMemoryModelDeviceScope = false;
  vk12_features.vulkanMemoryModelAvailabilityVisibilityChains = false;
  vk12_features.shaderOutputViewportIndex = false;
  vk12_features.shaderOutputLayer = false;
  vk12_features.subgroupBroadcastDynamicId = false;
  VkDeviceCreateInfo device_create_info;
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pNext = reinterpret_cast< void* >( &vk12_features );
  device_create_info.flags = 0;
  device_create_info.queueCreateInfoCount = queues.size();
  device_create_info.pQueueCreateInfos = queues.data();
  device_create_info.enabledExtensionCount = dext.size();
  device_create_info.ppEnabledExtensionNames = dext.data();
  device_create_info.enabledLayerCount = dlayers.size();
  device_create_info.ppEnabledLayerNames = dlayers.data();
  device_create_info.pEnabledFeatures = &features;
  VkDevice raw_device;
  if( vkCreateDevice( context.physical_device, &device_create_info, nullptr, &raw_device ) != VK_SUCCESS )
    return 1;
  {
    vk::ObjectDestroy< vk::NoParent, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > deleter( nullptr, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE () );
    context.set_device( vk::UniqueHandle< vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE >( raw_device, deleter ) );
  }
  VkCommandPoolCreateInfo graphics_command_pool_create_info;
  graphics_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  graphics_command_pool_create_info.pNext = nullptr;
  graphics_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  graphics_command_pool_create_info.queueFamilyIndex = context.graphics_queue_index;
  VkCommandPool raw_graphics_command_pool;
  if( vkCreateCommandPool( raw_device, &graphics_command_pool_create_info, nullptr, &raw_graphics_command_pool ) != VK_SUCCESS )
    return 1;
  {
    vk::ObjectDestroy< vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > deleter( *context.device, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE () );
    context.set_graphics_command_pool( vk::UniqueHandle< vk::CommandPool, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE >( raw_graphics_command_pool, deleter ) );
  }

  VkCommandPoolCreateInfo present_command_pool_create_info;
  present_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  present_command_pool_create_info.pNext = nullptr;
  present_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  present_command_pool_create_info.queueFamilyIndex = context.present_queue_index;
  VkCommandPool raw_present_command_pool;
  if( vkCreateCommandPool( raw_device, &present_command_pool_create_info, nullptr, &raw_present_command_pool ) != VK_SUCCESS )
    return 1;
  {
    vk::ObjectDestroy< vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > deleter( *context.device, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE () );
    context.set_present_command_pool( vk::UniqueHandle< vk::CommandPool, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE >( raw_present_command_pool, deleter ) );
  }
  create_swapchain( context );
  create_descriptor_set( context, descriptor_pool_size, descriptor_set_layout_bindings );
  create_allocator( context );
  create_pipeline_cache( context );

  VkSemaphoreTypeCreateInfo timeline_create_info;
  timeline_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
  timeline_create_info.pNext = NULL;
  timeline_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
  timeline_create_info.initialValue = 0;

  VkSemaphoreCreateInfo semaphore_create_info;
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphore_create_info.pNext = reinterpret_cast< void* >( &timeline_create_info );
  semaphore_create_info.flags = 0;
  VkSemaphore semaphore;
  if( vkCreateSemaphore( *context.device, &semaphore_create_info, nullptr, &semaphore ) != VK_SUCCESS )
    return 1;
  vkDestroySemaphore( *context.device, semaphore, nullptr );
}

