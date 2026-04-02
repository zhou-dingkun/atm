#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "LowtranModel.h"

struct CoefTable {
    std::vector<double> wavelength_um;
    std::vector<double> absorption_cm_inv;
};

static bool load_table_column(const std::string& path, CoefTable& table, size_t wavelength_col, size_t absorption_col) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    std::string line;
    if (!std::getline(in, line)) return false; // skip header

    std::vector<std::pair<double, double>> data;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> parts;
        while (std::getline(ss, token, ',')) {
            parts.push_back(token);
        }
        size_t required_cols = std::max(wavelength_col, absorption_col) + 1;
        if (parts.size() < required_cols) continue;
        double wavelength = std::stod(parts[wavelength_col]);
        double absorption_cm_inv = std::stod(parts[absorption_col]);
        data.emplace_back(wavelength, absorption_cm_inv);
    }

    if (data.empty()) return false;
    std::sort(data.begin(), data.end());
    table.wavelength_um.clear();
    table.absorption_cm_inv.clear();
    for (const auto& p : data) {
        table.wavelength_um.push_back(p.first);
        table.absorption_cm_inv.push_back(p.second);
    }
    return true;
}

static bool load_table(const std::string& path, CoefTable& table) {
    return load_table_column(path, table, 1, 2);
}

static double interp_coef(const CoefTable& table, double wavelength_um) {
    const auto& x = table.wavelength_um;
    const auto& y = table.absorption_cm_inv;
    if (wavelength_um <= x.front()) return y.front();
    if (wavelength_um >= x.back()) return y.back();

    auto it = std::lower_bound(x.begin(), x.end(), wavelength_um);
    size_t idx = static_cast<size_t>(it - x.begin());
    if (x[idx] == wavelength_um) return y[idx];
    size_t i0 = idx - 1;
    size_t i1 = idx;
    double t = (wavelength_um - x[i0]) / (x[i1] - x[i0]);
    return y[i0] + t * (y[i1] - y[i0]);
}

static double saturation_vapor_density_g_m3(double temp_c) {
    // 饱和水汽压（Tetens 公式，单位 hPa）
    double es_hpa = 6.112 * std::exp((17.67 * temp_c) / (temp_c + 243.5));
    // 绝对湿度（g/m^3）
    return 216.7 * es_hpa / (temp_c + 273.15);
}

static double saturation_vapor_pressure_pa(double temp_c) {
    double es_hpa = 6.112 * std::exp((17.67 * temp_c) / (temp_c + 243.5));
    return es_hpa * 100.0;
}

