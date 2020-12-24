#include <iostream>
#include <vulkan/vulkan.hpp>
#include <vw/config.h>
#include <vw/instance.h>

int main( int argc, const char *argv[] ) {
  const auto configs = vw::parse_configs( argc, argv );
  auto instance = vw::create_instance(
    configs,
    {},
    {}
  );
  auto devices = instance->enumeratePhysicalDevices();
  unsigned int index = 0;
  for( const auto &device: devices ) {
    const auto props = device.getProperties();
    std::cout << index++ << " : " << props.deviceName << "(";
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
    if( configs.direct ) {
      auto displays = device.getDisplayPropertiesKHR();
      std::cout << "  利用可能なディスプレイ" << std::endl;
      for( const auto &d: displays ) {
        std::cout << "    " << d.displayName << std::endl;
        std::cout << "      サイズ " << d.physicalDimensions.width << "x" << d.physicalDimensions.height << std::endl;
        std::cout << "      解像度 " << d.physicalResolution.width << "x" << d.physicalResolution.height << std::endl;
        if( d.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity )
          std::cout << "      そのままの向きで表示できる" << std::endl;
        if( d.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eRotate90 )
          std::cout << "      90度回転できる" << std::endl;
        if( d.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eRotate180 )
          std::cout << "      180度回転できる" << std::endl;
        if( d.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eRotate270 )
          std::cout << "      270度回転できる" << std::endl;
        if( d.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror )
          std::cout << "      左右反転できる" << std::endl;
        if( d.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90 )
          std::cout << "      左右反転かつ90度回転できる" << std::endl;
        if( d.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180 )
          std::cout << "      左右反転かつ180度回転できる" << std::endl;
        if( d.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270 )
          std::cout << "      左右反転かつ270度回転できる" << std::endl;
        if( d.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eInherit )
          std::cout << "      システムが指定する向きで表示できる" << std::endl;
        if( d.planeReorderPossible )
          std::cout << "      planeの優先度を変更できる" << std::endl;
        if( d.persistentContent )
          std::cout << "      自力でrefreshできる" << std::endl;
        auto modes = device.getDisplayModePropertiesKHR( d.display );
        std::cout << "      表示モード" << std::endl;
        for( const auto &m: modes ) {
          std::cout << "        " << m.parameters.visibleRegion.width << "x" << m.parameters.visibleRegion.height << "@" << double( m.parameters.refreshRate )/1000.0 << "Hz" << std::endl; 
        }
        auto highest_rate = std::max_element( modes.begin(), modes.end(), []( const auto &l, const auto &r ) { return l.parameters.refreshRate < r.parameters.refreshRate; } );
        auto &mode = *highest_rate;
        auto planes = device.getDisplayPlanePropertiesKHR();
        uint32_t plane_index = 0u;
        for( uint32_t pi = 0u; pi != planes.size(); ++pi ) {
          auto planes = device.getDisplayPlaneSupportedDisplaysKHR( pi );
          if( std::find( planes.begin(), planes.end(), d.display ) != planes.end() )
            plane_index = pi;
        }
        auto surface( instance->createDisplayPlaneSurfaceKHRUnique(
          vk::DisplaySurfaceCreateInfoKHR()
            .setDisplayMode( mode.displayMode )
            .setPlaneIndex( plane_index )
            .setPlaneStackIndex( 0 )
            .setImageExtent( mode.parameters.visibleRegion )
        ) );
        auto formats = device.getSurfaceFormatsKHR( *surface );
        std::cout << "      ピクセルフォーマット" << std::endl;
        for( const auto &format: formats ) {
          std::cout << "        " << vk::to_string( format.format ) << " " << vk::to_string( format.colorSpace ) << std::endl;
        }
      }
      auto planes = device.getDisplayPlanePropertiesKHR();
      std::cout << "  利用可能なPlane" << std::endl;
      for( const auto &p: planes ) {
        std::cout << "    Plane" << std::endl;
        if( !p.currentDisplay ) 
          std::cout << "      ディスプレイに割り当てられていない" << std::endl;
        std::cout << "      深度 " << p.currentStackIndex << std::endl;
      }
    }
  }
}

