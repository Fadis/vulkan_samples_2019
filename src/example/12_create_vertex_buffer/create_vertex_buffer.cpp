#include <iostream>
#include <vector>
#include <filesystem>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.hpp>
#include <vw/config.h>
#include <vw/instance.h>
#include <vw/list_device.h>
#include <vw/is_capable.h>
#include <vw/context.h>
#include <vw/exceptions.h>
#include <vw/buffer.h>
#include <vw/command_buffer.h>

struct vertex_t {
  vertex_t(
    const glm::vec3 &position_,
    const glm::vec3 &normal_,
    const glm::vec2 &texcoord_
  ) : position( position_ ), normal( normal_ ), texcoord( texcoord_ ) {}
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texcoord;
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
  create_allocator( context );
  const std::vector< vertex_t > vertices{
    { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
    { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
    { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } }
  };
  std::vector< uint8_t > data(
    reinterpret_cast< const uint8_t* >( vertices.data() ),
    reinterpret_cast< const uint8_t* >( vertices.data() + vertices.size() )
  );
  auto final_buffer = vw::get_buffer(
    context,
    vk::BufferCreateInfo()
      .setSize( data.size() )
      .setUsage( vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst ),
    VMA_MEMORY_USAGE_GPU_ONLY
  );
  auto temporary = vw::get_buffer(
    context,
    vk::BufferCreateInfo()
      .setSize( data.size() )
      .setUsage( vk::BufferUsageFlagBits::eTransferSrc ),
    VMA_MEMORY_USAGE_CPU_TO_GPU
  );
  {
    void* mapped_memory;
    const auto result = vmaMapMemory( *context.allocator, *temporary.allocation, &mapped_memory );
    if( result != VK_SUCCESS ) vk::throwResultException( vk::Result( result ), "バッファをマップできない" );
    std::shared_ptr< uint8_t > mapped(
      reinterpret_cast< uint8_t* >( mapped_memory ),
      [allocator=context.allocator,allocation=temporary.allocation]( uint8_t *p ) {
        if( p ) vmaUnmapMemory( *allocator, *allocation );
      }
    );
    std::copy( data.begin(), data.end(), mapped.get() );
  }
  auto commands_ = context.device->allocateCommandBuffersUnique(
    vk::CommandBufferAllocateInfo()
      .setCommandPool( *context.graphics_command_pool )
      .setLevel( vk::CommandBufferLevel::ePrimary )
      .setCommandBufferCount( 1 )
  );
  auto &commands = commands_.front();
  commands->begin(
    vk::CommandBufferBeginInfo()
      .setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
  );
  commands->copyBuffer(
    *temporary.buffer,
    *final_buffer.buffer,
    {
      vk::BufferCopy()
        .setSrcOffset( 0 )
        .setDstOffset( 0 )
        .setSize( data.size() )
    }
  );
  commands->end();
  auto graphics_queue = context.device->getQueue( context.graphics_queue_index, 0 );
  graphics_queue.submit(
  vk::SubmitInfo()
    .setCommandBufferCount( 1 )
    .setPCommandBuffers( &*commands ),
    vk::Fence()
  );
  graphics_queue.waitIdle();
}

