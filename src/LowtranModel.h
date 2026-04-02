#ifndef LOWTRAN_MODEL_H
#define LOWTRAN_MODEL_H

// 简化的接口，代表 LOWTRAN 计算器的典型工作方式
class LowtranTransmittance {
public:
    LowtranTransmittance();

    // 设置论文所需的气象参数
    // Hr: 相对湿度 (0~1)
    // f_ratio: f/f0，温度对应的饱和水汽质量比值
    void setHumidity(double hr, double f_ratio);

    // 计算水平路径透过率
    // range_km: 路径长度
    // altitude_km: 路径高度
    // wavelength_um: 波长（微米）
    // u1: 水汽单色线性吸收系数（查表）
    // delta: CO2 单色线性吸收系数（查表）
    double calculateTransmittance(double range_km, double altitude_km, double wavelength_um, double u1, double delta, double h2o_alt_scale = 1.0);

    // 计算两个高度之间斜程路径的透过率
    // h1_km: 起始高度
    // h2_km: 结束高度
    // ground_range_km: 两点间的水平距离
    // wavelength_um: 波长
    // u1: 水汽单色线性吸收系数（查表）
    // delta: CO2 单色线性吸收系数（查表）
    double calculateSlantTransmittance(double h1_km, double h2_km, double ground_range_km, double wavelength_um, double u1, double delta, double h2o_alt_scale = 1.0);

private:
    double hr_ = 0.46;
    double f_ratio_ = 1.0;
};

#endif // LOWTRAN_MODEL_H
