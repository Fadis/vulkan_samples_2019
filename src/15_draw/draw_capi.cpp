#include <fstream>
#include <iterator>
#include <cstring>
#include <memory>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <boost/filesystem/path.hpp>
#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <common/config.h>
#include <common/vertex.h>
#include <common/draw_state.h>
#include <common/mesh.h>

int main( int argc, const char *argv[] ) {
  using draw_state_t = common::draw_state_t;
  using vertex_t = common::vertex_t;
  auto config = common::parse_configs( argc, argv );
  common::scene mesh( config.mesh_file );
  glfwInit();
  std::vector< const char* > ext;
  std::vector< const char* > layers;
  uint32_t required_ext_count = 0u;
  const auto required_ext = glfwGetRequiredInstanceExtensions( &required_ext_count );
    for( uint32_t i = 0u; i != required_ext_count; ++i )
        ext.emplace_back( required_ext[ i ] );
  VkApplicationInfo application_info;
  application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  application_info.pNext = nullptr;
  application_info.pApplicationName = config.prog_name.c_str();
  application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.pEngineName ="sample_engine";
  application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.apiVersion = VK_API_VERSION_1_1;
  if( config.validation ) layers.emplace_back( "VK_LAYER_LUNARG_standard_validation" );
  VkInstanceCreateInfo create_instance_info;
  create_instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_instance_info.pNext = nullptr;
  create_instance_info.flags = 0;
  create_instance_info.pApplicationInfo = &application_info;
  create_instance_info.enabledLayerCount = layers.size();
  create_instance_info.ppEnabledLayerNames = layers.data();
  create_instance_info.enabledExtensionCount = ext.size();
  create_instance_info.ppEnabledExtensionNames = ext.data();
  VkInstance instance;
  VkResult result = vkCreateInstance(
    &create_instance_info,
    nullptr,
    &instance
  );
  if( result != VK_SUCCESS ) {
    vkDestroyInstance(
      instance,
      nullptr
    );
    glfwTerminate();
    return 1;
  }
  uint32_t device_count = 0;
  if( vkEnumeratePhysicalDevices( instance, &device_count, nullptr ) != VK_SUCCESS ) {
    vkDestroyInstance(
      instance,
      nullptr
    );
    glfwTerminate();
    return 1;
  }
  if( device_count == 0 ) {
    std::cerr << "利用可能なデバイスがない" << std::endl;
    return 1;
  }
  std::vector< VkPhysicalDevice > devices;
  devices.resize( device_count );
  if( vkEnumeratePhysicalDevices( instance, &device_count, devices.data() ) != VK_SUCCESS ) {
    std::cerr << "利用可能なデバイスを取得できない" << std::endl;
    vkDestroyInstance(
      instance,
      nullptr
    );
    glfwTerminate();
    return 1;
  }
  std::vector< const char* > dext{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  std::vector< const char* > dlayers;
  devices.erase( std::remove_if( devices.begin(), devices.end(), [&]( const auto &device ) -> bool {
    uint32_t ext_count = 0u;
    if( vkEnumerateDeviceExtensionProperties( device, nullptr, &ext_count, nullptr ) != VK_SUCCESS )
      return true;
    std::vector< VkExtensionProperties > avail_dext;
    avail_dext.resize( ext_count );
    if( vkEnumerateDeviceExtensionProperties( device, nullptr, &ext_count, avail_dext.data() ) != VK_SUCCESS )
      return true;
    for( const char *w: dext )
      if( std::find_if( avail_dext.begin(), avail_dext.end(), [&]( const auto &v ) { return !strcmp( v.extensionName, w ); } ) == avail_dext.end() ) return true;
    uint32_t layer_count = 0u;
    if( vkEnumerateDeviceLayerProperties( device, &layer_count, nullptr ) != VK_SUCCESS )
      return true;
    std::vector< VkLayerProperties > avail_dlayers;
    avail_dlayers.resize( layer_count );
    if( vkEnumerateDeviceLayerProperties( device, &layer_count, avail_dlayers.data() ) != VK_SUCCESS )
      return true;
    for( const char *w: dlayers )
      if( std::find_if( avail_dlayers.begin(), avail_dlayers.end(), [&]( const auto &v ) { return !strcmp( v.layerName, w ); } ) == avail_dlayers.end() ) return true;
    uint32_t queue_props_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queue_props_count, nullptr );
    std::vector< VkQueueFamilyProperties > queue_props;
    queue_props.resize( queue_props_count );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queue_props_count, queue_props.data() );
    bool has_compatible_queue = false;
    for( uint32_t i = 0; i < queue_props.size(); ++i ) {
      if( glfwGetPhysicalDevicePresentationSupport( instance, device, i ) ) {
        has_compatible_queue = true;
        break;
      }
    }
    return !has_compatible_queue;
  } ), devices.end() );
  if( devices.empty() ) {
    std::cerr << "必要な拡張とレイヤーを備えたデバイスがない" << std::endl;
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::cout << "利用可能なデバイス" << std::endl;
  for( unsigned int index = 0u; index != devices.size(); ++index ) {
    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties( devices[ index ], &prop );
    std::cout << index << ": " << prop.deviceName << std::endl;
  }
  glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
  GLFWwindow *window = glfwCreateWindow( config.width, config.height, config.prog_name.c_str(), config.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr );
  if( !window ) {
    std::cerr << "ウィンドウを作成できない" << std::endl;
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  VkSurfaceKHR surface;
  if( glfwCreateWindowSurface( instance, window, nullptr, &surface ) != VK_SUCCESS ) {
    std::cerr << "サーフェスを作成できない" << std::endl;
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  if( config.device_index >= devices.size() ) {
    std::cerr << config.device_index << "番目のデバイスは存在しない" << std::endl;
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  const auto &physical_device = devices[ config.device_index ];
  uint32_t queue_props_count;
  vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &queue_props_count, nullptr );
  std::vector< VkQueueFamilyProperties > queue_props( queue_props_count );
  vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &queue_props_count, queue_props.data() );
  std::vector< VkBool32 > supported;
  supported.reserve( queue_props.size() );
  for( uint32_t i = 0; i < queue_props.size(); ++i ) {
    VkBool32 v;
    if( vkGetPhysicalDeviceSurfaceSupportKHR( physical_device, i, surface, &v ) != VK_SUCCESS ) {
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    supported.emplace_back( v );
  }
  uint32_t graphics_queue_index = std::distance( queue_props.begin(), std::find_if( queue_props.begin(), queue_props.end(), []( const auto &v ) { return bool( v.queueFlags & VK_QUEUE_GRAPHICS_BIT ); } ) );
  uint32_t present_queue_index =
    ( graphics_queue_index != queue_props.size() && supported[ graphics_queue_index ] == VK_TRUE ) ?
    graphics_queue_index :
    std::distance( supported.begin(), std::find( supported.begin(), supported.end(), VK_TRUE ) );
  if( graphics_queue_index == queue_props.size() || present_queue_index == queue_props.size() ) {
    std::cerr << "必要なキューが備わっていない " << std::endl;
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  const float priority = 0.0f;
  std::vector< VkDeviceQueueCreateInfo > queues;
  VkDeviceQueueCreateInfo graphics_queue_create_info;
  graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  graphics_queue_create_info.pNext = nullptr;
  graphics_queue_create_info.flags = 0;
  graphics_queue_create_info.queueFamilyIndex = graphics_queue_index;
  graphics_queue_create_info.queueCount = 1;
  graphics_queue_create_info.pQueuePriorities = &priority;
  queues.push_back( graphics_queue_create_info );
  if ( graphics_queue_index != present_queue_index ) {
    VkDeviceQueueCreateInfo present_queue_create_info;
    present_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    present_queue_create_info.pNext = nullptr;
    present_queue_create_info.flags = 0;
    present_queue_create_info.queueFamilyIndex = present_queue_index;
    present_queue_create_info.queueCount = 1;
    present_queue_create_info.pQueuePriorities = &priority;
    queues.push_back( present_queue_create_info );
  }
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( physical_device, &features );
  VkDeviceCreateInfo device_create_info;
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pNext = nullptr;
  device_create_info.flags = 0;
  device_create_info.queueCreateInfoCount = queues.size();
  device_create_info.pQueueCreateInfos = queues.data();
  device_create_info.enabledLayerCount = dlayers.size();
  device_create_info.ppEnabledLayerNames = dlayers.data();
  device_create_info.enabledExtensionCount = dext.size();
  device_create_info.ppEnabledExtensionNames = dext.data();
  device_create_info.pEnabledFeatures = &features;
  VkDevice device;
  vkCreateDevice( physical_device, &device_create_info, nullptr, &device );
  VkQueue graphics_queue;
  VkQueue present_queue;
  if( graphics_queue_index != present_queue_index ) {
    vkGetDeviceQueue( device, graphics_queue_index,  0, &graphics_queue );
    vkGetDeviceQueue( device, present_queue_index,  0, &present_queue );
  }
  else {
    vkGetDeviceQueue( device, graphics_queue_index,  0, &graphics_queue );
    present_queue = graphics_queue;
  }
  VkCommandPool graphics_command_pool;
  VkCommandPoolCreateInfo graphics_command_pool_create_info;
  graphics_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  graphics_command_pool_create_info.pNext = nullptr;
  graphics_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  graphics_command_pool_create_info.queueFamilyIndex = graphics_queue_index;
  if( vkCreateCommandPool( device, &graphics_command_pool_create_info, nullptr, &graphics_command_pool ) != VK_SUCCESS ) {
    std::cerr << "グラフィックコマンドプールを作成出来ない " << std::endl;
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  uint32_t formats_count = 0u;
  if( vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, surface, &formats_count, nullptr ) != VK_SUCCESS ) {
    std::cout << "利用可能なピクセルフォーマットを取得できない" << std::endl;
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::vector< VkSurfaceFormatKHR > formats( formats_count );
  if( vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, surface, &formats_count, formats.data() ) != VK_SUCCESS ) {
    std::cout << "利用可能なピクセルフォーマットを取得できない" << std::endl;
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  if( formats.empty() ) {
    std::cerr << "利用可能なピクセルフォーマットが無い" << std::endl;
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  auto selected_format = std::find_if( formats.begin(), formats.end(), []( const auto &v ) { return v.format == VK_FORMAT_B8G8R8A8_UNORM; } );
  if( selected_format == formats.end() ) {
    selected_format = std::find_if( formats.begin(), formats.end(), []( const auto &v ) { return v.format == VK_FORMAT_R8G8B8A8_UNORM; } );
    if( selected_format == formats.end() ) {
      std::cerr << "利用可能なピクセルフォーマットが無い" << std::endl;
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
  }
  const auto &format = *selected_format;
  VkSurfaceCapabilitiesKHR surface_capabilities;
  if( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physical_device, surface, &surface_capabilities ) != VK_SUCCESS ) {
    std::cout << "ケーパビリティを取得できない" << std::endl;
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  VkExtent2D swapchain_extent;
  if ( surface_capabilities.currentExtent.width == static_cast< uint32_t >( -1 ) ) {
    swapchain_extent.width = config.width;
    swapchain_extent.height = config.height;
  } else {
    swapchain_extent = surface_capabilities.currentExtent;
    config.width = surface_capabilities.currentExtent.width;
    config.height = surface_capabilities.currentExtent.height;
  }
  const uint32_t swapchain_image_count = std::min(
    surface_capabilities.minImageCount + 1,
    surface_capabilities.maxImageCount
  );
  VkSwapchainCreateInfoKHR swapchain_create_info;
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.pNext = nullptr;
  swapchain_create_info.flags = 0;
  swapchain_create_info.surface = surface;
  swapchain_create_info.minImageCount = swapchain_image_count;
  swapchain_create_info.imageFormat = format.format;
  swapchain_create_info.imageColorSpace = format.colorSpace;
  swapchain_create_info.imageExtent = swapchain_extent;
  swapchain_create_info.imageArrayLayers = 1;
  swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchain_create_info.queueFamilyIndexCount = 0;
  swapchain_create_info.pQueueFamilyIndices = nullptr;
  swapchain_create_info.preTransform =
    ( surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) ?
    VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR :
    surface_capabilities.currentTransform;
  swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_create_info.clipped = true;
  swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain;
  if( vkCreateSwapchainKHR( device, &swapchain_create_info, nullptr, &swapchain ) != VK_SUCCESS ) {
    std::cout << "スワップチェインを作成できない" << std::endl;
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  const uint32_t max_descriptor_set_count = 20u;
  std::vector< VkDescriptorPoolSize > descriptor_pool_size( 1 );
  descriptor_pool_size[ 0 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_pool_size[ 0 ].descriptorCount = 1;
  std::vector< VkDescriptorSetLayoutBinding > descriptor_set_layout_bindings( 1 );
  descriptor_set_layout_bindings[ 0 ].binding = 0;
  descriptor_set_layout_bindings[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_set_layout_bindings[ 0 ].descriptorCount = 1;
  descriptor_set_layout_bindings[ 0 ].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  descriptor_set_layout_bindings[ 0 ].pImmutableSamplers = nullptr;
  VkDescriptorPoolCreateInfo descriptor_pool_create_info;
  descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_create_info.pNext = nullptr;
  descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  descriptor_pool_create_info.maxSets = max_descriptor_set_count;
  descriptor_pool_create_info.poolSizeCount = descriptor_pool_size.size();
  descriptor_pool_create_info.pPoolSizes = descriptor_pool_size.data();
  VkDescriptorPool descriptor_pool;
  if( vkCreateDescriptorPool( device, &descriptor_pool_create_info, nullptr, &descriptor_pool ) != VK_SUCCESS ) {
    std::cout << "デスクリプタプールを作れない" << std::endl;
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::vector< VkDescriptorSetLayout > descriptor_set_layout( swapchain_image_count );
  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
  descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptor_set_layout_create_info.pNext = nullptr;
  descriptor_set_layout_create_info.flags = 0;
  descriptor_set_layout_create_info.bindingCount = descriptor_set_layout_bindings.size();
  descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();
  for( size_t i = 0u; i != swapchain_image_count; ++i ) {
    if( vkCreateDescriptorSetLayout( device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout[ i ] ) != VK_SUCCESS ) {
      std::cout << "デスクリプタセットレイアウトを作れない" << std::endl;
      for( size_t j = 0u; j != i; ++i )
        vkDestroyDescriptorSetLayout( device, descriptor_set_layout[ j ], nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return -1;
    }
  }
  VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
  descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptor_set_allocate_info.pNext = nullptr;
  descriptor_set_allocate_info.descriptorPool = descriptor_pool;
  descriptor_set_allocate_info.descriptorSetCount = descriptor_set_layout.size();
  descriptor_set_allocate_info.pSetLayouts = descriptor_set_layout.data();
  std::vector< VkDescriptorSet > descriptor_set( descriptor_set_layout.size() );
  if( vkAllocateDescriptorSets( device, &descriptor_set_allocate_info, descriptor_set.data() ) != VK_SUCCESS ) {
    std::cout << "デスクリプタセットを確保できない" << std::endl;
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return -1;
  }
  std::vector< VkAttachmentDescription > attachments( 2 );
  attachments[ 0 ].flags = 0;
  attachments[ 0 ].format = format.format;
  attachments[ 0 ].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[ 0 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[ 0 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[ 0 ].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  attachments[ 1 ].flags = 0;
  attachments[ 1 ].format = VK_FORMAT_D16_UNORM;
  attachments[ 1 ].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[ 1 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[ 1 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[ 1 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[ 1 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  std::vector< VkAttachmentReference > color_reference( 1 );
  color_reference[ 0 ].attachment = 0;
  color_reference[ 0 ].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  VkAttachmentReference depth_reference;
  depth_reference.attachment = 1;
  depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  std::vector< VkSubpassDescription > subpass( 1 );
  subpass[ 0 ].flags = 0;
  subpass[ 0 ].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass[ 0 ].inputAttachmentCount = 0;
  subpass[ 0 ].pInputAttachments = nullptr;
  subpass[ 0 ].colorAttachmentCount = color_reference.size();
  subpass[ 0 ].pColorAttachments = color_reference.data();
  subpass[ 0 ].pResolveAttachments = nullptr;
  subpass[ 0 ].pDepthStencilAttachment = &depth_reference;
  subpass[ 0 ].preserveAttachmentCount = 0;
  subpass[ 0 ].pPreserveAttachments = nullptr;
  VkRenderPassCreateInfo render_pass_create_info;
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.pNext = nullptr;
  render_pass_create_info.flags = 0;
  render_pass_create_info.attachmentCount = attachments.size();
  render_pass_create_info.pAttachments = attachments.data();
  render_pass_create_info.subpassCount = subpass.size();
  render_pass_create_info.pSubpasses = subpass.data();
  render_pass_create_info.dependencyCount = 0;
  render_pass_create_info.pDependencies = nullptr;
  VkRenderPass render_pass;
  if( vkCreateRenderPass( device, &render_pass_create_info, nullptr, &render_pass ) != VK_SUCCESS ) {
    std::cout << "レンダーパスを作成できない" << std::endl;
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  VmaAllocatorCreateInfo allocator_info = {};
  allocator_info.physicalDevice = physical_device;
  allocator_info.device = device;
  VmaAllocator allocator;
  if( vmaCreateAllocator( &allocator_info, &allocator ) != VK_SUCCESS ) {
      std::cout << "アロケータを作成できない" << std::endl;
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
  uint32_t swapchain_image_count_ = 0;
  if( vkGetSwapchainImagesKHR( device, swapchain, &swapchain_image_count_, nullptr ) != VK_SUCCESS ) {
    std::cout << "スワップチェーンのイメージを取得できない" << std::endl;
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::vector< VkImage > color_images( swapchain_image_count_ );
  if( vkGetSwapchainImagesKHR( device, swapchain, &swapchain_image_count_, color_images.data() ) != VK_SUCCESS ) {
    std::cout << "スワップチェーンのイメージを取得できない" << std::endl;
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::vector< VkImageView > color_attachments;
  std::vector< std::shared_ptr< VkImage > > depth_images;
  std::vector< VkImageView > depth_attachments;
  std::vector< VkFramebuffer > framebuffers;
  for( auto &swapchain_image: color_images ) {
    std::vector< VkImageView > attachments_raw;
    VkImageViewCreateInfo color_image_view_create_info;
    color_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    color_image_view_create_info.pNext = nullptr;
    color_image_view_create_info.flags = 0;
    color_image_view_create_info.image = swapchain_image;
    color_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    color_image_view_create_info.format = format.format;
    color_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    color_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    color_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    color_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
    color_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_image_view_create_info.subresourceRange.baseMipLevel = 0;
    color_image_view_create_info.subresourceRange.levelCount = 1;
    color_image_view_create_info.subresourceRange.baseArrayLayer = 0;
    color_image_view_create_info.subresourceRange.layerCount = 1;
    VkImageView color_image_view;
    if( vkCreateImageView( device, &color_image_view_create_info, nullptr, &color_image_view ) != VK_SUCCESS ) {
      std::cout << "カラーイメージビューを作れない" << std::endl;
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    color_attachments.push_back( color_image_view );
    attachments_raw.push_back( color_image_view );
    VmaAllocation depth_image_allocation;
    VkImageCreateInfo depth_image_create_info;
    depth_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_create_info.pNext = nullptr;
    depth_image_create_info.flags = 0;
    depth_image_create_info.imageType = VK_IMAGE_TYPE_2D;
    depth_image_create_info.format = VK_FORMAT_D16_UNORM;
    depth_image_create_info.extent.width = swapchain_extent.width;
    depth_image_create_info.extent.height = swapchain_extent.height;
    depth_image_create_info.extent.depth = 1;
    depth_image_create_info.mipLevels = 1;
    depth_image_create_info.arrayLayers = 1;
    depth_image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depth_image_create_info.queueFamilyIndexCount = 0;
    depth_image_create_info.pQueueFamilyIndices = nullptr;
    depth_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaAllocationCreateInfo depth_image_alloc_info = {};
    depth_image_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkImage depth_image_;
    if( vmaCreateImage( allocator, &depth_image_create_info, &depth_image_alloc_info, &depth_image_, &depth_image_allocation, nullptr ) != VK_SUCCESS ) {
      std::cout << "深度イメージを作れない" << std::endl;
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    std::shared_ptr< VkImage > depth_image(
      new VkImage( depth_image_ ),
      [&allocator,depth_image_allocation]( VkImage *p ) {
        if( p ) {
	  vmaDestroyImage( allocator, *p, depth_image_allocation );
	  delete p;
	}
      }
    );
    depth_images.emplace_back( depth_image );
    VkImageViewCreateInfo depth_image_view_create_info;
    depth_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_create_info.pNext = nullptr;
    depth_image_view_create_info.flags = 0;
    depth_image_view_create_info.image = *depth_image;
    depth_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_create_info.format = VK_FORMAT_D16_UNORM;
    depth_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    depth_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    depth_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    depth_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
    depth_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_create_info.subresourceRange.baseMipLevel = 0;
    depth_image_view_create_info.subresourceRange.levelCount = 1;
    depth_image_view_create_info.subresourceRange.baseArrayLayer = 0;
    depth_image_view_create_info.subresourceRange.layerCount = 1;
    VkImageView depth_image_view;
    if( vkCreateImageView( device, &depth_image_view_create_info, nullptr, &depth_image_view ) != VK_SUCCESS ) {
      std::cout << "深度イメージビューを作れない" << std::endl;
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    depth_attachments.push_back( depth_image_view );
    attachments_raw.push_back( depth_attachments.back() );
    VkFramebufferCreateInfo framebuffer_create_info;
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = nullptr;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = render_pass;
    framebuffer_create_info.attachmentCount = attachments_raw.size();
    framebuffer_create_info.pAttachments = attachments_raw.data();
    framebuffer_create_info.width = swapchain_extent.width;
    framebuffer_create_info.height = swapchain_extent.height;
    framebuffer_create_info.layers = 1;
    VkFramebuffer framebuffer;
    if( vkCreateFramebuffer( device, &framebuffer_create_info, nullptr, &framebuffer ) != VK_SUCCESS ) {
      std::cout << "フレームバッファを作れない" << std::endl;
      for( auto &c: depth_attachments )
        vkDestroyImageView( device, c, nullptr );
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    framebuffers.push_back( framebuffer );
  }
  namespace fs = boost::filesystem;
  const auto vertex_shader_file_path = fs::path( config.shader_dir ) / fs::path( "simple.vert.spv" );
  std::fstream vertex_shader_file( vertex_shader_file_path.string(), std::ios::in|std::ios::binary );
  if( !vertex_shader_file.good() ) {
    std::cout << "頂点シェーダを読む事ができない" << std::endl;
    return 1;
  }
  const std::vector< char > vertex_shader_bin{
    std::istreambuf_iterator< char >( vertex_shader_file ),
    std::istreambuf_iterator< char >()
  };
  VkShaderModuleCreateInfo vertex_shader_module_create_info;
  vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vertex_shader_module_create_info.pNext = nullptr;
  vertex_shader_module_create_info.flags = 0;
  vertex_shader_module_create_info.codeSize = vertex_shader_bin.size();
  vertex_shader_module_create_info.pCode = reinterpret_cast< const uint32_t* >( vertex_shader_bin.data() );
  VkShaderModule vertex_shader_module;
  if( vkCreateShaderModule( device, &vertex_shader_module_create_info, nullptr, &vertex_shader_module ) != VK_SUCCESS ) {
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  const auto fragment_shader_file_path =
    fs::path( config.shader_dir ) / fs::path( "simple.frag.spv" );
  std::fstream fragment_shader_file( fragment_shader_file_path.string(), std::ios::in|std::ios::binary );
  if( !fragment_shader_file.good() ) {
    std::cout << "フラグメントシェーダを読む事ができない" << std::endl;
    return 1;
  }
  const std::vector< char > fragment_shader_bin{
    std::istreambuf_iterator< char >( fragment_shader_file ),
    std::istreambuf_iterator< char >()
  };
  VkShaderModuleCreateInfo fragment_shader_module_create_info;
  fragment_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  fragment_shader_module_create_info.pNext = nullptr;
  fragment_shader_module_create_info.flags = 0;
  fragment_shader_module_create_info.codeSize = fragment_shader_bin.size();
  fragment_shader_module_create_info.pCode = reinterpret_cast< const uint32_t* >( fragment_shader_bin.data() );
  VkShaderModule fragment_shader_module;
  if( vkCreateShaderModule( device, &fragment_shader_module_create_info, nullptr, &fragment_shader_module ) != VK_SUCCESS ) {
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::vector< VkPipelineShaderStageCreateInfo > pipeline_shader_stages( 2 );
  pipeline_shader_stages[ 0 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipeline_shader_stages[ 0 ].pNext = nullptr;
  pipeline_shader_stages[ 0 ].flags = 0;
  pipeline_shader_stages[ 0 ].stage = VK_SHADER_STAGE_VERTEX_BIT;
  pipeline_shader_stages[ 0 ].module = vertex_shader_module;
  pipeline_shader_stages[ 0 ].pName = "main";
  pipeline_shader_stages[ 0 ].pSpecializationInfo = nullptr;
  pipeline_shader_stages[ 1 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipeline_shader_stages[ 1 ].pNext = nullptr;
  pipeline_shader_stages[ 1 ].flags = 0;
  pipeline_shader_stages[ 1 ].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  pipeline_shader_stages[ 1 ].module = fragment_shader_module;
  pipeline_shader_stages[ 1 ].pName = "main";
  pipeline_shader_stages[ 1 ].pSpecializationInfo = nullptr;
  std::vector< VkPushConstantRange > push_constant_range( 1 );
  push_constant_range[ 0 ].stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
  push_constant_range[ 0 ].offset = 0;
  push_constant_range[ 0 ].size = sizeof( glm::mat4 ) * 2 + sizeof( glm::vec3 ) * 2;
  VkPipelineLayoutCreateInfo pipeline_layout_create_info;
  pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.pNext = nullptr;
  pipeline_layout_create_info.flags = 0;
  pipeline_layout_create_info.setLayoutCount = descriptor_set_layout.size();
  pipeline_layout_create_info.pSetLayouts = descriptor_set_layout.data();
  pipeline_layout_create_info.pushConstantRangeCount = push_constant_range.size();
  pipeline_layout_create_info.pPushConstantRanges = push_constant_range.data();
  VkPipelineLayout pipeline_layout;
  if( vkCreatePipelineLayout( device, &pipeline_layout_create_info, nullptr, &pipeline_layout ) != VK_SUCCESS ) {
    std::cout << "パイプラインレイアウトを作れない" << std::endl;
    vkDestroyShaderModule( device, fragment_shader_module, nullptr );
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  VkVertexInputBindingDescription vertex_input_binding;
  vertex_input_binding.binding = 0;
  vertex_input_binding.stride = sizeof( vertex_t );
  vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  std::vector< VkVertexInputAttributeDescription > vertex_input_attribute( 4 );
  vertex_input_attribute[ 0 ].location = 0;
  vertex_input_attribute[ 0 ].binding = 0;
  vertex_input_attribute[ 0 ].offset = offsetof( vertex_t, position );
  vertex_input_attribute[ 0 ].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertex_input_attribute[ 1 ].location = 1;
  vertex_input_attribute[ 1 ].binding = 0;
  vertex_input_attribute[ 1 ].offset = offsetof( vertex_t, normal );
  vertex_input_attribute[ 1 ].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertex_input_attribute[ 2 ].location = 2;
  vertex_input_attribute[ 2 ].binding = 0;
  vertex_input_attribute[ 2 ].offset = offsetof( vertex_t, tangent );
  vertex_input_attribute[ 2 ].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertex_input_attribute[ 3 ].location = 3;
  vertex_input_attribute[ 3 ].binding = 0;
  vertex_input_attribute[ 3 ].offset = offsetof( vertex_t, texcoord );
  vertex_input_attribute[ 3 ].format = VK_FORMAT_R32G32_SFLOAT;
  VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
  input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_info.pNext = nullptr;
  input_assembly_info.flags = 0;
  input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_info.primitiveRestartEnable = VK_FALSE;
  VkPipelineViewportStateCreateInfo viewport_info;
  viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_info.pNext = nullptr;
  viewport_info.flags = 0;
  viewport_info.viewportCount = 1;
  viewport_info.pViewports = nullptr;
  viewport_info.scissorCount = 1;
  viewport_info.pScissors = nullptr;
  VkPipelineRasterizationStateCreateInfo rasterization_info;
  rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization_info.pNext = nullptr;
  rasterization_info.flags = 0;
  rasterization_info.depthClampEnable = VK_FALSE;
  rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_info.cullMode = VK_CULL_MODE_NONE;
  rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization_info.depthBiasEnable = VK_FALSE;
  rasterization_info.lineWidth = 1.0f;
  VkStencilOpState stencil_op;
  stencil_op.failOp = VK_STENCIL_OP_KEEP;
  stencil_op.passOp = VK_STENCIL_OP_KEEP;
  stencil_op.depthFailOp = VK_STENCIL_OP_KEEP;
  stencil_op.compareOp = VK_COMPARE_OP_ALWAYS;
  stencil_op.compareMask = 0;
  stencil_op.writeMask = 0;
  stencil_op.reference = 0;
  VkPipelineDepthStencilStateCreateInfo depth_stencil_info;
  depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_info.pNext = nullptr;
  depth_stencil_info.flags = 0;
  depth_stencil_info.depthTestEnable = VK_TRUE;
  depth_stencil_info.depthWriteEnable = VK_TRUE;
  depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_info.front = stencil_op;
  depth_stencil_info.back = stencil_op;
  depth_stencil_info.minDepthBounds = 0.0f;
  depth_stencil_info.maxDepthBounds = 0.0f;
  std::vector< VkPipelineColorBlendAttachmentState > color_blend_attachments( 1 );
  color_blend_attachments[ 0 ].blendEnable = VK_FALSE;
  color_blend_attachments[ 0 ].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments[ 0 ].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments[ 0 ].colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachments[ 0 ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments[ 0 ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments[ 0 ].alphaBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachments[ 0 ].colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT |
    VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT;
  VkPipelineColorBlendStateCreateInfo color_blend_info;
  color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_info.pNext = nullptr;
  color_blend_info.flags = 0;
  color_blend_info.logicOpEnable = VK_FALSE;
  color_blend_info.logicOp = VK_LOGIC_OP_CLEAR;
  color_blend_info.attachmentCount = color_blend_attachments.size();
  color_blend_info.pAttachments = color_blend_attachments.data();
  color_blend_info.blendConstants[ 0 ] = 0;
  color_blend_info.blendConstants[ 1 ] = 0;
  color_blend_info.blendConstants[ 2 ] = 0;
  color_blend_info.blendConstants[ 3 ] = 0;
  const std::vector< VkDynamicState > dynamic_states{
    VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
  };
  VkPipelineDynamicStateCreateInfo dynamic_state_info;
  dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state_info.pNext = nullptr;
  dynamic_state_info.flags = 0;
  dynamic_state_info.dynamicStateCount = dynamic_states.size();
  dynamic_state_info.pDynamicStates = dynamic_states.data();
  VkPipelineVertexInputStateCreateInfo vertex_input_state;
  vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_state.pNext = nullptr;
  vertex_input_state.flags = 0;
  vertex_input_state.vertexBindingDescriptionCount = 1;
  vertex_input_state.pVertexBindingDescriptions = &vertex_input_binding;
  vertex_input_state.vertexAttributeDescriptionCount = vertex_input_attribute.size();
  vertex_input_state.pVertexAttributeDescriptions = vertex_input_attribute.data();
  VkPipelineMultisampleStateCreateInfo multisample_info;
  multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_info.pNext = nullptr;
  multisample_info.flags = 0;
  multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample_info.sampleShadingEnable = VK_FALSE;
  multisample_info.minSampleShading = 0.0f;
  multisample_info.pSampleMask = nullptr;
  multisample_info.alphaToCoverageEnable = VK_FALSE;
  multisample_info.alphaToOneEnable = VK_FALSE;
  std::vector< VkGraphicsPipelineCreateInfo > pipeline_create_info( 1 );
  pipeline_create_info[ 0 ].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_create_info[ 0 ].pNext = nullptr;
  pipeline_create_info[ 0 ].flags = 0;
  pipeline_create_info[ 0 ].stageCount = pipeline_shader_stages.size();
  pipeline_create_info[ 0 ].pStages = pipeline_shader_stages.data();
  pipeline_create_info[ 0 ].pVertexInputState = &vertex_input_state;
  pipeline_create_info[ 0 ].pInputAssemblyState = &input_assembly_info;
  pipeline_create_info[ 0 ].pTessellationState = nullptr;
  pipeline_create_info[ 0 ].pViewportState = &viewport_info;
  pipeline_create_info[ 0 ].pRasterizationState = &rasterization_info;
  pipeline_create_info[ 0 ].pMultisampleState = &multisample_info;
  pipeline_create_info[ 0 ].pDepthStencilState = &depth_stencil_info;
  pipeline_create_info[ 0 ].pColorBlendState = &color_blend_info;
  pipeline_create_info[ 0 ].pDynamicState = &dynamic_state_info;
  pipeline_create_info[ 0 ].layout = pipeline_layout;
  pipeline_create_info[ 0 ].renderPass = render_pass;
  pipeline_create_info[ 0 ].subpass = 0u;
  pipeline_create_info[ 0 ].basePipelineHandle = VK_NULL_HANDLE;
  pipeline_create_info[ 0 ].basePipelineIndex = 0;
  std::vector< VkPipeline > graphics_pipeline( pipeline_create_info.size() );
  if( vkCreateGraphicsPipelines(
    device,
    nullptr,
    pipeline_create_info.size(),
    pipeline_create_info.data(),
    nullptr,
    graphics_pipeline.data()
  ) != VK_SUCCESS ) {
    std::cout << "パイプラインを作れない" << std::endl;
    vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
    vkDestroyShaderModule( device, fragment_shader_module, nullptr );
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::vector< vertex_t > vertices = mesh.get_vertices();
  /*std::vector< vertex_t > vertices{
    vertex_t( glm::f32vec3( 0.f, 0.f, 0.f ), glm::f32vec3( 0.f, 0.f, 1.f ), glm::f32vec3( 1.f, 0.f, 0.f ), glm::f32vec2( 0.f, 0.f ) ),
    vertex_t( glm::f32vec3( 1.f, 0.f, 0.f ), glm::f32vec3( 0.f, 0.f, 1.f ), glm::f32vec3( 1.f, 0.f, 0.f ), glm::f32vec2( 1.f, 0.f ) ),
    vertex_t( glm::f32vec3( 0.f, 1.f, 0.f ), glm::f32vec3( 0.f, 0.f, 1.f ), glm::f32vec3( 1.f, 0.f, 0.f ), glm::f32vec2( 0.f, 1.f ) )
  };*/
  const uint32_t vertex_buffer_size = vertices.size() * sizeof( vertex_t );
  VkBufferCreateInfo temporary_vertex_buffer_create_info;
  temporary_vertex_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  temporary_vertex_buffer_create_info.pNext = nullptr;
  temporary_vertex_buffer_create_info.flags = 0;
  temporary_vertex_buffer_create_info.size = vertex_buffer_size;
  temporary_vertex_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  temporary_vertex_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  temporary_vertex_buffer_create_info.queueFamilyIndexCount = 0;
  temporary_vertex_buffer_create_info.pQueueFamilyIndices = nullptr;
  VmaAllocationCreateInfo temporary_vertex_buffer_alloc_info = {};
  temporary_vertex_buffer_alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  VmaAllocation temporary_vertex_buffer_allocation;
  VkBuffer temporary_vertex_buffer_;
  if( vmaCreateBuffer(
    allocator,
    &temporary_vertex_buffer_create_info,
    &temporary_vertex_buffer_alloc_info,
    &temporary_vertex_buffer_,
    &temporary_vertex_buffer_allocation,
    nullptr
  ) != VK_SUCCESS ) {
    std::cout << "一時頂点バッファを作成出来ない" << std::endl;
    for( auto &g: graphics_pipeline )
      vkDestroyPipeline( device, g, nullptr );
    vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
    vkDestroyShaderModule( device, fragment_shader_module, nullptr );
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::shared_ptr< VkBuffer > temporary_vertex_buffer(
    new VkBuffer( temporary_vertex_buffer_ ),
    [allocator,temporary_vertex_buffer_allocation]( VkBuffer *p ) {
      if( p ) {
        vmaDestroyBuffer( allocator, *p, temporary_vertex_buffer_allocation );
        delete p;
      }
    }
  );
  VkBufferCreateInfo vertex_buffer_create_info;
  vertex_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertex_buffer_create_info.pNext = nullptr;
  vertex_buffer_create_info.flags = 0;
  vertex_buffer_create_info.size = vertex_buffer_size;
  vertex_buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  vertex_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vertex_buffer_create_info.queueFamilyIndexCount = 0;
  vertex_buffer_create_info.pQueueFamilyIndices = nullptr;
  VmaAllocationCreateInfo vertex_buffer_alloc_info = {};
  vertex_buffer_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  VmaAllocation vertex_buffer_allocation;
  VkBuffer vertex_buffer_;
  if( vmaCreateBuffer(
    allocator,
    &vertex_buffer_create_info,
    &vertex_buffer_alloc_info,
    &vertex_buffer_,
    &vertex_buffer_allocation,
    nullptr
  ) != VK_SUCCESS ) {
    std::cout << "頂点バッファを作成出来ない" << std::endl;
    temporary_vertex_buffer.reset();
    for( auto &g: graphics_pipeline )
      vkDestroyPipeline( device, g, nullptr );
    vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
    vkDestroyShaderModule( device, fragment_shader_module, nullptr );
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::shared_ptr< VkBuffer > vertex_buffer(
    new VkBuffer( vertex_buffer_ ),
    [allocator,vertex_buffer_allocation]( VkBuffer *p ) {
      if( p ) {
        vmaDestroyBuffer( allocator, *p, vertex_buffer_allocation );
        delete p;
      }
    }
  );
  {
    void *mapped_memory;
    if( vmaMapMemory( allocator, temporary_vertex_buffer_allocation, &mapped_memory ) != VK_SUCCESS ) {
      std::cout << "バッファをマップできない" << std::endl;
      vertex_buffer.reset();
      temporary_vertex_buffer.reset();
      for( auto &g: graphics_pipeline )
        vkDestroyPipeline( device, g, nullptr );
      vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
      vkDestroyShaderModule( device, fragment_shader_module, nullptr );
      vkDestroyShaderModule( device, vertex_shader_module, nullptr );
      for( auto &f: framebuffers )
        vkDestroyFramebuffer( device, f, nullptr );
      for( auto &c: depth_attachments )
        vkDestroyImageView( device, c, nullptr );
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      vkDestroyRenderPass( device, render_pass, nullptr );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    std::shared_ptr< void > mapped(
      mapped_memory,
      [allocator,temporary_vertex_buffer_allocation]( void *p ) {
        if( p ) vmaUnmapMemory( allocator, temporary_vertex_buffer_allocation );
      }
    );
    memcpy( mapped.get(), vertices.data(), vertex_buffer_size );
  }
  std::vector< VkFence > fences;
  std::vector< VkSemaphore > image_acquired_semaphores;
  std::vector< VkSemaphore > draw_complete_semaphores;
  std::vector< VkSemaphore > image_ownership_semaphores;
  for( uint32_t i = 0; i != swapchain_image_count; ++i ) {
    VkFenceCreateInfo fence_create_info;
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence fence;
    if( vkCreateFence( device, &fence_create_info, nullptr, &fence ) != VK_SUCCESS ) {
      std::cout << "フェンスを作成できない" << std::endl;
      for( auto &f: fences )
        vkDestroyFence( device, f, nullptr );
      for( auto &s: image_acquired_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: draw_complete_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: image_ownership_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      vertex_buffer.reset();
      temporary_vertex_buffer.reset();
      for( auto &g: graphics_pipeline )
        vkDestroyPipeline( device, g, nullptr );
      vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
      vkDestroyShaderModule( device, fragment_shader_module, nullptr );
      vkDestroyShaderModule( device, vertex_shader_module, nullptr );
      for( auto &f: framebuffers )
        vkDestroyFramebuffer( device, f, nullptr );
      for( auto &c: depth_attachments )
        vkDestroyImageView( device, c, nullptr );
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      vkDestroyRenderPass( device, render_pass, nullptr );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    fences.push_back( fence );


    VkSemaphoreCreateInfo semaphore_create_info;
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    VkSemaphore image_acquired_semaphore;
    if( vkCreateSemaphore( device, &semaphore_create_info, nullptr, &image_acquired_semaphore ) != VK_SUCCESS ) {
      std::cout << "セマフォを作成できない" << std::endl;
      for( auto &f: fences )
        vkDestroyFence( device, f, nullptr );
      for( auto &s: image_acquired_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: draw_complete_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: image_ownership_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      vertex_buffer.reset();
      temporary_vertex_buffer.reset();
      for( auto &g: graphics_pipeline )
        vkDestroyPipeline( device, g, nullptr );
      vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
      vkDestroyShaderModule( device, fragment_shader_module, nullptr );
      vkDestroyShaderModule( device, vertex_shader_module, nullptr );
      for( auto &f: framebuffers )
        vkDestroyFramebuffer( device, f, nullptr );
      for( auto &c: depth_attachments )
        vkDestroyImageView( device, c, nullptr );
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      vkDestroyRenderPass( device, render_pass, nullptr );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    image_acquired_semaphores.push_back( image_acquired_semaphore );
    VkSemaphore draw_complete_semaphore;
    if( vkCreateSemaphore( device, &semaphore_create_info, nullptr, &draw_complete_semaphore ) != VK_SUCCESS ) {
      std::cout << "セマフォを作成できない" << std::endl;
      for( auto &f: fences )
        vkDestroyFence( device, f, nullptr );
      for( auto &s: image_acquired_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: draw_complete_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: image_ownership_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      vertex_buffer.reset();
      temporary_vertex_buffer.reset();
      for( auto &g: graphics_pipeline )
        vkDestroyPipeline( device, g, nullptr );
      vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
      vkDestroyShaderModule( device, fragment_shader_module, nullptr );
      vkDestroyShaderModule( device, vertex_shader_module, nullptr );
      for( auto &f: framebuffers )
        vkDestroyFramebuffer( device, f, nullptr );
      for( auto &c: depth_attachments )
        vkDestroyImageView( device, c, nullptr );
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      vkDestroyRenderPass( device, render_pass, nullptr );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    draw_complete_semaphores.push_back( draw_complete_semaphore );
    VkSemaphore image_ownership_semaphore;
    if( vkCreateSemaphore( device, &semaphore_create_info, nullptr, &image_ownership_semaphore ) != VK_SUCCESS ) {
      std::cout << "セマフォを作成できない" << std::endl;
      for( auto &f: fences )
        vkDestroyFence( device, f, nullptr );
      for( auto &s: image_acquired_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: draw_complete_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: image_ownership_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      vertex_buffer.reset();
      temporary_vertex_buffer.reset();
      for( auto &g: graphics_pipeline )
        vkDestroyPipeline( device, g, nullptr );
      vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
      vkDestroyShaderModule( device, fragment_shader_module, nullptr );
      vkDestroyShaderModule( device, vertex_shader_module, nullptr );
      for( auto &f: framebuffers )
        vkDestroyFramebuffer( device, f, nullptr );
      for( auto &c: depth_attachments )
        vkDestroyImageView( device, c, nullptr );
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      vkDestroyRenderPass( device, render_pass, nullptr );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    image_ownership_semaphores.push_back( image_ownership_semaphore );
  }
  VkCommandBufferAllocateInfo graphics_command_buffers_allocate_info;
  graphics_command_buffers_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  graphics_command_buffers_allocate_info.pNext = nullptr;
  graphics_command_buffers_allocate_info.commandPool = graphics_command_pool;
  graphics_command_buffers_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  graphics_command_buffers_allocate_info.commandBufferCount = swapchain_image_count * 3;
  std::vector< VkCommandBuffer > graphics_command_buffers( swapchain_image_count * 3 );
  if( vkAllocateCommandBuffers( device, &graphics_command_buffers_allocate_info, graphics_command_buffers.data() ) != VK_SUCCESS ) {
    std::cout << "描画用コマンドバッファを作れない" << std::endl;
    for( auto &f: fences )
      vkDestroyFence( device, f, nullptr );
    for( auto &s: image_acquired_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: draw_complete_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: image_ownership_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    vertex_buffer.reset();
    temporary_vertex_buffer.reset();
    for( auto &g: graphics_pipeline )
      vkDestroyPipeline( device, g, nullptr );
    vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
    vkDestroyShaderModule( device, fragment_shader_module, nullptr );
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::vector< VkBufferCopy > vertex_buffer_regions( 1 );
  vertex_buffer_regions[ 0 ].srcOffset = 0;
  vertex_buffer_regions[ 0 ].dstOffset = 0;
  vertex_buffer_regions[ 0 ].size = vertex_buffer_size;
  VkCommandBufferBeginInfo init_command_buffer_begin_info;
  init_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  init_command_buffer_begin_info.pNext = nullptr;
  init_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  init_command_buffer_begin_info.pInheritanceInfo = nullptr;
  if( vkBeginCommandBuffer( graphics_command_buffers[ 0 ], &init_command_buffer_begin_info ) != VK_SUCCESS ) {
    std::cout << "コマンドバッファを開始できない" << std::endl;
    vkFreeCommandBuffers( device, graphics_command_pool, graphics_command_buffers.size(), graphics_command_buffers.data() );
    for( auto &f: fences )
      vkDestroyFence( device, f, nullptr );
    for( auto &s: image_acquired_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: draw_complete_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: image_ownership_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    vertex_buffer.reset();
    temporary_vertex_buffer.reset();
    for( auto &g: graphics_pipeline )
      vkDestroyPipeline( device, g, nullptr );
    vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
    vkDestroyShaderModule( device, fragment_shader_module, nullptr );
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  vkCmdCopyBuffer( graphics_command_buffers[ 0 ], *temporary_vertex_buffer, *vertex_buffer, vertex_buffer_regions.size(), vertex_buffer_regions.data() );
  if( vkEndCommandBuffer( graphics_command_buffers[ 0 ] ) ) {
    std::cout << "コマンドバッファを終了できない" << std::endl;
    vkFreeCommandBuffers( device, graphics_command_pool, graphics_command_buffers.size(), graphics_command_buffers.data() );
    for( auto &f: fences )
      vkDestroyFence( device, f, nullptr );
    for( auto &s: image_acquired_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: draw_complete_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: image_ownership_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    vertex_buffer.reset();
    temporary_vertex_buffer.reset();
    for( auto &g: graphics_pipeline )
      vkDestroyPipeline( device, g, nullptr );
    vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
    vkDestroyShaderModule( device, fragment_shader_module, nullptr );
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  VkSubmitInfo init_submit_info;
  init_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  init_submit_info.pNext = nullptr;
  init_submit_info.waitSemaphoreCount = 0;
  init_submit_info.pWaitSemaphores = nullptr;
  init_submit_info.pWaitDstStageMask = nullptr;
  init_submit_info.commandBufferCount = 1;
  init_submit_info.pCommandBuffers = graphics_command_buffers.data();
  init_submit_info.signalSemaphoreCount = 0;
  init_submit_info.pSignalSemaphores = nullptr;
  if( vkQueueSubmit( graphics_queue, 1, &init_submit_info, VK_NULL_HANDLE ) != VK_SUCCESS ) {
    std::cout << "コマンドバッファをキューに流せない" << std::endl;
    vkFreeCommandBuffers( device, graphics_command_pool, graphics_command_buffers.size(), graphics_command_buffers.data() );
    for( auto &f: fences )
      vkDestroyFence( device, f, nullptr );
    for( auto &s: image_acquired_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: draw_complete_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: image_ownership_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    vertex_buffer.reset();
    temporary_vertex_buffer.reset();
    for( auto &g: graphics_pipeline )
      vkDestroyPipeline( device, g, nullptr );
    vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
    vkDestroyShaderModule( device, fragment_shader_module, nullptr );
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  if( vkQueueWaitIdle( graphics_queue ) != VK_SUCCESS ) {
    std::cout << "キューの完了を待機できない" << std::endl;
    vkFreeCommandBuffers( device, graphics_command_pool, graphics_command_buffers.size(), graphics_command_buffers.data() );
    for( auto &f: fences )
      vkDestroyFence( device, f, nullptr );
    for( auto &s: image_acquired_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: draw_complete_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    for( auto &s: image_ownership_semaphores )
      vkDestroySemaphore( device, s, nullptr );
    vertex_buffer.reset();
    temporary_vertex_buffer.reset();
    for( auto &g: graphics_pipeline )
      vkDestroyPipeline( device, g, nullptr );
    vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
    vkDestroyShaderModule( device, fragment_shader_module, nullptr );
    vkDestroyShaderModule( device, vertex_shader_module, nullptr );
    for( auto &f: framebuffers )
      vkDestroyFramebuffer( device, f, nullptr );
    for( auto &c: depth_attachments )
      vkDestroyImageView( device, c, nullptr );
    depth_images.clear();
    for( auto &c: color_attachments )
      vkDestroyImageView( device, c, nullptr );
    vmaDestroyAllocator( allocator );
    vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
    vkDestroyRenderPass( device, render_pass, nullptr );
    for( auto &d: descriptor_set_layout )
      vkDestroyDescriptorSetLayout( device, d, nullptr );
    vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
    vkDestroySwapchainKHR( device, swapchain, nullptr );
    vkDestroyCommandPool( device, graphics_command_pool, nullptr );
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  std::array< VkClearValue, 2 > clear_values;
  clear_values[ 0 ].color.float32[ 0 ] = 0.0f;
  clear_values[ 0 ].color.float32[ 1 ] = 0.0f;
  clear_values[ 0 ].color.float32[ 2 ] = 0.0f;
  clear_values[ 0 ].color.float32[ 3 ] = 1.0f;
  clear_values[ 1 ].depthStencil.depth = 1.0f;
  clear_values[ 1 ].depthStencil.stencil = 0.0f;
  for( size_t i = 0; i != swapchain_image_count; ++i ) {
    auto &command_buffer = graphics_command_buffers[ i ];
    if( vkResetCommandBuffer( command_buffer, VkCommandBufferResetFlags( 0 ) ) != VK_SUCCESS ) {
      std::cout << "コマンドバッファをリセットできない" << std::endl;
      vkFreeCommandBuffers( device, graphics_command_pool, graphics_command_buffers.size(), graphics_command_buffers.data() );
      for( auto &f: fences )
        vkDestroyFence( device, f, nullptr );
      for( auto &s: image_acquired_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: draw_complete_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: image_ownership_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      vertex_buffer.reset();
      temporary_vertex_buffer.reset();
      for( auto &g: graphics_pipeline )
        vkDestroyPipeline( device, g, nullptr );
      vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
      vkDestroyShaderModule( device, fragment_shader_module, nullptr );
      vkDestroyShaderModule( device, vertex_shader_module, nullptr );
      for( auto &f: framebuffers )
        vkDestroyFramebuffer( device, f, nullptr );
      for( auto &c: depth_attachments )
        vkDestroyImageView( device, c, nullptr );
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      vkDestroyRenderPass( device, render_pass, nullptr );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    VkCommandBufferBeginInfo command_buffer_begin_info;
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    if( vkBeginCommandBuffer( command_buffer, &command_buffer_begin_info ) != VK_SUCCESS ) {
      std::cout << "コマンドバッファを開始できない" << std::endl;
      vkFreeCommandBuffers( device, graphics_command_pool, graphics_command_buffers.size(), graphics_command_buffers.data() );
      for( auto &f: fences )
        vkDestroyFence( device, f, nullptr );
      for( auto &s: image_acquired_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: draw_complete_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: image_ownership_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      vertex_buffer.reset();
      temporary_vertex_buffer.reset();
      for( auto &g: graphics_pipeline )
        vkDestroyPipeline( device, g, nullptr );
      vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
      vkDestroyShaderModule( device, fragment_shader_module, nullptr );
      vkDestroyShaderModule( device, vertex_shader_module, nullptr );
      for( auto &f: framebuffers )
        vkDestroyFramebuffer( device, f, nullptr );
      for( auto &c: depth_attachments )
        vkDestroyImageView( device, c, nullptr );
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      vkDestroyRenderPass( device, render_pass, nullptr );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    VkRenderPassBeginInfo pass_info;
    pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_info.pNext = nullptr;
    pass_info.renderPass = render_pass;
    pass_info.framebuffer = framebuffers[ i ];
    pass_info.renderArea.offset.x = 0;
    pass_info.renderArea.offset.y = 0;
    pass_info.renderArea.extent.width = config.width;
    pass_info.renderArea.extent.height = config.height;
    pass_info.clearValueCount = clear_values.size();
    pass_info.pClearValues = clear_values.data();
    vkCmdBeginRenderPass( command_buffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline[ 0 ] );
    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = config.width;
    viewport.height = config.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    vkCmdSetViewport( command_buffer, 0, 1, &viewport );
    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = config.width;
    scissor.extent.height = config.height;
    vkCmdSetScissor( command_buffer, 0, 1, &scissor );
    std::vector< uint32_t > ds_offset{};
    vkCmdBindDescriptorSets( command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set[ i ], ds_offset.size(), ds_offset.data() );
    std::vector< VkBuffer > vertex_buffers{ *vertex_buffer };
    std::vector< VkDeviceSize > vertex_buffer_offsets{ 0 };
    vkCmdBindVertexBuffers( command_buffer, 0, vertex_buffers.size(), vertex_buffers.data(), vertex_buffer_offsets.data() );
    for( const auto &offset:mesh.get_offsets() ) {
      glm::mat4 ident{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
      vkCmdPushConstants( command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( glm::mat4 ), &mesh.get_matrices()[ offset.matrix ] );
      vkCmdPushConstants( command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, sizeof( glm::mat4 ), sizeof( glm::mat4 ), &mesh.get_cameras()[ 0 ] );
      vkCmdPushConstants( command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, sizeof( glm::mat4 ) * 2, sizeof( glm::vec3 ), &mesh.get_camera_pos()[ 0 ] );
      const glm::vec3 lightpos( 10.0, 15.0, 40.0 );
      vkCmdPushConstants( command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, sizeof( glm::mat4 ) * 2 + sizeof( glm::vec3 ), sizeof( glm::vec3 ), &lightpos );
      std::vector< VkDeviceSize > vb_offsets{ offset.vertex_begin * sizeof( vertex_t ) };
      vkCmdBindVertexBuffers( command_buffer, 0, vertex_buffers.size(), vertex_buffers.data(), vb_offsets.data() );
      vkCmdDraw( command_buffer, offset.vertex_count, 1, 0, 0 );
    }
    vkCmdEndRenderPass( command_buffer );
    if( vkEndCommandBuffer( command_buffer ) != VK_SUCCESS ) {
      std::cout << "コマンドバッファを終了できない" << std::endl;
      vkFreeCommandBuffers( device, graphics_command_pool, graphics_command_buffers.size(), graphics_command_buffers.data() );
      for( auto &f: fences )
        vkDestroyFence( device, f, nullptr );
      for( auto &s: image_acquired_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: draw_complete_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      for( auto &s: image_ownership_semaphores )
        vkDestroySemaphore( device, s, nullptr );
      vertex_buffer.reset();
      temporary_vertex_buffer.reset();
      for( auto &g: graphics_pipeline )
        vkDestroyPipeline( device, g, nullptr );
      vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
      vkDestroyShaderModule( device, fragment_shader_module, nullptr );
      vkDestroyShaderModule( device, vertex_shader_module, nullptr );
      for( auto &f: framebuffers )
        vkDestroyFramebuffer( device, f, nullptr );
      for( auto &c: depth_attachments )
        vkDestroyImageView( device, c, nullptr );
      depth_images.clear();
      for( auto &c: color_attachments )
        vkDestroyImageView( device, c, nullptr );
      vmaDestroyAllocator( allocator );
      vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
      vkDestroyRenderPass( device, render_pass, nullptr );
      for( auto &d: descriptor_set_layout )
        vkDestroyDescriptorSetLayout( device, d, nullptr );
      vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyCommandPool( device, graphics_command_pool, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
  }
  bool oops = false;
  draw_state_t draw_state;
  glfwSetWindowUserPointer( window, reinterpret_cast< void* >( &draw_state ) );
  glfwSetKeyCallback( window, common::on_key_event );
  uint32_t current_frame = 0;
  while( !draw_state.end ) {
    auto &fence = fences[ current_frame ];
    if( vkWaitForFences( device, 1, &fence, VK_TRUE, UINT64_MAX ) != VK_SUCCESS ) {
      std::cout << "フェンスを待機できない" << std::endl;
      oops = true;
      break;
    }
    if( vkResetFences( device, 1, &fence ) ) {
      std::cout << "フェンスをリセットできない" << std::endl;
      oops = true;
      break;
    }
    uint32_t image_index;
    if( vkAcquireNextImageKHR( device, swapchain, UINT64_MAX, image_acquired_semaphores[ current_frame ], VK_NULL_HANDLE, &image_index ) != VK_SUCCESS ) {
      std::cout << "スワップチェーンから描画できるイメージを取得できない" << std::endl;
      oops = true;
      break;
    }
    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_acquired_semaphores[ current_frame ];
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &graphics_command_buffers[ image_index ];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &draw_complete_semaphores[ current_frame ];
    if( vkQueueSubmit( graphics_queue, 1, &submit_info, fence ) != VK_SUCCESS ) {
      std::cout << "コマンドをキューに流せない" << std::endl;
      oops = true;
      break;
    }
    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &draw_complete_semaphores[ current_frame ];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;
    vkQueuePresentKHR( present_queue, &present_info );
    ++current_frame;
    current_frame %= swapchain_image_count;
    const auto begin_time = std::chrono::system_clock::now();
    glfwPollEvents();
    const auto end_time = std::chrono::system_clock::now();
    const auto elapsed_time = end_time - begin_time;
    if( elapsed_time < std::chrono::microseconds( 16667 ) ) {
      const auto sleep_for = std::chrono::microseconds( 16667 ) - elapsed_time;
      std::this_thread::sleep_for( sleep_for );
    }
  }
  if( vkQueueWaitIdle( graphics_queue ) != VK_SUCCESS ) {
    std::cout << "キューの完了を待機できない" << std::endl;
    oops = true;
  }
  vkFreeCommandBuffers( device, graphics_command_pool, graphics_command_buffers.size(), graphics_command_buffers.data() );
  for( auto &f: fences )
    vkDestroyFence( device, f, nullptr );
  for( auto &s: image_acquired_semaphores )
    vkDestroySemaphore( device, s, nullptr );
  for( auto &s: draw_complete_semaphores )
    vkDestroySemaphore( device, s, nullptr );
  for( auto &s: image_ownership_semaphores )
    vkDestroySemaphore( device, s, nullptr );
  vertex_buffer.reset();
  temporary_vertex_buffer.reset();
  for( auto &g: graphics_pipeline )
    vkDestroyPipeline( device, g, nullptr );
  vkDestroyPipelineLayout( device, pipeline_layout, nullptr );
  vkDestroyShaderModule( device, fragment_shader_module, nullptr );
  vkDestroyShaderModule( device, vertex_shader_module, nullptr );
  for( auto &f: framebuffers )
    vkDestroyFramebuffer( device, f, nullptr );
  for( auto &c: depth_attachments )
    vkDestroyImageView( device, c, nullptr );
  depth_images.clear();
  for( auto &c: color_attachments )
    vkDestroyImageView( device, c, nullptr );
  vmaDestroyAllocator( allocator );
  vkFreeDescriptorSets( device, descriptor_pool, descriptor_set.size(),  descriptor_set.data() );
  vkDestroyRenderPass( device, render_pass, nullptr );
  for( auto &d: descriptor_set_layout )
    vkDestroyDescriptorSetLayout( device, d, nullptr );
  vkDestroyDescriptorPool( device, descriptor_pool, nullptr );
  vkDestroySwapchainKHR( device, swapchain, nullptr );
  vkDestroyCommandPool( device, graphics_command_pool, nullptr );
  vkDestroyDevice( device, nullptr );
  vkDestroySurfaceKHR( instance, surface, nullptr );
  glfwDestroyWindow( window );
  vkDestroyInstance( instance, nullptr );
  glfwTerminate();
  return oops ? 1 : 0;  
}

