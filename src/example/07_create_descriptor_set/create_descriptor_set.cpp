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
  create_swapchain( context );

  std::vector< vk::DescriptorPoolSize > descriptor_pool_size{
    vk::DescriptorPoolSize().setType( vk::DescriptorType::eUniformBuffer ).setDescriptorCount( 400 ),
    vk::DescriptorPoolSize().setType( vk::DescriptorType::eCombinedImageSampler ).setDescriptorCount( 400 )
  };
  std::vector< vk::DescriptorSetLayoutBinding > descriptor_set_layout_bindings{
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
  };
  context.set_descriptor_pool( context.device->createDescriptorPoolUnique(
    vk::DescriptorPoolCreateInfo()
      .setPoolSizeCount( descriptor_pool_size.size() )
      .setPPoolSizes( descriptor_pool_size.data() )
      .setMaxSets( 1000 )
      .setFlags( vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet )
  ) );
  for( size_t i = 0u; i != context.swapchain_image_count; ++i ) {
    context.descriptor_set_layout.emplace_back( context.device->createDescriptorSetLayoutUnique(
      vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount( descriptor_set_layout_bindings.size() )
        .setPBindings( descriptor_set_layout_bindings.data() ),
      nullptr
    ) );
  }
  std::vector< vk::DescriptorSetLayout > raw_descriptor_set_layout;
  raw_descriptor_set_layout.reserve( context.descriptor_set_layout.size() );
  std::transform(
    context.descriptor_set_layout.begin(),
    context.descriptor_set_layout.end(),
    std::back_inserter( raw_descriptor_set_layout ),
    []( const auto &v ) { return *v; }
  );
  context.set_descriptor_set( context.device->allocateDescriptorSetsUnique(
    vk::DescriptorSetAllocateInfo()
      .setDescriptorPool( *context.descriptor_pool )
      .setDescriptorSetCount( raw_descriptor_set_layout.size() )
      .setPSetLayouts( raw_descriptor_set_layout.data() )
  ) );
}

