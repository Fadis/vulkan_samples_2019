#include <cstring>
#include <memory>
#include <iostream>
#include <vector>
#include <algorithm>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <common/config.h>

int main( int argc, const char *argv[] ) {
  auto config = common::parse_configs( argc, argv );
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


  uint32_t formats_count = 0u;
  if( vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, surface, &formats_count, nullptr ) != VK_SUCCESS ) {
    std::cout << "利用可能なピクセルフォーマットを取得できない" << std::endl;
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
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
  }
  if( formats.empty() ) {
    std::cerr << "利用可能なピクセルフォーマットが無い" << std::endl;
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
    vkDestroyDevice( device, nullptr );
    vkDestroySurfaceKHR( instance, surface, nullptr );
    glfwDestroyWindow( window );
    vkDestroyInstance( instance, nullptr );
    glfwTerminate();
    return 1;
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
    vkDestroySwapchainKHR( device, swapchain, nullptr );
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
    vkDestroySwapchainKHR( device, swapchain, nullptr );
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
    vkDestroySwapchainKHR( device, swapchain, nullptr );
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
    vkDestroySwapchainKHR( device, swapchain, nullptr );
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
      vkDestroySwapchainKHR( device, swapchain, nullptr );
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
      vkDestroySwapchainKHR( device, swapchain, nullptr );
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
      vkDestroySwapchainKHR( device, swapchain, nullptr );
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
      vkDestroySwapchainKHR( device, swapchain, nullptr );
      vkDestroyDevice( device, nullptr );
      vkDestroySurfaceKHR( instance, surface, nullptr );
      glfwDestroyWindow( window );
      vkDestroyInstance( instance, nullptr );
      glfwTerminate();
      return 1;
    }
    framebuffers.push_back( framebuffer );
  }
  for( auto &f: framebuffers )
    vkDestroyFramebuffer( device, f, nullptr );
  for( auto &c: depth_attachments )
    vkDestroyImageView( device, c, nullptr );
  depth_images.clear();
  for( auto &c: color_attachments )
    vkDestroyImageView( device, c, nullptr );
  vmaDestroyAllocator( allocator );
  vkDestroyRenderPass( device, render_pass, nullptr );
  vkDestroySwapchainKHR( device, swapchain, nullptr );
  vkDestroyDevice( device, nullptr );
  vkDestroySurfaceKHR( instance, surface, nullptr );
  glfwDestroyWindow( window );
  vkDestroyInstance( instance, nullptr );
  glfwTerminate();
}

