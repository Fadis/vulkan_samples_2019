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
  const auto formats = context.physical_device.getSurfaceFormatsKHR( *context.surface );
  if( formats.empty() ) {
    std::cerr << "利用可能なピクセルフォーマットが無い" << std::endl;
    throw vw::unable_to_create_surface{};
  }
  const std::vector< vk::Format > supported_formats {
    vk::Format::eA2B10G10R10UnormPack32,
    vk::Format::eA2R10G10B10UnormPack32,
    vk::Format::eB8G8R8A8Unorm,
    vk::Format::eR8G8B8A8Unorm,
    vk::Format::eA8B8G8R8UnormPack32,
    vk::Format::eB5G5R5A1UnormPack16,
    vk::Format::eA1R5G5B5UnormPack16
  };
  bool surface_format_selected = false;
  for( const auto &s: supported_formats ) {
    auto selected_format = std::find_if( formats.begin(), formats.end(), [&]( const auto &v ) { return v.format == s && v.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; } );
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
  const auto surface_capabilities = context.physical_device.getSurfaceCapabilitiesKHR( *context.surface );
  if ( surface_capabilities.currentExtent.width == static_cast< uint32_t >( -1 ) ) {
    context.swapchain_extent.width = context.width;
    context.swapchain_extent.height = context.height;
  } else {
    context.set_swapchain_extent( surface_capabilities.currentExtent );
    context.width = surface_capabilities.currentExtent.width;
    context.height = surface_capabilities.currentExtent.height;
  }
  context.swapchain_image_count = std::min( surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount );
  context.set_swapchain( context.device->createSwapchainKHRUnique(
    vk::SwapchainCreateInfoKHR()
      .setSurface( *context.surface )
      .setMinImageCount( context.swapchain_image_count )
      .setImageFormat( context.surface_format.format )
      .setImageColorSpace( context.surface_format.colorSpace )
      .setImageExtent( { context.swapchain_extent.width, context.swapchain_extent.height } )
      .setImageArrayLayers( 1 )
      .setImageUsage( vk::ImageUsageFlagBits::eColorAttachment )
      .setImageSharingMode( vk::SharingMode::eExclusive )
      .setQueueFamilyIndexCount( 1 )
      .setPQueueFamilyIndices( &context.present_queue_index )
      .setPreTransform(
        ( surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ) ?
        vk::SurfaceTransformFlagBitsKHR::eIdentity :
        surface_capabilities.currentTransform
      )
      .setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
      .setPresentMode( vk::PresentModeKHR::eFifo )
      .setClipped( true )
  ) );

}

