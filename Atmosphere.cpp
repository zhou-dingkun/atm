#include "Atmosphere.h"
#include <cmath>
#include <iostream>

StandardAtmosphere1976::StandardAtmosphere1976() {
    // 定义 1976 美国标准大气的各层（高达 86km）
    // H (km), T (K), P (Pa), 直减率 (K/km)
    
    // 第 0 层: 对流层
    layers.push_back({0.0, 288.15, 101325.0, -6.5});
    // 第 1 层: 对流层顶
    layers.push_back({11.0, 216.65, 22632.1, 0.0}); 
    // 第 2 层: 平流层 1
    layers.push_back({20.0, 216.65, 5474.89, 1.0});
    // 第 3 层: 平流层 2
    layers.push_back({32.0, 228.65, 868.019, 2.8});
    // 第 4 层: 平流层顶
    layers.push_back({47.0, 270.65, 110.906, 0.0});
    // 第 5 层: 中间层 1
    layers.push_back({51.0, 270.65, 66.9389, -2.8});
    // 第 6 层: 中间层 2
    layers.push_back({71.0, 214.65, 3.95642, -2.0});
    // 第 7 层: 中间层顶 (部分)
    layers.push_back({84.852, 186.95, 0.3734, 0.0}); 
}

bool StandardAtmosphere1976::getProperties(double altitude_km, double& temp_k, double& pressure_pa, double& density_kg_m3) {
    if (altitude_km < 0 || altitude_km > 86.0) {
        return false;
    }

    // Find the correct layer
    size_t currentInfoIndex = 0;
    for (size_t i = 0; i < layers.size() - 1; ++i) {
        if (altitude_km >= layers[i].baseAltitude && altitude_km < layers[i+1].baseAltitude) {
            currentInfoIndex = i;
            break;
        }
    }
    // Handle the last layer case
    if (altitude_km >= layers.back().baseAltitude) {
        currentInfoIndex = layers.size() - 1;
    }

    const auto& layer = layers[currentInfoIndex];
    
    double dh = altitude_km - layer.baseAltitude; // dh: 距离当前层底部的垂直高度差 (km)
    
    // 温度计算
    if (std::abs(layer.lapseRate) < 1e-9) {
        // 等温层 ( lapseRate 约为 0 )
        temp_k = layer.baseTemp;
        // 等温层的压力公式：P = P0 * exp(-g0/(R*T) * dh * 1000)
        // dh 单位为 km，转换为 m 需要 * 1000.0
        // R: 气体常数, g0: 重力加速度, temp_k: 层温度
        pressure_pa = layer.basePressure * std::exp((-g0 * dh * 1000.0) / (R * temp_k));
    } else {
        // 梯度层 ( lapseRate 不为 0 )
        // temp_k = T0 + L * dh
        temp_k = layer.baseTemp + layer.lapseRate * dh;
        
        // 压力公式：P = P0 * (T/T0) ^ (-g0 / (R * lapse_rate_SI))
        // lapse_si: 直减率转换为标准单位 (K/m), 原单位为 K/km, 所以除以 1000
        double lapse_si = layer.lapseRate / 1000.0;
        
        // exponent: 压力计算指数项 -g0 / (R * L)
        double exponent = -g0 / (R * lapse_si);
        
        // 计算压力
        pressure_pa = layer.basePressure * std::pow(temp_k / layer.baseTemp, exponent);
    }

    // 根据理想气体定律计算密度：rho = P / (R * T)
    // density_kg_m3: 空气密度 (kg/m^3)
    density_kg_m3 = pressure_pa / (R * temp_k);

    return true;
}
