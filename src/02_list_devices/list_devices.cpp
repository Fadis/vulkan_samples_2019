#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <common/config.h>

int main( int argc, const char *argv[] ) {
  const auto config = common::parse_configs( argc, argv );
  const auto app_info = vk::ApplicationInfo(
    config.prog_name.c_str(), VK_MAKE_VERSION(1, 0, 0),
    "sample_engine", VK_MAKE_VERSION(1, 0, 0),
    VK_API_VERSION_1_1
  );
  const std::vector< const char* > ext;
  std::vector< const char* > layers;
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
    exit( 1 );
  }
  for( const auto &device: devices ) {
    const auto props = device.getProperties();
    std::cout << props.deviceName << "(";
    if( props.deviceType == vk::PhysicalDeviceType::eOther )
      std::cout << "その他のデバイス";
    else if( props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu )
      std::cout << "統合GPU";
    else if( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
      std::cout << "ディスクリートGPU";
    else if( props.deviceType == vk::PhysicalDeviceType::eVirtualGpu )
      std::cout << "仮想GPU";
    else if( props.deviceType == vk::PhysicalDeviceType::eCpu )
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
    const auto avail_dext = device.enumerateDeviceExtensionProperties();
    std::cout << "  利用可能な拡張" << std::endl;
    for( const auto &ext: avail_dext )
      std::cout << "    " << ext.extensionName << " 規格バージョン" << ext.specVersion << std::endl;
    if( avail_dext.empty() )
      std::cout << "    (なし)" << std::endl;
    const auto avail_dlayers = device.enumerateDeviceLayerProperties();
    std::cout << "  利用可能なレイヤー" << std::endl;
    for( const auto &layer: avail_dlayers ) {
      std::cout << "    " << layer.layerName << " 規格バージョン" << layer.specVersion << " 実装バージョン" << layer.implementationVersion << std::endl;
      std::cout << "      " << layer.description << std::endl;
    }
    if( avail_dlayers.empty() )
      std::cout << "    (なし)" << std::endl;
  }
}

