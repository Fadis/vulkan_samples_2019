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
#include <vw/shader.h>
#include <vw/pipeline.h>
#include <vw/render_pass.h>


struct push_constants_t {
  LIBSTAMP_SETTER( world_matrix )
  LIBSTAMP_SETTER( projection_matrix )
  LIBSTAMP_SETTER( camera_matrix )
  LIBSTAMP_SETTER( eye_pos )
  LIBSTAMP_SETTER( light_pos )
  glm::mat4 world_matrix;
  glm::mat4 projection_matrix;
  glm::mat4 camera_matrix;
  glm::vec4 eye_pos;
  glm::vec4 light_pos;
  float light_energy;
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
  auto context = vw::create_context(
    *instance,
    configs,
    {
      VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME
    },
    {},
    {
      vk::DescriptorPoolSize().setType( vk::DescriptorType::eUniformBuffer ).setDescriptorCount( 400 ),
      vk::DescriptorPoolSize().setType( vk::DescriptorType::eCombinedImageSampler ).setDescriptorCount( 400 )
    },
    {
      vk::DescriptorSetLayoutBinding()
        .setDescriptorType( vk::DescriptorType::eUniformBuffer )
        .setDescriptorCount( 1 )
        .setBinding( 0 )
        .setStageFlags( vk::ShaderStageFlagBits::eFragment )
        .setPImmutableSamplers( nullptr ),
      vk::DescriptorSetLayoutBinding() // base color
        .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
        .setDescriptorCount( 1 )
        .setBinding( 1 )
        .setStageFlags( vk::ShaderStageFlagBits::eFragment )
        .setPImmutableSamplers( nullptr ),
      vk::DescriptorSetLayoutBinding() // roughness metallness
        .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
        .setDescriptorCount( 1 )
        .setBinding( 2 )
        .setStageFlags( vk::ShaderStageFlagBits::eFragment )
        .setPImmutableSamplers( nullptr ),
      vk::DescriptorSetLayoutBinding() // normal
        .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
        .setDescriptorCount( 1 )
        .setBinding( 3 )
        .setStageFlags( vk::ShaderStageFlagBits::eFragment )
        .setPImmutableSamplers( nullptr ),
      vk::DescriptorSetLayoutBinding() // occlusion
        .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
        .setDescriptorCount( 1 )
        .setBinding( 4 )
        .setStageFlags( vk::ShaderStageFlagBits::eFragment )
        .setPImmutableSamplers( nullptr ),
      vk::DescriptorSetLayoutBinding() // emissive
        .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
        .setDescriptorCount( 1 )
        .setBinding( 5 )
        .setStageFlags( vk::ShaderStageFlagBits::eFragment )
        .setPImmutableSamplers( nullptr )
    }
  );
  auto render_pass = create_render_pass( context );
  auto vs = get_shader( context, std::filesystem::path( configs.shader ) / std::filesystem::path( "world.vert.spv" ) );
  auto fs = get_shader( context, std::filesystem::path( configs.shader ) / std::filesystem::path( "world.frag.spv" ) );

