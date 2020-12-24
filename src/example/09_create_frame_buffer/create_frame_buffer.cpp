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
#include <vw/render_pass.h>
#include <vw/framebuffer.h>
#include <vw/image.h>

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

  std::vector< vw::framebuffer_t > framebuffers;
  for( auto &swapchain_image: context.device->getSwapchainImagesKHR( *context.swapchain ) ) {
    framebuffers.push_back( vw::framebuffer_t() );
    framebuffers.back().set_swapchain_image_view(
      context.device->createImageViewUnique(
        vk::ImageViewCreateInfo()
          .setImage( swapchain_image )
          .setViewType( vk::ImageViewType::e2D )
          .setFormat( context.surface_format.format )
          .setSubresourceRange( vk::ImageSubresourceRange( vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 ) )
      )
    );
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
    framebuffers.back().set_depth_image_view(
      context.device->createImageViewUnique(
       vk::ImageViewCreateInfo()
        .setImage( *framebuffers.back().depth_image.image )
        .setViewType( vk::ImageViewType::e2D )
        .setFormat( vk::Format::eD16Unorm )
        .setSubresourceRange( vk::ImageSubresourceRange( vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 ) )
      )
    );
    const std::array< vk::ImageView, 2 > attachments{
      *framebuffers.back().swapchain_image_view,
      *framebuffers.back().depth_image_view
    };
    framebuffers.back().set_framebuffer(
      context.device->createFramebufferUnique(
        vk::FramebufferCreateInfo()
          .setRenderPass( *render_pass.render_pass )
          .setAttachmentCount( attachments.size() )
          .setPAttachments( attachments.data() )
          .setWidth( context.width )
          .setHeight( context.height )
          .setLayers( 1 )
      )
    );
  }
}

