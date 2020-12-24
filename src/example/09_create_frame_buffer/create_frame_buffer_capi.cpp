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
#include <vw/render_pass.h>
#include <vw/image.h>

struct framebuffer_t {
  LIBSTAMP_SETTER( swapchain_image_view )
  LIBSTAMP_SETTER( depth_image )
  LIBSTAMP_SETTER( depth_image_view )
  LIBSTAMP_SETTER( framebuffer )
  vk::ImageView swapchain_image_view;
  vw::image_t depth_image;
  vk::ImageView depth_image_view;
  vk::Framebuffer framebuffer;
};
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
  create_swapchain( context );
  create_allocator( context );
  auto render_pass = vw::create_render_pass( context );

  std::vector< framebuffer_t > framebuffers;

  uint32_t swapchain_image_count = 0u;
  if( vkGetSwapchainImagesKHR( *context.device, *context.swapchain, &swapchain_image_count, nullptr ) != VK_SUCCESS )
    return 1;
  std::vector< VkImage > swapchain_images( swapchain_image_count );
  if( vkGetSwapchainImagesKHR( *context.device, *context.swapchain, &swapchain_image_count, swapchain_images.data() ) != VK_SUCCESS )
    return 1;
  VkImageSubresourceRange swapchain_image_view_subresource_range;
  swapchain_image_view_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  swapchain_image_view_subresource_range.baseMipLevel = 0;
  swapchain_image_view_subresource_range.levelCount = 1;
  swapchain_image_view_subresource_range.baseArrayLayer = 0;
  swapchain_image_view_subresource_range.layerCount = 1;
  VkComponentMapping swapchain_image_view_component_mapping;
  swapchain_image_view_component_mapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  swapchain_image_view_component_mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  swapchain_image_view_component_mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  swapchain_image_view_component_mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  VkImageSubresourceRange depth_image_view_subresource_range;
  depth_image_view_subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depth_image_view_subresource_range.baseMipLevel = 0;
  depth_image_view_subresource_range.levelCount = 1;
  depth_image_view_subresource_range.baseArrayLayer = 0;
  depth_image_view_subresource_range.layerCount = 1;
  VkComponentMapping depth_image_view_component_mapping;
  depth_image_view_component_mapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  depth_image_view_component_mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  depth_image_view_component_mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  depth_image_view_component_mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  for( auto &swapchain_image: swapchain_images ) {
    framebuffers.push_back( framebuffer_t() );
    VkImageViewCreateInfo swapchain_image_view_create_info;
    swapchain_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    swapchain_image_view_create_info.pNext = nullptr;
    swapchain_image_view_create_info.flags = 0;
    swapchain_image_view_create_info.image = swapchain_image;
    swapchain_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    swapchain_image_view_create_info.format = static_cast< VkFormat >( context.surface_format.format );
    swapchain_image_view_create_info.components = swapchain_image_view_component_mapping;
    swapchain_image_view_create_info.subresourceRange = swapchain_image_view_subresource_range;
    VkImageView swapchain_image_view;
    if( vkCreateImageView( *context.device, &swapchain_image_view_create_info, nullptr, &swapchain_image_view ) != VK_SUCCESS )
      return 1;
    framebuffers.back().set_swapchain_image_view( swapchain_image_view ); 

    framebuffers.back().set_depth_image( vw::get_image(
      context,
      vk::ImageCreateInfo()
        .setImageType( vk::ImageType::e2D )
        .setFormat( vk::Format::eD16Unorm )
        .setExtent( { context.width, context.height, 1 } )
        .setMipLevels( 1 )
        .setArrayLayers( 1 )
        .setUsage( vk::ImageUsageFlagBits::eDepthStencilAttachment ),
      VMA_MEMORY_USAGE_GPU_ONLY
    ) );

    VkImageViewCreateInfo depth_image_view_create_info;
    depth_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_create_info.pNext = nullptr;
    depth_image_view_create_info.flags = 0;
    depth_image_view_create_info.image = *framebuffers.back().depth_image.image;
    depth_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_create_info.format = VK_FORMAT_D16_UNORM;
    depth_image_view_create_info.components = depth_image_view_component_mapping;
    depth_image_view_create_info.subresourceRange = depth_image_view_subresource_range;
    VkImageView depth_image_view;
    if( vkCreateImageView( *context.device, &depth_image_view_create_info, nullptr, &depth_image_view ) != VK_SUCCESS ) {
      vkDestroyImageView( *context.device, swapchain_image_view, nullptr );
      return 1;
    }
    framebuffers.back().set_depth_image_view( depth_image_view );
    const std::array< VkImageView, 2 > attachments{
      framebuffers.back().swapchain_image_view,
      framebuffers.back().depth_image_view
    };
    VkFramebufferCreateInfo framebuffer_create_info;
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = nullptr;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = *render_pass.render_pass;
    framebuffer_create_info.attachmentCount = attachments.size();
    framebuffer_create_info.pAttachments = attachments.data();
    framebuffer_create_info.width = context.width;
    framebuffer_create_info.height = context.height;
    framebuffer_create_info.layers = 1;
    VkFramebuffer framebuffer;
    if( vkCreateFramebuffer( *context.device, &framebuffer_create_info, nullptr, &framebuffer ) != VK_SUCCESS ) {
      vkDestroyImageView( *context.device, depth_image_view, nullptr );
      vkDestroyImageView( *context.device, swapchain_image_view, nullptr );
      return 1;
    }
    vkDestroyFramebuffer( *context.device, framebuffer, nullptr );
    vkDestroyImageView( *context.device, depth_image_view, nullptr );
    vkDestroyImageView( *context.device, swapchain_image_view, nullptr );
  }
}

