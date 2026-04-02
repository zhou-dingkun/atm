#ifndef LOWTRAN_MODEL_H
#define LOWTRAN_MODEL_H

#include <vector>
#include <string>

// 简化的接口，代表 LOWTRAN 计算器的典型工作方式
class LowtranTransmittance {
public:
    LowtranTransmittance();

    // 使用标准大气或自定义数据初始化
    // 这里为了简化，假设使用标准大气
    
    // 计算透过率
    // range_km: 路径长度
    // altitude_km: 路径的平均高度（简化）
    // wavelength_um: 波长（微米）
    // visibility_km: 气溶胶计算的能见度范围
    double calculateTransmittance(double range_km, double altitude_km, double wavelength_um, double visibility_km);

    // 计算两个高度之间斜程路径的透过率
    // h1_km: 起始高度
    // h2_km: 结束高度
    // ground_range_km: 两点间的水平距离
    // wavelength_um: 波长
    // visibility_km: 能见度
    double calculateSlantTransmittance(double h1_km, double h2_km, double ground_range_km, double wavelength_um, double visibility_km);

private:
    // 计算分子吸收的辅助函数（水汽、CO2、臭氧）
    // 这通常使用通过波长决定的系数 C_n
    double calculateMolecularAbsorption(double range_km, double altitude_km, double wavelength_um);

    // 计算气溶胶散射/吸收的辅助函数
    // 很大程度上取决于能见度
    double calculateAerosolExtinction(double range_km, double altitude_km, double wavelength_um, double visibility_km);
    
    // 在真正的 LOWTRAN 实现中，这些将是查找表
};

#endif // LOWTRAN_MODEL_H
