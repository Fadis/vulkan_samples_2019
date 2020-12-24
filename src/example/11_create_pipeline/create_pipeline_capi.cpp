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

  std::vector< VkVertexInputBindingDescription > vertex_input_binding;
  VkVertexInputBindingDescription position_input_binding;
  position_input_binding.binding = 0;
  position_input_binding.stride = 0;
  position_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertex_input_binding.push_back( position_input_binding );
  VkVertexInputBindingDescription normal_input_binding;
  normal_input_binding.binding = 1;
  normal_input_binding.stride = 12;
  normal_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertex_input_binding.push_back( normal_input_binding );
  VkVertexInputBindingDescription texcoord_input_binding;
  texcoord_input_binding.binding = 3;
  texcoord_input_binding.stride = 24;
  texcoord_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertex_input_binding.push_back( texcoord_input_binding );
  std::vector< VkVertexInputAttributeDescription > vertex_input_attribute;
  VkVertexInputAttributeDescription position_input_attribute;
  position_input_attribute.location = 0;
  position_input_attribute.binding = 0;
  position_input_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
  position_input_attribute.offset = 0;
  vertex_input_attribute.push_back( position_input_attribute );
  VkVertexInputAttributeDescription normal_input_attribute;
  normal_input_attribute.location = 1;
  normal_input_attribute.binding = 1;
  normal_input_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
  normal_input_attribute.offset = 0;
  vertex_input_attribute.push_back( normal_input_attribute );
  VkVertexInputAttributeDescription texcoord_input_attribute;
  texcoord_input_attribute.location = 3;
  texcoord_input_attribute.binding = 3;
  texcoord_input_attribute.format = VK_FORMAT_R32G32_SFLOAT;
  texcoord_input_attribute.offset = 0;
  vertex_input_attribute.push_back( texcoord_input_attribute );
  VkPushConstantRange push_constant_range;
  push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  push_constant_range.offset = 0;
  push_constant_range.size = sizeof( push_constants_t );
  std::vector< VkDescriptorSetLayout > raw_descriptor_set_layout;
  raw_descriptor_set_layout.reserve( context.descriptor_set_layout.size() );
  std::transform(
    context.descriptor_set_layout.begin(),
    context.descriptor_set_layout.end(),
    std::back_inserter( raw_descriptor_set_layout ),
    []( const auto &v ) { return *v; }
  );
  std::vector< VkPipelineShaderStageCreateInfo > pipeline_shader_stages;
  VkPipelineShaderStageCreateInfo vertex_shader_stage;
  vertex_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertex_shader_stage.pNext = nullptr;
  vertex_shader_stage.flags = 0;
  vertex_shader_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertex_shader_stage.module = *vs;
  vertex_shader_stage.pName = "main";
  vertex_shader_stage.pSpecializationInfo = nullptr;
  pipeline_shader_stages.push_back( vertex_shader_stage );
  VkPipelineShaderStageCreateInfo fragment_shader_stage;
  fragment_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragment_shader_stage.pNext = nullptr;
  fragment_shader_stage.flags = 0;
  fragment_shader_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragment_shader_stage.module = *fs;
  fragment_shader_stage.pName = "main";
  fragment_shader_stage.pSpecializationInfo = nullptr;
  pipeline_shader_stages.push_back( fragment_shader_stage );
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
  rasterization_info.rasterizerDiscardEnable = VK_FALSE;
  rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization_info.depthBiasEnable = VK_FALSE;
  rasterization_info.depthBiasConstantFactor = 0.0f;
  rasterization_info.depthBiasClamp = 0.0f;
  rasterization_info.depthBiasSlopeFactor = 0.0f;
  rasterization_info.lineWidth = 1.0f;
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
  depth_stencil_info.stencilTestEnable = VK_FALSE;
  depth_stencil_info.front = stencil_op;
  depth_stencil_info.back = stencil_op;
  depth_stencil_info.minDepthBounds = 0.0f;
  depth_stencil_info.maxDepthBounds = 0.0f;
  VkPipelineColorBlendAttachmentState color_blend_attachments;
  color_blend_attachments.blendEnable = VK_FALSE;
  color_blend_attachments.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments.alphaBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachments.colorWriteMask =
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
  color_blend_info.attachmentCount = 1;
  color_blend_info.pAttachments = &color_blend_attachments;
  color_blend_info.blendConstants[ 0 ] = 0.0f;
  color_blend_info.blendConstants[ 1 ] = 0.0f;
  color_blend_info.blendConstants[ 2 ] = 0.0f;
  color_blend_info.blendConstants[ 3 ] = 0.0f;
  const std::array< VkDynamicState, 2u > dynamic_states{
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
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
  vertex_input_state.vertexAttributeDescriptionCount = vertex_input_attribute.size();
  vertex_input_state.pVertexAttributeDescriptions = vertex_input_attribute.data();
  vertex_input_state.vertexBindingDescriptionCount = vertex_input_binding.size();
  vertex_input_state.pVertexBindingDescriptions = vertex_input_binding.data();
  VkPipelineLayoutCreateInfo pipeline_layout_create_info;
  pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.pNext = nullptr;
  pipeline_layout_create_info.flags = 0;
  pipeline_layout_create_info.setLayoutCount = raw_descriptor_set_layout.size();
  pipeline_layout_create_info.pSetLayouts = raw_descriptor_set_layout.data();
  pipeline_layout_create_info.pushConstantRangeCount = 1;
  pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
  VkPipelineLayout pipeline_layout;
  if( vkCreatePipelineLayout( *context.device, &pipeline_layout_create_info, nullptr, &pipeline_layout ) != VK_SUCCESS )
    return 1;
  VkPipelineTessellationStateCreateInfo tessellation_state_create_info;
  tessellation_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
  tessellation_state_create_info.pNext = nullptr;
  tessellation_state_create_info.flags = 0;
  tessellation_state_create_info.patchControlPoints = 1;
  VkGraphicsPipelineCreateInfo pipeline_create_info;
  pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_create_info.pNext = nullptr;
  pipeline_create_info.flags = 0;
  pipeline_create_info.stageCount = pipeline_shader_stages.size();
  pipeline_create_info.pStages = pipeline_shader_stages.data();
  pipeline_create_info.pVertexInputState = &vertex_input_state;
  pipeline_create_info.pInputAssemblyState = &input_assembly_info;
  pipeline_create_info.pTessellationState = &tessellation_state_create_info;
  pipeline_create_info.pViewportState = &viewport_info;
  pipeline_create_info.pRasterizationState = &rasterization_info;
  pipeline_create_info.pMultisampleState = &multisample_info;
  pipeline_create_info.pDepthStencilState = &depth_stencil_info;
  pipeline_create_info.pColorBlendState = &color_blend_info;
  pipeline_create_info.pDynamicState = &dynamic_state_info;
  pipeline_create_info.layout = pipeline_layout;
  pipeline_create_info.renderPass = *render_pass.render_pass;
  pipeline_create_info.subpass = 0;
  pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_create_info.basePipelineIndex = 0;
  VkPipeline pipeline;
  if( vkCreateGraphicsPipelines( *context.device, *context.pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline ) != VK_SUCCESS ) {
    vkDestroyPipelineLayout( *context.device, pipeline_layout, nullptr );
    return 1;
  }
  vkDestroyPipeline( *context.device, pipeline, nullptr );
  vkDestroyPipelineLayout( *context.device, pipeline_layout, nullptr );
}

