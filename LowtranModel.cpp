#include "LowtranModel.h"
#include <cmath>

LowtranTransmittance::LowtranTransmittance() {
    // 完整实现中，在此处加载光谱系数
}

namespace {
    // 论文中的参数（简化到常数/分段常数）
    // Hr: 相对湿度（论文示例中常取 46%）
    // f/f0: 温度对应的饱和水汽质量比值，未引入温度时取 1
    constexpr double DEFAULT_HR = 0.46;
    constexpr double F_RATIO = 1.0;

    // u1(lambda): 水汽单色线性吸收系数（公式(3)、(10)、(11)）
    // 论文需要查表，这里用简化分段近似
    double h2o_u1(double wavelength_um) {
        if (wavelength_um >= 3.0 && wavelength_um <= 5.0) return 0.12;
        if (wavelength_um >= 8.0 && wavelength_um <= 12.0) return 0.04;
        return 0.25;
    }

    // δ(lambda): CO2 单色线性吸收系数（公式(13)、(18)、(19)）
    // 论文需要查表，这里用简化分段近似
    double co2_delta(double wavelength_um) {
        if (wavelength_um >= 3.0 && wavelength_um <= 5.0) return 0.06;
        if (wavelength_um >= 8.0 && wavelength_um <= 12.0) return 0.02;
        return 0.10;
    }
}

double LowtranTransmittance::calculateTransmittance(double range_km, double altitude_km, double wavelength_um, double visibility_km) {
    // 水平路径吸收透过率：
    // 公式(10)：tau_H2O(λ) = exp(-u1(λ) * (f/f0) * Hr * R * exp(-0.5154 * H))
    // 公式(18)：tau_CO2(λ) = exp(-δ(λ) * R * exp(-0.313  * H))
    // 公式(21)：tau_z(λ) = tau_H2O(λ) * tau_CO2(λ)
    (void)visibility_km; // 该简化模型不使用能见度参数

    double h2o_factor = h2o_u1(wavelength_um) * F_RATIO * DEFAULT_HR;
    double co2_factor = co2_delta(wavelength_um);

    double tau_h2o = std::exp(-h2o_factor * range_km * std::exp(-0.5154 * altitude_km));
    double tau_co2 = std::exp(-co2_factor * range_km * std::exp(-0.313 * altitude_km));

    return tau_h2o * tau_co2;
}

double LowtranTransmittance::calculateMolecularAbsorption(double range_km, double altitude_km, double wavelength_um) {
    // 与 calculateTransmittance 相同，仅保留分子吸收（H2O + CO2）
    // 公式(10)、(18)、(21)
    double h2o_factor = h2o_u1(wavelength_um) * F_RATIO * DEFAULT_HR;
    double co2_factor = co2_delta(wavelength_um);

    double tau_h2o = std::exp(-h2o_factor * range_km * std::exp(-0.5154 * altitude_km));
    double tau_co2 = std::exp(-co2_factor * range_km * std::exp(-0.313 * altitude_km));
    return tau_h2o * tau_co2;
}

double LowtranTransmittance::calculateAerosolExtinction(double range_km, double altitude_km, double wavelength_um, double visibility_km) {
    // 本论文计算模型聚焦于 H2O/CO2 吸收（公式(10)、(18)、(21)）
    // 因此此接口保留但返回 1，表示忽略气溶胶消光。
    (void)range_km;
    (void)altitude_km;
    (void)wavelength_um;
    (void)visibility_km;
    return 1.0;
}

double LowtranTransmittance::calculateSlantTransmittance(double h1_km, double h2_km, double ground_range_km, double wavelength_um, double visibility_km) {
    // 倾斜路程：对路径分段并数值积分
    // 论文公式(11)、(19)给出解析形式，这里采用离散积分近似：
    // tau = exp(-∫[u1(λ)*(f/f0)*Hr*exp(-0.5154*h) + δ(λ)*exp(-0.313*h)] dr )
    int steps = 50;
    double dy = h2_km - h1_km;
    double slant_range = std::sqrt(ground_range_km * ground_range_km + dy * dy);
    double ds = slant_range / steps;

    double h2o_factor = h2o_u1(wavelength_um) * F_RATIO * DEFAULT_HR;
    double co2_factor = co2_delta(wavelength_um);

    double total_optical_depth = 0.0;
    for (int i = 0; i < steps; ++i) {
        double fraction = (i + 0.5) / steps;
        double current_h = h1_km + dy * fraction;

        double tau_h2o = h2o_factor * ds * std::exp(-0.5154 * current_h);
        double tau_co2 = co2_factor * ds * std::exp(-0.313 * current_h);

        total_optical_depth += (tau_h2o + tau_co2);
    }

    (void)visibility_km;
    return std::exp(-total_optical_depth);
}
