#ifndef VW_BUFFER_H
#define VW_BUFFER_H
/*
 * Copyright (C) 2020 Naomasa Matsubayashi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>
#include <vw/context.h>
#include <vw/exceptions.h>
namespace vw {
  struct buffer_t {
    LIBSTAMP_SETTER( buffer )
    LIBSTAMP_SETTER( allocation )
    LIBSTAMP_SETTER( size )
    size_t size;
    std::shared_ptr< vk::Buffer > buffer;
    std::shared_ptr< VmaAllocation > allocation;
  };
  buffer_t get_buffer(
    const context_t &context,
    const vk::BufferCreateInfo &buffer_create_info,
    VmaMemoryUsage usage
  );
  void barrier_buffer(
    const vk::CommandBuffer &commands,
    const buffer_t &buffer
  );
  buffer_t load_buffer(
    const context_t &context,
    const std::vector< uint8_t > &data,
    vk::BufferUsageFlags usage
  );
  buffer_t load_buffer_from_file(
    const context_t &context,
    const std::string &filename,
    vk::BufferUsageFlags usage
  );
  buffer_t create_staging_buffer(
    const context_t &context,
    size_t size
  );
  buffer_t create_uniform_buffer(
    const context_t &context,
    size_t size
  );
  void transfer_buffer_internal(
    vk::UniqueHandle< vk::CommandBuffer, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > &commands,
    buffer_t &temporary,
    buffer_t &destination
  );
  template< typename Iterator >
  void transfer_buffer(
    const context_t &context,
    vk::UniqueHandle< vk::CommandBuffer, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > &commands,
    Iterator begin, Iterator end,
    buffer_t &temporary,
    buffer_t &destination
  ) {
    if( temporary.size != uint32_t( std::distance( begin, end ) ) ) throw invalid_argument( "dataとtemporaryのサイズが合わない" );
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
      std::copy( begin, end, mapped.get() );
    }
    transfer_buffer_internal( commands, temporary, destination );
  }
}
#endif

