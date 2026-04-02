#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include <vector>
#include <cmath>

struct AtmosphereLayer {
    double baseAltitude; // km
    double baseTemp;     // K
    double basePressure; // Pa
    double lapseRate;    // K/km
};

class StandardAtmosphere1976 {
public:
    StandardAtmosphere1976();

    // 计算给定几何高度 (km) 的属性
    // 如果高度在有效范围内 (0-86km) 返回 true，否则返回 false
    bool getProperties(double altitude_km, double& temp_k, double& pressure_pa, double& density_kg_m3);

private:
    std::vector<AtmosphereLayer> layers;
    static constexpr double R = 287.05287; // 干空气的比气体常数, J/(kg·K)
    static constexpr double g0 = 9.80665;  // 标准重力, m/s^2
    static constexpr double Re = 6356.766; // 地球半径, km (如果需要用于位势高度转换，通常对于低层直接使用简单高度h)
    
    // 对于 1976 模型，我们通常使用位势高度，但对于“低层”(< 20km)，几何高度接近位势高度。
    // 我们将使用简化的几何高度概念用于各层。
};

#endif // ATMOSPHERE_H
