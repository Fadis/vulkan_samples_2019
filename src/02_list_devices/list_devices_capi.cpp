#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include <common/config.h>

int main( int argc, const char *argv[] ) {
  const auto config = common::parse_configs( argc, argv );
  VkApplicationInfo application_info;
  application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  application_info.pNext = nullptr;
  application_info.pApplicationName = config.prog_name.c_str();
  application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.pEngineName ="sample_engine";
  application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.apiVersion = VK_API_VERSION_1_1;
  const std::vector< const char* > ext;
  std::vector< const char* > layers;
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
    return -1;
  }
  uint32_t device_count = 0;
  if( vkEnumeratePhysicalDevices( instance, &device_count, nullptr ) != VK_SUCCESS ) {
    vkDestroyInstance(
      instance,
      nullptr
    );
    return -1;
  }
  if( device_count == 0 ) {
    std::cerr << "利用可能なデバイスがない" << std::endl;
    exit( 1 );
  }
  std::vector< VkPhysicalDevice > devices;
  devices.resize( device_count );
  if( vkEnumeratePhysicalDevices( instance, &device_count, devices.data() ) != VK_SUCCESS ) {
    vkDestroyInstance(
      instance,
      nullptr
    );
    return -1;
  }
  for( const auto &device: devices ) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties( device, &props );
    std::cout << props.deviceName << "(";
    if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER )
      std::cout << "その他のデバイス";
    else if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU )
      std::cout << "統合GPU";
    else if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
      std::cout << "ディスクリートGPU";
    else if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU )
      std::cout << "仮想GPU";
    else if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU )
      std::cout << "CPU";
    else
      std::cout << "不明なデバイス" << std::endl;
    std::cout << ")" << std::endl;
    std::cout << "  APIバージョン" << std::endl;
    std::cout << "    " << VK_VERSION_MAJOR( props.apiVersion ) << "." << VK_VERSION_MINOR( props.apiVersion ) << "." << VK_VERSION_PATCH( props.apiVersion ) << std::endl;
    std::cout << "  ドライババージョン" << std::endl;
    std::cout << "    " << VK_VERSION_MAJOR( props.driverVersion ) << "." << VK_VERSION_MINOR( props.apiVersion ) << "." << VK_VERSION_PATCH( props.apiVersion ) << std::endl;
    std::cout << "  ベンダーID" << std::endl;
    std::cout << "    " << props.vendorID << std::endl;
    std::cout << "  デバイスID" << std::endl;
    std::cout << "    " << props.deviceID << std::endl;
    uint32_t ext_count = 0u;
    if( vkEnumerateDeviceExtensionProperties( device, nullptr, &ext_count, nullptr ) != VK_SUCCESS ) {
      vkDestroyInstance(
        instance,
        nullptr
      );
      return -1;
    }
    std::vector< VkExtensionProperties > avail_dext;
    avail_dext.resize( ext_count );
    if( vkEnumerateDeviceExtensionProperties( device, nullptr, &ext_count, avail_dext.data() ) != VK_SUCCESS ) {
      vkDestroyInstance(
        instance,
        nullptr
      );
      return -1;
    }
    std::cout << "  利用可能な拡張" << std::endl;
    for( const auto &ext: avail_dext )
      std::cout << "    " << ext.extensionName << " 規格バージョン" << ext.specVersion << std::endl;
    if( avail_dext.empty() )
      std::cout << "    (なし)" << std::endl;
    uint32_t layer_count = 0u;
    if( vkEnumerateDeviceLayerProperties( device, &layer_count, nullptr ) ) {
      vkDestroyInstance(
        instance,
        nullptr
      );
      return -1;
    }
    std::vector< VkLayerProperties > avail_dlayers;
    avail_dext.resize( layer_count );
    if( vkEnumerateDeviceLayerProperties( device, &layer_count, avail_dlayers.data() ) ) {
      vkDestroyInstance(
        instance,
        nullptr
      );
      return -1;
    }
    std::cout << "  利用可能なレイヤー" << std::endl;
    for( const auto &layer: avail_dlayers ) {
      std::cout << "    " << layer.layerName << " 規格バージョン" << layer.specVersion << " 実装バージョン" << layer.implementationVersion << std::endl;
      std::cout << "      " << layer.description << std::endl;
    }
    if( avail_dlayers.empty() )
      std::cout << "    (なし)" << std::endl;
  }
  vkDestroyInstance(
    instance,
    nullptr
  );
}