  std::vector< vk::VertexInputBindingDescription > vertex_input_binding{
    vk::VertexInputBindingDescription()
      .setBinding( 0 )
      .setStride( 0 )
      .setInputRate( vk::VertexInputRate::eVertex ),
    vk::VertexInputBindingDescription()
      .setBinding( 1 )
      .setStride( 12 )
      .setInputRate( vk::VertexInputRate::eVertex ),
    vk::VertexInputBindingDescription()
      .setBinding( 3 )
      .setStride( 24 )
      .setInputRate( vk::VertexInputRate::eVertex )
  };
  std::vector< vk::VertexInputAttributeDescription > vertex_input_attribute{
    vk::VertexInputAttributeDescription()
      .setLocation( 0 )
      .setBinding( 0 )
      .setFormat( vk::Format::eR32G32B32Sfloat ),
    vk::VertexInputAttributeDescription()
      .setLocation( 1 )
      .setBinding( 1 )
      .setFormat( vk::Format::eR32G32B32Sfloat ),
    vk::VertexInputAttributeDescription()
      .setLocation( 3 )
      .setBinding( 3 )
      .setFormat( vk::Format::eR32G32Sfloat )
  };
  const std::vector< vk::PushConstantRange > push_constant_range{
    vk::PushConstantRange()
      .setStageFlags( vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment )
      .setOffset( 0 )
      .setSize( sizeof( push_constants_t ) )
  };
  std::vector< vk::DescriptorSetLayout > raw_descriptor_set_layout;
  raw_descriptor_set_layout.reserve( context.descriptor_set_layout.size() );
  std::transform(
    context.descriptor_set_layout.begin(),
    context.descriptor_set_layout.end(),
    std::back_inserter( raw_descriptor_set_layout ),
    []( const auto &v ) { return *v; }
  );
  std::vector< vk::PipelineShaderStageCreateInfo > pipeline_shader_stages{
    vk::PipelineShaderStageCreateInfo()
      .setStage( vk::ShaderStageFlagBits::eVertex )
      .setModule( *vs )
      .setPName( "main" ),
    vk::PipelineShaderStageCreateInfo()
      .setStage( vk::ShaderStageFlagBits::eFragment )
      .setModule( *fs )
      .setPName( "main" )
  };
  const auto input_assembly_info = vk::PipelineInputAssemblyStateCreateInfo()
    .setTopology( vk::PrimitiveTopology::eTriangleList );
  const auto viewport_info = vk::PipelineViewportStateCreateInfo().setViewportCount( 1 ).setScissorCount( 1 );
  const auto rasterization_info = vk::PipelineRasterizationStateCreateInfo()
    .setDepthClampEnable( VK_FALSE )
    .setRasterizerDiscardEnable( VK_FALSE )
    .setPolygonMode( vk::PolygonMode::eFill )
    .setCullMode( vk::CullModeFlagBits::eBack )
    .setFrontFace( vk::FrontFace::eCounterClockwise )
    .setDepthBiasEnable( VK_FALSE )
    .setLineWidth( 1.0f );
  const auto multisample_info = vk::PipelineMultisampleStateCreateInfo();
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
      .setBlendEnable( VK_FALSE )
      .setColorWriteMask(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
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
    .setVertexBindingDescriptionCount( vertex_input_binding.size() )
    .setPVertexBindingDescriptions( vertex_input_binding.data() );

  vw::pipeline_t pipeline;
  pipeline.set_pipeline_layout( context.device->createPipelineLayoutUnique(
    vk::PipelineLayoutCreateInfo()
      .setSetLayoutCount( context.descriptor_set_layout.size() )
      .setPSetLayouts( raw_descriptor_set_layout.data() )
      .setPushConstantRangeCount( push_constant_range.size() )
      .setPPushConstantRanges( push_constant_range.data() )
  ) );
  const auto tessellation_info = 
    vk::PipelineTessellationStateCreateInfo()
      .setPatchControlPoints( 1 );
  const std::vector< vk::GraphicsPipelineCreateInfo > pipeline_create_info{
    vk::GraphicsPipelineCreateInfo()
      .setStageCount( pipeline_shader_stages.size() )
      .setPStages( pipeline_shader_stages.data() )
      .setPVertexInputState( &vertex_input_state )
      .setPInputAssemblyState( &input_assembly_info )
      .setPTessellationState( &tessellation_info )
      .setPViewportState( &viewport_info )
      .setPRasterizationState( &rasterization_info )
      .setPMultisampleState( &multisample_info )
      .setPDepthStencilState( &depth_stencil_info )
      .setPColorBlendState( &color_blend_info )
      .setPDynamicState( &dynamic_state_info )
      .setLayout( *pipeline.pipeline_layout )
      .setRenderPass( *render_pass.render_pass )
      .setSubpass( 0 )
  };

  auto raw_pipeline = context.device->createGraphicsPipelines(
    *context.pipeline_cache, pipeline_create_info
  );
  vk::ObjectDestroy< vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > deleter( *context.device, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE () );
  pipeline.set_pipeline( vk::UniqueHandle< vk::Pipeline, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE >( raw_pipeline.value[ 0 ], deleter ) );
}

