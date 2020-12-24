#include <iostream>
#include <vector>
#include <filesystem>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>
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
  VkCommandBufferAllocateInfo command_buffer_allocate_info;
  command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_allocate_info.pNext = nullptr;
  command_buffer_allocate_info.commandPool = *context.graphics_command_pool;
  command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_allocate_info.commandBufferCount = 1;
  VkCommandBuffer commands;
  if( vkAllocateCommandBuffers( *context.device, &command_buffer_allocate_info, &commands ) != VK_SUCCESS )
    return 1;
  VkCommandBufferBeginInfo command_buffer_begin_info;
  command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  command_buffer_begin_info.pNext = nullptr;
  command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  command_buffer_begin_info.pInheritanceInfo = nullptr;
  if( vkBeginCommandBuffer( commands, &command_buffer_begin_info ) != VK_SUCCESS ) {
    vkFreeCommandBuffers( *context.device, *context.graphics_command_pool, 1, &commands );
    return 1;
  }
  VkBufferCopy buffer_copy;
  buffer_copy.srcOffset = 0;
  buffer_copy.dstOffset = 0;
  buffer_copy.size = data.size();
  vkCmdCopyBuffer( commands, *temporary.buffer, *final_buffer.buffer, 1, &buffer_copy );
  vkEndCommandBuffer( commands );
  VkQueue graphics_queue;
  vkGetDeviceQueue( *context.device, context.graphics_queue_index, 0, &graphics_queue );
  VkSubmitInfo submit_info;
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = nullptr;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitSemaphores = nullptr;
  submit_info.pWaitDstStageMask = nullptr;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &commands;
  submit_info.signalSemaphoreCount = 0;
  submit_info.pSignalSemaphores = nullptr;
  if( vkQueueSubmit( graphics_queue, 1, &submit_info, VK_NULL_HANDLE ) != VK_SUCCESS ) {
    vkFreeCommandBuffers( *context.device, *context.graphics_command_pool, 1, &commands );
    return 1;
  }
  if( vkQueueWaitIdle( graphics_queue ) != VK_SUCCESS ) {
    vkFreeCommandBuffers( *context.device, *context.graphics_command_pool, 1, &commands );
    return 1;
  }
  vkFreeCommandBuffers( *context.device, *context.graphics_command_pool, 1, &commands );
}
