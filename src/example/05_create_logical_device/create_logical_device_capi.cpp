#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
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
  VkDeviceCreateInfo device_create_info;
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pNext = nullptr;
  device_create_info.flags = 0;
  device_create_info.queueCreateInfoCount = queues.size();
  device_create_info.pQueueCreateInfos = queues.data();
  device_create_info.enabledExtensionCount = dext.size();
  device_create_info.ppEnabledExtensionNames = dext.data();
  device_create_info.enabledLayerCount = dlayers.size();
  device_create_info.ppEnabledLayerNames = dlayers.data();
  device_create_info.pEnabledFeatures = &features;
  VkDevice device;
  if( vkCreateDevice( context.physical_device, &device_create_info, nullptr, &device ) != VK_SUCCESS )
    return 1;
  VkCommandPoolCreateInfo graphics_command_pool_create_info;
  graphics_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  graphics_command_pool_create_info.pNext = nullptr;
  graphics_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  graphics_command_pool_create_info.queueFamilyIndex = context.graphics_queue_index;
  VkCommandPool graphics_command_pool;
  if( vkCreateCommandPool( device, &graphics_command_pool_create_info, nullptr, &graphics_command_pool ) != VK_SUCCESS ) {
    vkDestroyDevice( device, nullptr );
    return 1;
  }
  VkCommandPoolCreateInfo present_command_pool_create_info;
  present_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  present_command_pool_create_info.pNext = nullptr;
  present_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  present_command_pool_create_info.queueFamilyIndex = context.present_queue_index;
  VkCommandPool present_command_pool;
  if( vkCreateCommandPool( device, &present_command_pool_create_info, nullptr, &present_command_pool ) != VK_SUCCESS ) {
    vkDestroyDevice( device, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    return 1;
  }
  vkDestroyCommandPool( device, present_command_pool, nullptr );
  vkDestroyCommandPool( device, graphics_command_pool, nullptr );
  vkDestroyDevice( device, nullptr );
}

