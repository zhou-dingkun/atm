#include "LowtranModel.h"
#include <cmath>

LowtranTransmittance::LowtranTransmittance() {
    // 需要查表的系数由调用方输入
}

void LowtranTransmittance::setHumidity(double hr, double f_ratio) {
    hr_ = hr;
    f_ratio_ = f_ratio;
}

double LowtranTransmittance::calculateTransmittance(double range_km, double altitude_km, double wavelength_um, double u1, double delta, double h2o_alt_scale) {
    // 水平路径吸收透过率：
    // 公式(10)：tau_H2O(λ) = exp(-u1(λ) * (f/f0) * Hr * R * exp(-0.5154 * H))
    // 公式(18)：tau_CO2(λ) = exp(-δ(λ) * R * exp(-0.313  * H))
    // 公式(21)：tau_z(λ) = tau_H2O(λ) * tau_CO2(λ)
    (void)wavelength_um;

    double h2o_factor = u1 * f_ratio_ * hr_;
    double tau_h2o = std::exp(-h2o_factor * range_km * std::exp(-0.5154 * h2o_alt_scale * altitude_km));
    double tau_co2 = std::exp(-delta * range_km * std::exp(-0.313 * altitude_km));

    return tau_h2o * tau_co2;
}

double LowtranTransmittance::calculateSlantTransmittance(double h1_km, double h2_km, double ground_range_km, double wavelength_um, double u1, double delta, double h2o_alt_scale) {
    // 倾斜路程：对路径分段并数值积分
    // 论文公式(11)、(19)给出解析形式，这里采用离散积分近似：
    // tau = exp(-∫[u1(λ)*(f/f0)*Hr*exp(-0.5154*h) + δ(λ)*exp(-0.313*h)] dr )
    int steps = 50;
    double dy = h2_km - h1_km;
    double slant_range = std::sqrt(ground_range_km * ground_range_km + dy * dy);
    double ds = slant_range / steps;

    double h2o_factor = u1 * f_ratio_ * hr_;
    double total_optical_depth = 0.0;
    for (int i = 0; i < steps; ++i) {
        double fraction = (i + 0.5) / steps;
        double current_h = h1_km + dy * fraction;

        double tau_h2o = h2o_factor * ds * std::exp(-0.5154 * h2o_alt_scale * current_h);
        double tau_co2 = delta * ds * std::exp(-0.313 * current_h);

        total_optical_depth += (tau_h2o + tau_co2);
    }

    (void)wavelength_um;
    return std::exp(-total_optical_depth);
}
