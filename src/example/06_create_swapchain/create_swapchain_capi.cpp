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
  create_device( context, dext, dlayers );
  uint32_t formats_count = 0u;
  if( vkGetPhysicalDeviceSurfaceFormatsKHR( context.physical_device, *context.surface, &formats_count, nullptr ) != VK_SUCCESS )
    return 1;
  std::vector< VkSurfaceFormatKHR > formats( formats_count );
  if( vkGetPhysicalDeviceSurfaceFormatsKHR( context.physical_device, *context.surface, &formats_count, formats.data() ) != VK_SUCCESS )
    return 1;
  if( formats.empty() ) {
    std::cerr << "利用可能なピクセルフォーマットが無い" << std::endl;
    throw vw::unable_to_create_surface{};
  }
  const std::vector< VkFormat > supported_formats {
    VK_FORMAT_A2B10G10R10_UNORM_PACK32,
    VK_FORMAT_A2R10G10B10_UNORM_PACK32,
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_A8B8G8R8_UNORM_PACK32,
    VK_FORMAT_B5G5R5A1_UNORM_PACK16,
    VK_FORMAT_A1R5G5B5_UNORM_PACK16,
  };
  bool surface_format_selected = false;
  for( const auto &s: supported_formats ) {
    auto selected_format = std::find_if( formats.begin(), formats.end(), [&]( const auto &v ) { return v.format == s && v.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; } );
    if( selected_format != formats.end() ) {
      context.set_surface_format( *selected_format );
      surface_format_selected = true;
      break;
    }
  }
  if( !surface_format_selected ) {
    std::cerr << "互換性のあるピクセルフォーマットが無い" << std::endl;
    throw vw::unable_to_create_surface{};
  }
  VkSurfaceCapabilitiesKHR surface_capabilities;
  if( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( context.physical_device, *context.surface, &surface_capabilities ) != VK_SUCCESS )
    return 1;
  if ( surface_capabilities.currentExtent.width == static_cast< uint32_t >( -1 ) ) {
    context.swapchain_extent.width = context.width;
    context.swapchain_extent.height = context.height;
  } else {
    context.set_swapchain_extent( surface_capabilities.currentExtent );
    context.width = surface_capabilities.currentExtent.width;
    context.height = surface_capabilities.currentExtent.height;
  }
  context.swapchain_image_count = std::min( surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount );
  VkSwapchainCreateInfoKHR swapchain_create_info;
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.pNext = nullptr;
  swapchain_create_info.flags = 0;
  swapchain_create_info.surface = *context.surface;
  swapchain_create_info.minImageCount = context.swapchain_image_count;
  swapchain_create_info.imageFormat = static_cast< VkFormat >( context.surface_format.format );
  swapchain_create_info.imageColorSpace = static_cast< VkColorSpaceKHR >( context.surface_format.colorSpace );
  swapchain_create_info.imageExtent.width = context.swapchain_extent.width;
  swapchain_create_info.imageExtent.height = context.swapchain_extent.height;
  swapchain_create_info.imageArrayLayers = 1;
  swapchain_create_info.imageUsage = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchain_create_info.queueFamilyIndexCount = 1;
  swapchain_create_info.pQueueFamilyIndices = &context.present_queue_index;
  swapchain_create_info.preTransform = 
    ( surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) ?
    VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR :
    surface_capabilities.currentTransform;
  swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapchain_create_info.clipped = VK_TRUE;
  swapchain_create_info.oldSwapchain = nullptr;
  VkSwapchainKHR swapchain;
  if( vkCreateSwapchainKHR( *context.device, &swapchain_create_info, nullptr, &swapchain ) != VK_SUCCESS )
    return 1;
  vkDestroySwapchainKHR( *context.device, swapchain, nullptr );
}