int main() {
    std::cout << "=== Atmospheric Transmittance (LOWTRAN-style, simplified) ===\n";
    std::cout << "Input: RH(0~1)  Temperature(C)\n";
    std::cout << "Example: 0.46 15\n\n";

    double hr = 0.46;
    double temp_c = 15.0;
    if (!(std::cin >> hr >> temp_c)) {
        return 0;
    }

    std::cout << "\nPath type (0=horizontal, 1=slant):\n\n";
    int path_type = 0;
    if (!(std::cin >> path_type)) {
        return 0;
    }

    double altitude_km = 0.2;
    double range_km = 1.0;
    double h1_km = 0.0;
    double h2_km = 1.0;
    double ground_range_km = 1.0;
    if (path_type == 0) {
        std::cout << "\nInput: Altitude_km  Range_km\n";
        std::cout << "Example: 0.2 1.0\n\n";
        if (!(std::cin >> altitude_km >> range_km)) {
            return 0;
        }
    } else {
        std::cout << "\nInput: H1_km H2_km GroundRange_km\n";
        std::cout << "Example: 0.0 1.0 0.0\n\n";
        if (!(std::cin >> h1_km >> h2_km >> ground_range_km)) {
            return 0;
        }
    }

    std::cout << "\nInput f0 (saturated water vapor density at 5C, g/m^3):\n";
    std::cout << "Example: 6.8\n\n";
    double f0 = 6.8;
    if (!(std::cin >> f0)) {
        return 0;
    }

    std::cout << "\nInput CO2 ppm (e.g., 400):\n\n";
    double co2_ppm = 400.0;
    if (!(std::cin >> co2_ppm)) {
        return 0;
    }

    std::cout << "\nScale HAPI coefficients by mixing ratio? (1=yes, 0=no):\n\n";
    int scale_by_mix = 1;
    if (!(std::cin >> scale_by_mix)) {
        return 0;
    }

    std::cout << "\nInput band: start_um end_um step_um\n";
    std::cout << "Example: 8 12 0.05\n\n";
    double start_um = 8.0;
    double end_um = 12.0;
    double step_um = 0.05;
    if (!(std::cin >> start_um >> end_um >> step_um)) {
        return 0;
    }

    double default_h2o_scale = 1.0;
    double default_co2_scale = 1.0;
    double default_h2o_alt_scale = 1.0;
    bool is_band_5_8 = (start_um >= 5.0 && end_um <= 8.0);
    if (start_um >= 3.0 && end_um <= 5.0) {
        default_h2o_scale = 2.740958;
        default_co2_scale = 7.600136;
        default_h2o_alt_scale = 0.979922;
    } else if (start_um >= 8.0 && end_um <= 12.0) {
        default_h2o_scale = 0.647254;
        default_co2_scale = 13.232524;
        default_h2o_alt_scale = 1.239969;
    }

    std::cout << "\nInput H2O coefficient scale (enter -1 for calibrated default):\n\n";
    double h2o_scale = default_h2o_scale;
    if (!(std::cin >> h2o_scale)) {
        return 0;
    }
    if (h2o_scale < 0.0) {
        h2o_scale = default_h2o_scale;
    }

    std::cout << "\nInput CO2 coefficient scale (enter -1 for calibrated default):\n\n";
    double co2_scale = default_co2_scale;
    if (!(std::cin >> co2_scale)) {
        return 0;
    }
    if (co2_scale < 0.0) {
        co2_scale = default_co2_scale;
    }

    std::cout << "\nInput H2O altitude exponent scale (enter -1 for calibrated default):\n\n";
    double h2o_alt_scale = default_h2o_alt_scale;
    if (!(std::cin >> h2o_alt_scale)) {
        return 0;
    }
    if (h2o_alt_scale < 0.0) {
        h2o_alt_scale = default_h2o_alt_scale;
    }

    std::cout << "\nInput H2O CSV path (enter '-' for default):\n";
    std::cout << "Example: c:/Users/zdk/Desktop/project/python/output/H2O_absorption_8-12um.csv\n\n";
    std::string h2o_path;
    std::cin >> h2o_path;

    std::cout << "\nInput CO2 CSV path (enter '-' for default):\n";
    std::cout << "Example: c:/Users/zdk/Desktop/project/python/output/CO2_absorption_8-12um.csv\n\n";
    std::string co2_path;
    std::cin >> co2_path;

    if (h2o_path == "-") {
        if (start_um >= 3.0 && end_um <= 5.0) {
            h2o_path = "c:/Users/zdk/Desktop/project/python/output/H2O_absorption_3-5um.csv";
        } else if (is_band_5_8) {
            h2o_path = "c:/Users/zdk/Desktop/project/python/output/hapi_data_5_8um.txt";
        } else {
            h2o_path = "c:/Users/zdk/Desktop/project/python/output/H2O_absorption_8-12um.csv";
        }
    }
    if (co2_path == "-") {
        if (start_um >= 3.0 && end_um <= 5.0) {
            co2_path = "c:/Users/zdk/Desktop/project/python/output/CO2_absorption_3-5um.csv";
        } else if (is_band_5_8) {
            co2_path = "c:/Users/zdk/Desktop/project/python/output/hapi_data_5_8um.txt";
        } else {
            co2_path = "c:/Users/zdk/Desktop/project/python/output/CO2_absorption_8-12um.csv";
        }
    }

    CoefTable h2o_table;
    CoefTable co2_table;
    if (is_band_5_8) {
        // 5-8um 使用统一的 HAPI 数据文件：
        // wavelength 在第2列(索引1), H2O 在第3列(索引2), CO2 在第4列(索引3)。
        if (!load_table_column(h2o_path, h2o_table, 1, 2)) {
            std::cout << "Failed to load H2O table (5-8um): " << h2o_path << "\n";
            return 0;
        }
        if (!load_table_column(co2_path, co2_table, 1, 3)) {
            std::cout << "Failed to load CO2 table (5-8um): " << co2_path << "\n";
            return 0;
        }
    } else {
        if (!load_table(h2o_path, h2o_table)) {
            std::cout << "Failed to load H2O table: " << h2o_path << "\n";
            return 0;
        }
        if (!load_table(co2_path, co2_table)) {
            std::cout << "Failed to load CO2 table: " << co2_path << "\n";
            return 0;
        }
    }

    double f = saturation_vapor_density_g_m3(temp_c);
    double f_ratio = (f0 > 0.0) ? (f / f0) : 1.0;

    // 参考条件：5C、1 atm
    double t_ref_k = 273.15 + 5.0;
    double e_ref_pa = saturation_vapor_pressure_pa(5.0);
    double ref_h2o_mixing_ratio = e_ref_pa / 101325.0;

    // CO2 参考体积分数
    double ref_co2_mixing_ratio = co2_ppm * 1.0e-6;

    LowtranTransmittance model;
    model.setHumidity(hr, f_ratio);

    double integral = 0.0;
    double prev_lambda = 0.0;
    double prev_tau = 0.0;
    bool first = true;

    for (double lambda = start_um; lambda <= end_um + 1e-12; lambda += step_um) {
        double alpha_h2o_cm_inv = interp_coef(h2o_table, lambda);
        double alpha_co2_cm_inv = interp_coef(co2_table, lambda);

        // 将吸收系数 (cm^-1) 依据参考混合比换算，并转换为 (km^-1)
        double u1 = alpha_h2o_cm_inv * 1.0e5 * h2o_scale;
        double delta = alpha_co2_cm_inv * 1.0e5 * co2_scale;
        if (scale_by_mix != 0) {
            u1 *= ref_h2o_mixing_ratio;
            delta *= ref_co2_mixing_ratio;
        }

        double tau = 1.0;
        if (path_type == 0) {
            tau = model.calculateTransmittance(range_km, altitude_km, lambda, u1, delta, h2o_alt_scale);
        } else {
            tau = model.calculateSlantTransmittance(h1_km, h2_km, ground_range_km, lambda, u1, delta, h2o_alt_scale);
        }

        if (!first) {
            double dl = lambda - prev_lambda;
            integral += 0.5 * (prev_tau + tau) * dl;
        } else {
            first = false;
        }

        prev_lambda = lambda;
        prev_tau = tau;
    }

    double band_width = end_um - start_um;
    double avg_tau = (band_width > 0) ? (integral / band_width) : 0.0;

    std::cout << "\nResults:\n";
    std::cout << "  f(T) (g/m^3): " << std::fixed << std::setprecision(4) << f << "\n";
    std::cout << "  f_ratio (f/f0): " << std::fixed << std::setprecision(4) << f_ratio << "\n";
    std::cout << "  Band-average transmittance: " << std::fixed << std::setprecision(6) << avg_tau << "\n";

    std::cout << "\n===== CURVE DATA =====\n";
    std::cout << "Wavelength_um,Transmittance,H2O_Alpha_cm_inv,CO2_Alpha_cm_inv\n";

    integral = 0.0;
    for (double lambda = start_um; lambda <= end_um + 1e-12; lambda += step_um) {
        double alpha_h2o_cm_inv = interp_coef(h2o_table, lambda);
        double alpha_co2_cm_inv = interp_coef(co2_table, lambda);

        double u1 = alpha_h2o_cm_inv * 1.0e5 * h2o_scale;
        double delta = alpha_co2_cm_inv * 1.0e5 * co2_scale;
        if (scale_by_mix != 0) {
            u1 *= ref_h2o_mixing_ratio;
            delta *= ref_co2_mixing_ratio;
        }

        double tau = 1.0;
        if (path_type == 0) {
            tau = model.calculateTransmittance(range_km, altitude_km, lambda, u1, delta, h2o_alt_scale);
        } else {
            tau = model.calculateSlantTransmittance(h1_km, h2_km, ground_range_km, lambda, u1, delta, h2o_alt_scale);
        }
        std::cout << std::fixed << std::setprecision(4) << lambda << "," 
                  << std::fixed << std::setprecision(6) << tau << ","
                  << std::fixed << std::scientific << std::setprecision(6) << alpha_h2o_cm_inv << ","
                  << std::fixed << std::scientific << std::setprecision(6) << alpha_co2_cm_inv << "\n";
    }
    std::cout << "===== END CURVE DATA =====\n";

    return 0;
}

