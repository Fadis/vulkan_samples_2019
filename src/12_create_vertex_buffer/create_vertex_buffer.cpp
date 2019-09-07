#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <boost/filesystem/path.hpp>
#include <boost/scope_exit.hpp>
#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <common/config.h>
#include <common/vertex.h>

int main( int argc, const char *argv[] ) {
  using vertex_t = common::vertex_t;
  auto config = common::parse_configs( argc, argv );
  glfwInit();
  std::shared_ptr< void > glfw_deleter( nullptr, []( const void* ) { glfwTerminate(); } );
  const auto app_info = vk::ApplicationInfo(
    config.prog_name.c_str(), VK_MAKE_VERSION(1, 0, 0),
    "sample_engine", VK_MAKE_VERSION(1, 0, 0),
    VK_API_VERSION_1_1
  );
  std::vector< const char* > ext;
  std::vector< const char* > layers;
  uint32_t required_ext_count = 0u;
  const auto required_ext = glfwGetRequiredInstanceExtensions( &required_ext_count );
    for( uint32_t i = 0u; i != required_ext_count; ++i )
        ext.emplace_back( required_ext[ i ] );
  if( config.validation ) layers.emplace_back( "VK_LAYER_LUNARG_standard_validation" );
  auto instance = vk::createInstanceUnique(
    vk::InstanceCreateInfo()
      .setPApplicationInfo( &app_info )
      .setEnabledExtensionCount( ext.size() )
      .setPpEnabledExtensionNames( ext.data() )
      .setEnabledLayerCount( layers.size() )
      .setPpEnabledLayerNames( layers.data() )
  );
  auto devices = instance->enumeratePhysicalDevices();
  if( devices.empty() ) {
    std::cerr << "利用可能なデバイスがない" << std::endl;
    return 1;
  }
  std::vector< const char* > dext{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  std::vector< const char* > dlayers;
  devices.erase( std::remove_if( devices.begin(), devices.end(), [&]( const auto &device ) -> bool {
    auto avail_dext = device.enumerateDeviceExtensionProperties();
    for( const char *w: dext )
      if( std::find_if( avail_dext.begin(), avail_dext.end(), [&]( const auto &v ) { return !strcmp( v.extensionName, w ); } ) == avail_dext.end() ) return true;
    const auto avail_dlayers = device.enumerateDeviceLayerProperties();
    for( const char *w: dlayers )
      if( std::find_if( avail_dlayers.begin(), avail_dlayers.end(), [&]( const auto &v ) { return !strcmp( v.layerName, w ); } ) == avail_dlayers.end() ) return true;
    const auto queue_props = device.getQueueFamilyProperties();
    bool has_compatible_queue = false;
    for( uint32_t i = 0; i < queue_props.size(); ++i ) {
      if( glfwGetPhysicalDevicePresentationSupport( *instance, device, i ) ) {
        has_compatible_queue = true;
        break;
      }
    }
    return !has_compatible_queue;
  } ), devices.end() );
  if( devices.empty() ) {
    std::cerr << "必要な拡張とレイヤーを備えたデバイスがない" << std::endl;
    return 1;
  }
  std::cout << "利用可能なデバイス" << std::endl;
  for( unsigned int index = 0u; index != devices.size(); ++index ) {
    const auto prop = devices[ index ].getProperties();
    std::cout << index << ": " << prop.deviceName << std::endl;
  }
  glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
  std::shared_ptr< GLFWwindow > window(
    glfwCreateWindow( config.width, config.height, config.prog_name.c_str(), config.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr ),
    [glfw_deleter]( GLFWwindow *p ) { if( p ) glfwDestroyWindow( p ); }
  );
  if( !window ) {
    std::cerr << "ウィンドウを作成できない" << std::endl;
    return 1;
  }
  VkSurfaceKHR raw_surface;
  if( glfwCreateWindowSurface( *instance, window.get(), nullptr, &raw_surface ) != VK_SUCCESS ) {
    std::cerr << "サーフェスを作成できない" << std::endl;
    return 1;
  }
  std::shared_ptr< vk::SurfaceKHR > surface(
    new vk::SurfaceKHR( raw_surface ),
    [&instance,window]( vk::SurfaceKHR *p ) {
      if( p ) {
        instance->destroySurfaceKHR( *p );
        delete p;
      }
    }
  );
  if( config.device_index >= devices.size() ) {
    std::cerr << config.device_index << "番目のデバイスは存在しない" << std::endl;
    return 1;
  }
  const auto &physical_device = devices[ config.device_index ];
  const auto queue_props = physical_device.getQueueFamilyProperties();
  std::vector< VkBool32 > supported;
  supported.reserve( queue_props.size() );
  for( uint32_t i = 0; i < queue_props.size(); ++i )
    supported.emplace_back( physical_device.getSurfaceSupportKHR( i, *surface ) );
  uint32_t graphics_queue_index =std::distance( queue_props.begin(), std::find_if( queue_props.begin(), queue_props.end(), []( const auto &v ) { return bool( v.queueFlags & vk::QueueFlagBits::eGraphics ); } ) );
  uint32_t present_queue_index =
    ( graphics_queue_index != queue_props.size() && supported[ graphics_queue_index ] == VK_TRUE ) ?
    graphics_queue_index :
    std::distance( supported.begin(), std::find( supported.begin(), supported.end(), VK_TRUE ) );
  if( graphics_queue_index == queue_props.size() || present_queue_index == queue_props.size() ) {
    std::cerr << "必要なキューが備わっていない " << std::endl;
    return 1;
  }
  const float priority = 0.0f;
  std::vector< vk::DeviceQueueCreateInfo > queues{
    vk::DeviceQueueCreateInfo()
      .setQueueFamilyIndex( graphics_queue_index ).setQueueCount( 1 ).setPQueuePriorities( &priority )
  };
  if ( graphics_queue_index != present_queue_index ) {
    queues.emplace_back(
      vk::DeviceQueueCreateInfo()
        .setQueueFamilyIndex( present_queue_index ).setQueueCount( 1 ).setPQueuePriorities( &priority )
    );
  }
  const auto features = physical_device.getFeatures();
  auto device = physical_device.createDeviceUnique(
    vk::DeviceCreateInfo()
      .setQueueCreateInfoCount( queues.size() )
      .setPQueueCreateInfos( queues.data() )
      .setEnabledExtensionCount( dext.size() )
      .setPpEnabledExtensionNames( dext.data() )
      .setEnabledLayerCount( dlayers.size() )
      .setPpEnabledLayerNames( dlayers.data() )
      .setPEnabledFeatures( &features )
  );
  VmaAllocatorCreateInfo allocator_info = {};
  allocator_info.physicalDevice = physical_device;
  allocator_info.device = *device;
  VmaAllocator allocator_;
  {
    const auto result = vmaCreateAllocator( &allocator_info, &allocator_ );
    if( result != VK_SUCCESS ) vk::throwResultException( vk::Result( result ), "アロケータを作成できない" );
  }
  std::shared_ptr< VmaAllocator > allocator(
    new VmaAllocator( allocator_ ),
    [&instance,&device]( VmaAllocator *p ) {
      if( p ) {
        vmaDestroyAllocator( *p );
        delete p;
      }
    }
  );
  std::vector< vertex_t > vertices{
    vertex_t( glm::f32vec3( 0.f, 0.f, 0.f ), glm::f32vec3( 0.f, 0.f, 1.f ), glm::f32vec3( 1.f, 0.f, 0.f ), glm::f32vec2( 0.f, 0.f ) ),
    vertex_t( glm::f32vec3( 1.f, 0.f, 0.f ), glm::f32vec3( 0.f, 0.f, 1.f ), glm::f32vec3( 1.f, 0.f, 0.f ), glm::f32vec2( 1.f, 0.f ) ),
    vertex_t( glm::f32vec3( 0.f, 1.f, 0.f ), glm::f32vec3( 0.f, 0.f, 1.f ), glm::f32vec3( 1.f, 0.f, 0.f ), glm::f32vec2( 0.f, 1.f ) )
  };
  const uint32_t vertex_buffer_size = vertices.size() * sizeof( vertex_t );
  VkBufferCreateInfo temporary_vertex_buffer_create_info =
    vk::BufferCreateInfo()
      .setSize( vertex_buffer_size )
      .setUsage( vk::BufferUsageFlagBits::eTransferSrc );
  VmaAllocationCreateInfo temporary_vertex_buffer_alloc_info = {};
  temporary_vertex_buffer_alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  VmaAllocation temporary_vertex_buffer_allocation;
  VkBuffer temporary_vertex_buffer_;
  {
    auto result = vmaCreateBuffer(
      *allocator,
      &temporary_vertex_buffer_create_info,
      &temporary_vertex_buffer_alloc_info,
      &temporary_vertex_buffer_,
      &temporary_vertex_buffer_allocation,
      nullptr
    );
    if( result != VK_SUCCESS ) vk::throwResultException( vk::Result( result ), "一時頂点バッファを作成できない" );
  }
  std::shared_ptr< vk::Buffer > temporary_vertex_buffer(
    new vk::Buffer( temporary_vertex_buffer_ ),
    [allocator,temporary_vertex_buffer_allocation]( vk::Buffer *p ) {
      if( p ) {
        vmaDestroyBuffer( *allocator, *p, temporary_vertex_buffer_allocation );
        delete p;
      }
    }
  );
  VkBufferCreateInfo vertex_buffer_create_info =
    vk::BufferCreateInfo()
      .setSize( vertex_buffer_size )
      .setUsage( vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst );
  VmaAllocationCreateInfo vertex_buffer_alloc_info = {};
  vertex_buffer_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  VmaAllocation vertex_buffer_allocation;
  VkBuffer vertex_buffer_;
  {
    auto result = vmaCreateBuffer(
      *allocator,
      &vertex_buffer_create_info,
      &vertex_buffer_alloc_info,
      &vertex_buffer_,
      &vertex_buffer_allocation,
      nullptr
    );
    if( result != VK_SUCCESS ) vk::throwResultException( vk::Result( result ), "頂点バッファを作成できない" );
  }
  std::shared_ptr< vk::Buffer > vertex_buffer(
    new vk::Buffer( vertex_buffer_ ),
    [allocator,vertex_buffer_allocation]( vk::Buffer *p ) {
      if( p ) {
        vmaDestroyBuffer( *allocator, *p, vertex_buffer_allocation );
        delete p;
      }
    }
  );
  {
    void *mapped_memory;
    const auto result = vmaMapMemory( *allocator, temporary_vertex_buffer_allocation, &mapped_memory );
    if( result != VK_SUCCESS ) vk::throwResultException( vk::Result( result ), "バッファをマップできない" );
    std::shared_ptr< void > mapped(
      mapped_memory,
      [allocator,temporary_vertex_buffer_allocation]( void *p ) {
        if( p ) vmaUnmapMemory( *allocator, temporary_vertex_buffer_allocation );
      }
    );
    memcpy( mapped.get(), vertices.data(), vertex_buffer_size );
  }
}

