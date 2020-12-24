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

  const std::vector< vk::AttachmentDescription > attachments{
    vk::AttachmentDescription()
      .setFormat( context.surface_format.format )
      .setSamples( vk::SampleCountFlagBits::e1 )
      .setLoadOp( vk::AttachmentLoadOp::eClear )
      .setStoreOp( vk::AttachmentStoreOp::eStore )
      .setStencilLoadOp( vk::AttachmentLoadOp::eDontCare )
      .setStencilStoreOp( vk::AttachmentStoreOp::eDontCare )
      .setInitialLayout( vk::ImageLayout::eUndefined )
      .setFinalLayout( vk::ImageLayout::ePresentSrcKHR ),
    vk::AttachmentDescription()
      .setFormat( vk::Format::eD16Unorm )
      .setSamples( vk::SampleCountFlagBits::e1 )
      .setLoadOp( vk::AttachmentLoadOp::eClear )
      .setStoreOp( vk::AttachmentStoreOp::eDontCare )
      .setStencilLoadOp( vk::AttachmentLoadOp::eDontCare )
      .setStencilStoreOp( vk::AttachmentStoreOp::eDontCare )
      .setInitialLayout( vk::ImageLayout::eUndefined )
      .setFinalLayout( vk::ImageLayout::eDepthStencilAttachmentOptimal )
  };
  const std::vector< vk::AttachmentReference > color_reference{
    vk::AttachmentReference().setAttachment( 0 ).setLayout( vk::ImageLayout::eColorAttachmentOptimal )
  };
  const auto depth_reference =
    vk::AttachmentReference().setAttachment( 1 ).setLayout( vk::ImageLayout::eDepthStencilAttachmentOptimal );
  const std::vector< vk::SubpassDescription > subpass{
    vk::SubpassDescription()
      .setPipelineBindPoint( vk::PipelineBindPoint::eGraphics )
      .setColorAttachmentCount( color_reference.size() )
      .setPColorAttachments( color_reference.data() )
      .setPDepthStencilAttachment( &depth_reference )
  };
  auto render_pass = context.device->createRenderPassUnique(
    vk::RenderPassCreateInfo()
      .setAttachmentCount( attachments.size() )
      .setPAttachments( attachments.data() )
      .setSubpassCount( subpass.size() )
      .setPSubpasses( subpass.data() )
  );
}

