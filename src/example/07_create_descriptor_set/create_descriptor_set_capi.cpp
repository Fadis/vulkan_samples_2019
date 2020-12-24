#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
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

  std::vector< VkDescriptorPoolSize > descriptor_pool_size;
  VkDescriptorPoolSize uniform_buffer_descriptor_pool_size;
  uniform_buffer_descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniform_buffer_descriptor_pool_size.descriptorCount = 400;
  descriptor_pool_size.push_back( uniform_buffer_descriptor_pool_size );
  VkDescriptorPoolSize combined_image_sampler_descriptor_pool_size;
  combined_image_sampler_descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  combined_image_sampler_descriptor_pool_size.descriptorCount = 400;
  descriptor_pool_size.push_back( combined_image_sampler_descriptor_pool_size );
  std::vector< VkDescriptorSetLayoutBinding > descriptor_set_layout_bindings;
  VkDescriptorSetLayoutBinding uniform_buffer_descriptor_set_layout_binding;
  uniform_buffer_descriptor_set_layout_binding.binding = 0;
  uniform_buffer_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniform_buffer_descriptor_set_layout_binding.descriptorCount = 1;
  uniform_buffer_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  uniform_buffer_descriptor_set_layout_binding.pImmutableSamplers = nullptr;
  descriptor_set_layout_bindings.push_back( uniform_buffer_descriptor_set_layout_binding );
  VkDescriptorSetLayoutBinding base_color_descriptor_set_layout_binding;
  base_color_descriptor_set_layout_binding.binding = 1;
  base_color_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  base_color_descriptor_set_layout_binding.descriptorCount = 1;
  base_color_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  base_color_descriptor_set_layout_binding.pImmutableSamplers = nullptr;
  descriptor_set_layout_bindings.push_back( base_color_descriptor_set_layout_binding );
  VkDescriptorSetLayoutBinding roughness_metallness_descriptor_set_layout_binding;
  roughness_metallness_descriptor_set_layout_binding.binding = 2;
  roughness_metallness_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  roughness_metallness_descriptor_set_layout_binding.descriptorCount = 1;
  roughness_metallness_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  roughness_metallness_descriptor_set_layout_binding.pImmutableSamplers = nullptr;
  descriptor_set_layout_bindings.push_back( roughness_metallness_descriptor_set_layout_binding );
  VkDescriptorSetLayoutBinding normal_descriptor_set_layout_binding;
  normal_descriptor_set_layout_binding.binding = 3;
  normal_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  normal_descriptor_set_layout_binding.descriptorCount = 1;
  normal_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  normal_descriptor_set_layout_binding.pImmutableSamplers = nullptr;
  descriptor_set_layout_bindings.push_back( normal_descriptor_set_layout_binding );
  VkDescriptorSetLayoutBinding occlusion_descriptor_set_layout_binding;
  occlusion_descriptor_set_layout_binding.binding = 4;
  occlusion_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  occlusion_descriptor_set_layout_binding.descriptorCount = 1;
  occlusion_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  occlusion_descriptor_set_layout_binding.pImmutableSamplers = nullptr;
  descriptor_set_layout_bindings.push_back( occlusion_descriptor_set_layout_binding );
  VkDescriptorSetLayoutBinding emissive_descriptor_set_layout_binding;
  emissive_descriptor_set_layout_binding.binding = 5;
  emissive_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  emissive_descriptor_set_layout_binding.descriptorCount = 1;
  emissive_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  emissive_descriptor_set_layout_binding.pImmutableSamplers = nullptr;
  descriptor_set_layout_bindings.push_back( emissive_descriptor_set_layout_binding );
  VkDescriptorPoolCreateInfo descriptor_pool_create_info;
  descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_create_info.pNext = nullptr;
  descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  descriptor_pool_create_info.poolSizeCount = descriptor_pool_size.size();
  descriptor_pool_create_info.pPoolSizes = descriptor_pool_size.data();
  descriptor_pool_create_info.maxSets = 1000;
  VkDescriptorPool descriptor_pool;
  if( vkCreateDescriptorPool( *context.device, &descriptor_pool_create_info, nullptr, &descriptor_pool ) != VK_SUCCESS )
    return 1;
  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
  descriptor_set_layout_create_info.sType =  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptor_set_layout_create_info.pNext = nullptr;
  descriptor_set_layout_create_info.flags = 0;
  descriptor_set_layout_create_info.bindingCount = descriptor_set_layout_bindings.size();
  descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();
  std::vector< VkDescriptorSetLayout > descriptor_set_layouts;
  for( size_t i = 0u; i != context.swapchain_image_count; ++i ) {
    VkDescriptorSetLayout descriptor_set_layout;
    if( vkCreateDescriptorSetLayout( *context.device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout ) != VK_SUCCESS ) {
      for( auto &d: descriptor_set_layouts ) vkDestroyDescriptorSetLayout( *context.device, d, nullptr );
      vkDestroyDescriptorPool( *context.device, descriptor_pool, nullptr );
      return 1;
    }
    descriptor_set_layouts.push_back( descriptor_set_layout );
  }
  VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
  descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptor_set_allocate_info.pNext = nullptr;
  descriptor_set_allocate_info.descriptorPool = descriptor_pool;
  descriptor_set_allocate_info.descriptorSetCount = descriptor_set_layouts.size();
  descriptor_set_allocate_info.pSetLayouts = descriptor_set_layouts.data();
  std::vector< VkDescriptorSet > descriptor_set( descriptor_set_layouts.size() );
  if( vkAllocateDescriptorSets( *context.device, &descriptor_set_allocate_info, descriptor_set.data() ) != VK_SUCCESS ) {
    for( auto &d: descriptor_set_layouts ) vkDestroyDescriptorSetLayout( *context.device, d, nullptr );
    vkDestroyDescriptorPool( *context.device, descriptor_pool, nullptr );
    return 1;
  }
  if( vkFreeDescriptorSets( *context.device, descriptor_pool, descriptor_set.size(), descriptor_set.data() ) != VK_SUCCESS ) {
    for( auto &d: descriptor_set_layouts ) vkDestroyDescriptorSetLayout( *context.device, d, nullptr );
    vkDestroyDescriptorPool( *context.device, descriptor_pool, nullptr );
    return 1;
  }
  for( auto &d: descriptor_set_layouts ) vkDestroyDescriptorSetLayout( *context.device, d, nullptr );
  vkDestroyDescriptorPool( *context.device, descriptor_pool, nullptr );
}

