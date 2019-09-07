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

struct vertex_t {
  vertex_t() = default;
  vertex_t(
    glm::f32vec3 position_,
    glm::f32vec3 normal_,
    glm::f32vec3 tangent_,
    glm::f32vec2 texcoord_
  ) :
    position( position_ ),
    normal( normal_ ),
    tangent( tangent_ ),
    texcoord( texcoord_ ) {}
  glm::f32vec3 position;
  glm::f32vec3 normal;
  glm::f32vec3 tangent;
  glm::f32vec2 texcoord;
};

int main( int argc, const char *argv[] ) {
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
  const auto formats = physical_device.getSurfaceFormatsKHR( *surface );
  if( formats.empty() ) {
    std::cerr << "利用可能なピクセルフォーマットが無い" << std::endl;
    return 1;
  }
  auto selected_format = std::find_if( formats.begin(), formats.end(), []( const auto &v ) { return v.format == vk::Format::eB8G8R8A8Unorm; } );
  if( selected_format == formats.end() ) {
    selected_format = std::find_if( formats.begin(), formats.end(), []( const auto &v ) { return v.format == vk::Format::eR8G8B8A8Unorm; } );
    if( selected_format == formats.end() ) {
      std::cerr << "利用可能なピクセルフォーマットが無い" << std::endl;
      return 1;
    }
  }
  const auto &format = *selected_format;
  const auto surface_capabilities = physical_device.getSurfaceCapabilitiesKHR( *surface );
  vk::Extent2D swapchain_extent;
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
  auto swapchain = device->createSwapchainKHRUnique(
    vk::SwapchainCreateInfoKHR()
      .setSurface( *surface )
      .setMinImageCount( swapchain_image_count )
      .setImageFormat( format.format )
      .setImageColorSpace( format.colorSpace )
      .setImageExtent( { swapchain_extent.width, swapchain_extent.height } )
      .setImageArrayLayers( 1 )
      .setImageUsage( vk::ImageUsageFlagBits::eColorAttachment )
      .setImageSharingMode( vk::SharingMode::eExclusive )
      .setQueueFamilyIndexCount( 0 )
      .setPreTransform(
        ( surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ) ?
        vk::SurfaceTransformFlagBitsKHR::eIdentity :
        surface_capabilities.currentTransform
      )
      .setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
      .setPresentMode( vk::PresentModeKHR::eFifo )
      .setClipped( true )
  );
  const std::vector< vk::DescriptorSetLayoutBinding > descriptor_set_layout_bindings{
    vk::DescriptorSetLayoutBinding()
      .setDescriptorType( vk::DescriptorType::eUniformBuffer )
      .setDescriptorCount( 1 )
      .setBinding( 0 )
      .setStageFlags( vk::ShaderStageFlagBits::eVertex )
      .setPImmutableSamplers( nullptr )
  };
  std::vector< vk::DescriptorSetLayout > descriptor_set_layout;
  BOOST_SCOPE_EXIT( &descriptor_set_layout, &device ) {
    for( auto &d: descriptor_set_layout ) device->destroyDescriptorSetLayout( d );
  } BOOST_SCOPE_EXIT_END
  for( size_t i = 0u; i != swapchain_image_count; ++i ) {
    descriptor_set_layout.emplace_back( device->createDescriptorSetLayout(
      vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount( descriptor_set_layout_bindings.size() )
        .setPBindings( descriptor_set_layout_bindings.data() ),
      nullptr
    ) );
  }
  const std::vector< vk::AttachmentDescription > attachments{
    vk::AttachmentDescription()
      .setFormat( format.format )
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
    vk::AttachmentReference()
      .setAttachment( 0 )
      .setLayout( vk::ImageLayout::eColorAttachmentOptimal )
  };
  const auto depth_reference =
    vk::AttachmentReference()
      .setAttachment( 1 )
      .setLayout( vk::ImageLayout::eDepthStencilAttachmentOptimal );
  const std::vector< vk::SubpassDescription > subpass{
    vk::SubpassDescription()
      .setPipelineBindPoint( vk::PipelineBindPoint::eGraphics )
      .setColorAttachmentCount( color_reference.size() )
      .setPColorAttachments( color_reference.data() )
      .setPDepthStencilAttachment( &depth_reference )
  };
  auto render_pass = device->createRenderPassUnique(
    vk::RenderPassCreateInfo()
      .setAttachmentCount( attachments.size() )
      .setPAttachments( attachments.data() )
      .setSubpassCount( subpass.size() )
      .setPSubpasses( subpass.data() )
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
  std::vector< vk::UniqueHandle< vk::ImageView, vk::DispatchLoaderStatic > > color_attachments;
  std::vector< std::shared_ptr< vk::Image > > depth_images;
  std::vector< vk::UniqueHandle< vk::ImageView, vk::DispatchLoaderStatic > > depth_attachments;
  std::vector< vk::UniqueHandle< vk::Framebuffer, vk::DispatchLoaderStatic > > framebuffers;
  for( auto &swapchain_image: device->getSwapchainImagesKHR( *swapchain ) ) {
    std::vector< vk::ImageView > attachments_raw;
    color_attachments.emplace_back( device->createImageViewUnique(
      vk::ImageViewCreateInfo()
        .setImage( swapchain_image )
        .setViewType( vk::ImageViewType::e2D )
        .setFormat( format.format )
        .setSubresourceRange( vk::ImageSubresourceRange( vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 ) )
    ) );
    attachments_raw.push_back( *color_attachments.back() );
    VmaAllocation depth_image_allocation;
    VkImageCreateInfo depth_image_create_info =
      vk::ImageCreateInfo()
        .setFormat( vk::Format::eD16Unorm )
        .setMipLevels( 1 )
        .setArrayLayers( 1 )
        .setUsage( vk::ImageUsageFlagBits::eDepthStencilAttachment )
        .setInitialLayout( vk::ImageLayout::eUndefined )
        .setImageType( vk::ImageType::e2D )
	.setExtent( { swapchain_extent.width, swapchain_extent.height, 1 } )
	.setMipLevels( 1 )
	.setArrayLayers( 1 );
    VmaAllocationCreateInfo depth_image_alloc_info = {};
    depth_image_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkImage depth_image_;
    {
      const auto result = vmaCreateImage( *allocator, &depth_image_create_info, &depth_image_alloc_info, &depth_image_, &depth_image_allocation, nullptr );
      if( result != VK_SUCCESS ) vk::throwResultException( vk::Result( result ), "イメージを作成できない" );
    }
    std::shared_ptr< vk::Image > depth_image(
      new vk::Image( depth_image_ ),
      [allocator,depth_image_allocation]( vk::Image *p ) {
        if( p ) {
	  vmaDestroyImage( *allocator, *p, depth_image_allocation );
	  delete p;
	}
      }
    );
    depth_images.emplace_back( depth_image );
    depth_attachments.emplace_back( device->createImageViewUnique(
      vk::ImageViewCreateInfo()
        .setImage( *depth_image )
        .setViewType( vk::ImageViewType::e2D )
        .setFormat( vk::Format::eD16Unorm )
        .setSubresourceRange( vk::ImageSubresourceRange( vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 ) )
    ) );
    attachments_raw.push_back( *depth_attachments.back() );
    framebuffers.push_back(
      device->createFramebufferUnique(
        vk::FramebufferCreateInfo()
          .setRenderPass( *render_pass )
          .setAttachmentCount( attachments_raw.size() )
          .setPAttachments( attachments_raw.data() )
          .setWidth( swapchain_extent.width )
          .setHeight( swapchain_extent.height )
          .setLayers( 1 )
      )
    );
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
  auto vertex_shader_module = device->createShaderModuleUnique(
    vk::ShaderModuleCreateInfo()
      .setCodeSize( vertex_shader_bin.size() )
      .setPCode( reinterpret_cast< const uint32_t* >( vertex_shader_bin.data() ) )
  );
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
  auto fragment_shader_module = device->createShaderModuleUnique(
    vk::ShaderModuleCreateInfo()
      .setCodeSize( fragment_shader_bin.size() )
      .setPCode( reinterpret_cast< const uint32_t* >( fragment_shader_bin.data() ) )
  );
  std::vector< vk::PipelineShaderStageCreateInfo > pipeline_shader_stages{
    vk::PipelineShaderStageCreateInfo()
      .setStage( vk::ShaderStageFlagBits::eVertex )
      .setModule( *vertex_shader_module )
      .setPName( "main" ),
    vk::PipelineShaderStageCreateInfo()
      .setStage( vk::ShaderStageFlagBits::eFragment )
      .setModule( *fragment_shader_module )
      .setPName( "main" )
  };
  const std::vector< vk::PushConstantRange > push_constant_range{
    vk::PushConstantRange()
      .setStageFlags( vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment )
      .setOffset( 0 )
      .setSize( sizeof( glm::mat4 ) * 2 + sizeof( glm::vec3 ) * 2 )
  };
  auto pipeline_layout = device->createPipelineLayoutUnique(
    vk::PipelineLayoutCreateInfo()
      .setSetLayoutCount( descriptor_set_layout.size() )
      .setPSetLayouts( descriptor_set_layout.data() )
      .setPushConstantRangeCount( push_constant_range.size() )
      .setPPushConstantRanges( push_constant_range.data() )
  );
  const auto vertex_input_binding = vk::VertexInputBindingDescription()
    .setBinding( 0 ).setStride( sizeof( vertex_t ) ).setInputRate( vk::VertexInputRate::eVertex );
  const std::vector< vk::VertexInputAttributeDescription > vertex_input_attribute{
    vk::VertexInputAttributeDescription()
      .setLocation( 0 ).setBinding( 0 ).setFormat( vk::Format::eR32G32B32Sfloat ).setOffset( offsetof( vertex_t, position ) ),
    vk::VertexInputAttributeDescription()
      .setLocation( 1 ).setBinding( 0 ).setFormat( vk::Format::eR32G32B32Sfloat ).setOffset( offsetof( vertex_t, normal ) ),
    vk::VertexInputAttributeDescription()
      .setLocation( 2 ).setBinding( 0 ).setFormat( vk::Format::eR32G32B32Sfloat ).setOffset( offsetof( vertex_t, tangent ) ),
    vk::VertexInputAttributeDescription()
      .setLocation( 3 ).setBinding( 0 ).setFormat( vk::Format::eR32G32B32Sfloat ).setOffset( offsetof( vertex_t, texcoord ) )
  };
  const auto input_assembly_info = vk::PipelineInputAssemblyStateCreateInfo()                                          
    .setTopology( vk::PrimitiveTopology::eTriangleList );
  const auto viewport_info = vk::PipelineViewportStateCreateInfo().setViewportCount( 1 ).setScissorCount( 1 );         
  const auto rasterization_info = vk::PipelineRasterizationStateCreateInfo()                                           
    .setDepthClampEnable( VK_FALSE )
    .setRasterizerDiscardEnable( VK_FALSE )
    .setPolygonMode( vk::PolygonMode::eFill )
    .setCullMode( vk::CullModeFlagBits::eNone )
    .setFrontFace( vk::FrontFace::eCounterClockwise )
    .setDepthBiasEnable( VK_FALSE )
    .setLineWidth( 1.0f );
  const auto stencil_op = vk::StencilOpState()
    .setFailOp( vk::StencilOp::eKeep )
    .setPassOp( vk::StencilOp::eKeep )
    .setCompareOp( vk::CompareOp::eAlways );
  const auto depth_stencil_info = vk::PipelineDepthStencilStateCreateInfo()                                            
    .setDepthTestEnable( VK_TRUE )
    .setDepthWriteEnable( VK_TRUE )
    .setDepthCompareOp( vk::CompareOp::eLessOrEqual )
    .setDepthBoundsTestEnable( VK_FALSE )
    .setStencilTestEnable( VK_FALSE )
    .setFront( stencil_op )
    .setBack( stencil_op );
  const std::array< vk::PipelineColorBlendAttachmentState, 1u > color_blend_attachments{                               
    vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(
        vk::ColorComponentFlagBits::eR |
	vk::ColorComponentFlagBits::eG |
	vk::ColorComponentFlagBits::eB |             
        vk::ColorComponentFlagBits::eA
      )
  };
  const auto color_blend_info =
    vk::PipelineColorBlendStateCreateInfo()
      .setAttachmentCount( color_blend_attachments.size() )
      .setPAttachments( color_blend_attachments.data() );
  const std::array< vk::DynamicState, 2u > dynamic_states{
    vk::DynamicState::eViewport, vk::DynamicState::eScissor
  };
  const auto dynamic_state_info =
    vk::PipelineDynamicStateCreateInfo()
      .setDynamicStateCount( dynamic_states.size() )
     .setPDynamicStates( dynamic_states.data() );
  const vk::PipelineVertexInputStateCreateInfo vertex_input_state = vk::PipelineVertexInputStateCreateInfo()           
    .setVertexAttributeDescriptionCount( vertex_input_attribute.size() )                                               
    .setPVertexAttributeDescriptions( vertex_input_attribute.data() )                                                  
    .setVertexBindingDescriptionCount( 1 )
    .setPVertexBindingDescriptions( &vertex_input_binding );
  const auto multisample_info = vk::PipelineMultisampleStateCreateInfo();
  const std::vector< vk::GraphicsPipelineCreateInfo > pipeline_create_info{                                            
    vk::GraphicsPipelineCreateInfo()
      .setStageCount( pipeline_shader_stages.size() )
      .setPStages( pipeline_shader_stages.data() )
      .setPVertexInputState( &vertex_input_state )
      .setPInputAssemblyState( &input_assembly_info )
      .setPViewportState( &viewport_info )
      .setPRasterizationState( &rasterization_info )
      .setPMultisampleState( &multisample_info )
      .setPDepthStencilState( &depth_stencil_info )
      .setPColorBlendState( &color_blend_info )
      .setPDynamicState( &dynamic_state_info )
      .setLayout( *pipeline_layout )
      .setRenderPass( *render_pass )
      .setSubpass( 0 )
  };
  auto graphics_pipeline = device->createGraphicsPipelinesUnique(
    nullptr, pipeline_create_info
  );
}

