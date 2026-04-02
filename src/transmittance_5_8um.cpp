#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Atmosphere.h"
#include "LowtranModel.h"

struct MultiCoefTable {
    std::vector<double> wavelength_um;
    std::vector<double> alpha_h2o_cm_inv;
    std::vector<double> alpha_co2_cm_inv;
    std::vector<double> alpha_o3_cm_inv;
    std::vector<double> alpha_n2o_cm_inv;
    std::vector<double> alpha_co_cm_inv;
    std::vector<double> alpha_ch4_cm_inv;
    std::vector<double> alpha_total_cm_inv;
};

struct GasProfileRef {
    double h2o_ref = 0.010000;
    double co2_ref = 420.0e-6;
    double o3_ref = 8.0e-8;
    double n2o_ref = 330.0e-9;
    double co_ref = 120.0e-9;
    double ch4_ref = 1.90e-6;
};

static std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        parts.push_back(token);
    }
    return parts;
}

static bool load_multi_table(const std::string& path, MultiCoefTable& table) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    std::string line;
    if (!std::getline(in, line)) return false; // skip header

    std::vector<std::pair<double, size_t>> order;
    std::vector<double> wavelength_um;
    std::vector<double> h2o;
    std::vector<double> co2;
    std::vector<double> o3;
    std::vector<double> n2o;
    std::vector<double> co;
    std::vector<double> ch4;
    std::vector<double> total;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto parts = split_csv(line);
        if (parts.size() < 9) continue;

        double wavelength = std::stod(parts[1]);
        wavelength_um.push_back(wavelength);
        h2o.push_back(std::stod(parts[2]));
        co2.push_back(std::stod(parts[3]));
        o3.push_back(std::stod(parts[4]));
        n2o.push_back(std::stod(parts[5]));
        co.push_back(std::stod(parts[6]));
        ch4.push_back(std::stod(parts[7]));
        total.push_back(std::stod(parts[8]));

        order.emplace_back(wavelength, order.size());
    }

    if (order.empty()) return false;
    std::sort(order.begin(), order.end());

    table.wavelength_um.clear();
    table.alpha_h2o_cm_inv.clear();
    table.alpha_co2_cm_inv.clear();
    table.alpha_o3_cm_inv.clear();
    table.alpha_n2o_cm_inv.clear();
    table.alpha_co_cm_inv.clear();
    table.alpha_ch4_cm_inv.clear();
    table.alpha_total_cm_inv.clear();

    for (const auto& item : order) {
        size_t i = item.second;
        table.wavelength_um.push_back(wavelength_um[i]);
        table.alpha_h2o_cm_inv.push_back(h2o[i]);
        table.alpha_co2_cm_inv.push_back(co2[i]);
        table.alpha_o3_cm_inv.push_back(o3[i]);
        table.alpha_n2o_cm_inv.push_back(n2o[i]);
        table.alpha_co_cm_inv.push_back(co[i]);
        table.alpha_ch4_cm_inv.push_back(ch4[i]);
        table.alpha_total_cm_inv.push_back(total[i]);
    }
    return true;
}

static double interp_vector(const std::vector<double>& x, const std::vector<double>& y, double wavelength_um) {
    if (x.empty()) return 0.0;
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
    double es_hpa = 6.112 * std::exp((17.67 * temp_c) / (temp_c + 243.5));
    return 216.7 * es_hpa / (temp_c + 273.15);
}

static double profile_h2o(double altitude_km, double hr) {
    double x0 = std::max(0.0, std::min(0.03, 0.018 * hr));
    return x0 * std::exp(-altitude_km / 2.0);
}

static double profile_co2(double altitude_km, double co2_ppm) {
    double x0 = std::max(200.0, co2_ppm) * 1.0e-6;
    return x0 * std::exp(-altitude_km / 70.0);
}

static double profile_o3(double altitude_km) {
    double peak = std::exp(-((altitude_km - 25.0) * (altitude_km - 25.0)) / (2.0 * 9.0 * 9.0));
    return 8.0e-8 * (1.0 + 60.0 * peak);
}

static double profile_n2o(double altitude_km) {
    return 330.0e-9 * std::exp(-altitude_km / 25.0);
}

static double profile_co(double altitude_km) {
    return 120.0e-9 * std::exp(-altitude_km / 15.0);
}

static double profile_ch4(double altitude_km) {
    return 1.90e-6 * std::exp(-altitude_km / 30.0);
}

static double extra_optical_depth_segment(
    const MultiCoefTable& table,
    const GasProfileRef& ref,
    double wavelength_um,
    double altitude_km,
    double ds_km,
    StandardAtmosphere1976& atm
) {
    double temp_k = 288.15;
    double pressure_pa = 101325.0;
    double density = 1.225;
    if (!atm.getProperties(altitude_km, temp_k, pressure_pa, density)) {
        return 0.0;
    }
    (void)density;

    const double p0 = 101325.0;
    const double t0 = 296.0;
    double thermo_scale = (pressure_pa / p0) * (t0 / std::max(150.0, temp_k));

    double alpha_o3 = interp_vector(table.wavelength_um, table.alpha_o3_cm_inv, wavelength_um)
                    * thermo_scale * (profile_o3(altitude_km) / ref.o3_ref);
    double alpha_n2o = interp_vector(table.wavelength_um, table.alpha_n2o_cm_inv, wavelength_um)
                     * thermo_scale * (profile_n2o(altitude_km) / ref.n2o_ref);
    double alpha_co = interp_vector(table.wavelength_um, table.alpha_co_cm_inv, wavelength_um)
                    * thermo_scale * (profile_co(altitude_km) / ref.co_ref);
    double alpha_ch4 = interp_vector(table.wavelength_um, table.alpha_ch4_cm_inv, wavelength_um)
                     * thermo_scale * (profile_ch4(altitude_km) / ref.ch4_ref);

    double alpha_extra_cm_inv = std::max(0.0, alpha_o3 + alpha_n2o + alpha_co + alpha_ch4);
    return alpha_extra_cm_inv * 1.0e5 * ds_km;
}

int main() {
    std::cout << "=== Atmospheric Transmittance (5-8um, LOWTRAN-style) ===\n";
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

    std::cout << "\nInput CO2 ppm (e.g., 420):\n\n";
    double co2_ppm = 420.0;
    if (!(std::cin >> co2_ppm)) {
        return 0;
    }

    std::cout << "\nInput wavelength step for fixed 5-8 um band (e.g., 0.05):\n\n";
    double step_um = 0.05;
    if (!(std::cin >> step_um)) {
        return 0;
    }
    if (step_um <= 0.0) {
        std::cout << "Invalid step_um.\n";
        return 0;
    }

    std::cout << "\nInput H2O coefficient scale:\n\n";
    double h2o_scale = 1.0;
    if (!(std::cin >> h2o_scale)) {
        return 0;
    }

    std::cout << "\nInput CO2 coefficient scale:\n\n";
    double co2_scale = 1.0;
    if (!(std::cin >> co2_scale)) {
        return 0;
    }

    std::cout << "\nInput H2O altitude exponent scale:\n\n";
    double h2o_alt_scale = 1.0;
    if (!(std::cin >> h2o_alt_scale)) {
        return 0;
    }

    std::cout << "\nInput 5-8um HAPI TXT path (enter '-' for default):\n";
    std::cout << "Example: c:/Users/zdk/Desktop/project/python/output/hapi_data_5_8um.txt\n\n";
    std::string data_path;
    std::cin >> data_path;
    if (data_path == "-") {
        data_path = "c:/Users/zdk/Desktop/project/python/output/hapi_data_5_8um.txt";
    }

    MultiCoefTable table;
    if (!load_multi_table(data_path, table)) {
        std::cout << "Failed to load spectral table: " << data_path << "\n";
        return 0;
    }

    const double start_um = 5.0;
    const double end_um = 8.0;

    double f = saturation_vapor_density_g_m3(temp_c);
    double f_ratio = (f0 > 0.0) ? (f / f0) : 1.0;

    StandardAtmosphere1976 atmosphere;
    GasProfileRef ref;
    LowtranTransmittance model;
    model.setHumidity(hr, f_ratio);

    double integral = 0.0;
    double prev_lambda = 0.0;
    double prev_tau = 0.0;
    bool first = true;

    for (double lambda = start_um; lambda <= end_um + 1e-12; lambda += step_um) {
        double sample_h_km = (path_type == 0) ? altitude_km : 0.5 * (h1_km + h2_km);

        double temp_k = 288.15;
        double pressure_pa = 101325.0;
        double density = 1.225;
        if (!atmosphere.getProperties(std::max(0.0, sample_h_km), temp_k, pressure_pa, density)) {
            temp_k = 288.15;
            pressure_pa = 101325.0;
            density = 1.225;
        }
        (void)density;

        // HAPI 数据以参考条件给出，这里按 1976 标准大气进行热力学缩放。
        const double p0 = 101325.0;
        const double t0 = 296.0;
        double thermo_scale = (pressure_pa / p0) * (t0 / std::max(150.0, temp_k));

        double x_h2o = profile_h2o(std::max(0.0, sample_h_km), hr);
        double x_co2 = profile_co2(std::max(0.0, sample_h_km), co2_ppm);

        double alpha_h2o_cm_inv = interp_vector(table.wavelength_um, table.alpha_h2o_cm_inv, lambda)
                                * thermo_scale * (x_h2o / ref.h2o_ref);
        double alpha_co2_cm_inv = interp_vector(table.wavelength_um, table.alpha_co2_cm_inv, lambda)
                                * thermo_scale * (x_co2 / ref.co2_ref);

        double u1 = alpha_h2o_cm_inv * 1.0e5 * h2o_scale;
        double delta = alpha_co2_cm_inv * 1.0e5 * co2_scale;

        double tau_h2o_co2 = 1.0;
        if (path_type == 0) {
            tau_h2o_co2 = model.calculateTransmittance(range_km, altitude_km, lambda, u1, delta, h2o_alt_scale);
        } else {
            tau_h2o_co2 = model.calculateSlantTransmittance(h1_km, h2_km, ground_range_km, lambda, u1, delta, h2o_alt_scale);
        }

        // O3/N2O/CO/CH4 采用标准大气分层积分，补充 5-8um 窗口内的主吸收贡献。
        double extra_optical_depth = 0.0;
        if (path_type == 0) {
            extra_optical_depth = extra_optical_depth_segment(table, ref, lambda, altitude_km, range_km, atmosphere);
        } else {
            int steps = 80;
            double dy = h2_km - h1_km;
            double slant_range_km = std::sqrt(ground_range_km * ground_range_km + dy * dy);
            double ds_km = slant_range_km / steps;
            for (int i = 0; i < steps; ++i) {
                double frac = (i + 0.5) / steps;
                double h_km = h1_km + dy * frac;
                extra_optical_depth += extra_optical_depth_segment(table, ref, lambda, h_km, ds_km, atmosphere);
            }
        }

        double tau_extra = std::exp(-extra_optical_depth);
        double tau = tau_h2o_co2 * tau_extra;

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
    double avg_tau = (band_width > 0.0) ? (integral / band_width) : 0.0;

    std::cout << "\nResults:\n";
    std::cout << "  f(T) (g/m^3): " << std::fixed << std::setprecision(4) << f << "\n";
    std::cout << "  f_ratio (f/f0): " << std::fixed << std::setprecision(4) << f_ratio << "\n";
    std::cout << "  Band-average transmittance (5-8um): " << std::fixed << std::setprecision(6) << avg_tau << "\n";

    std::cout << "\n===== CURVE DATA =====\n";
    std::cout << "Wavelength_um,Transmittance,H2O_Alpha_cm_inv,CO2_Alpha_cm_inv,O3_Alpha_cm_inv,N2O_Alpha_cm_inv,CO_Alpha_cm_inv,CH4_Alpha_cm_inv,Total_Alpha_cm_inv\n";

    for (double lambda = start_um; lambda <= end_um + 1e-12; lambda += step_um) {
        double sample_h_km = (path_type == 0) ? altitude_km : 0.5 * (h1_km + h2_km);

        double temp_k = 288.15;
        double pressure_pa = 101325.0;
        double density = 1.225;
        if (!atmosphere.getProperties(std::max(0.0, sample_h_km), temp_k, pressure_pa, density)) {
            temp_k = 288.15;
            pressure_pa = 101325.0;
            density = 1.225;
        }
        (void)density;

        const double p0 = 101325.0;
        const double t0 = 296.0;
        double thermo_scale = (pressure_pa / p0) * (t0 / std::max(150.0, temp_k));

        double x_h2o = profile_h2o(std::max(0.0, sample_h_km), hr);
        double x_co2 = profile_co2(std::max(0.0, sample_h_km), co2_ppm);
        double x_o3 = profile_o3(std::max(0.0, sample_h_km));
        double x_n2o = profile_n2o(std::max(0.0, sample_h_km));
        double x_co = profile_co(std::max(0.0, sample_h_km));
        double x_ch4 = profile_ch4(std::max(0.0, sample_h_km));

        double a_h2o = interp_vector(table.wavelength_um, table.alpha_h2o_cm_inv, lambda) * thermo_scale * (x_h2o / ref.h2o_ref);
        double a_co2 = interp_vector(table.wavelength_um, table.alpha_co2_cm_inv, lambda) * thermo_scale * (x_co2 / ref.co2_ref);
        double a_o3 = interp_vector(table.wavelength_um, table.alpha_o3_cm_inv, lambda) * thermo_scale * (x_o3 / ref.o3_ref);
        double a_n2o = interp_vector(table.wavelength_um, table.alpha_n2o_cm_inv, lambda) * thermo_scale * (x_n2o / ref.n2o_ref);
        double a_co = interp_vector(table.wavelength_um, table.alpha_co_cm_inv, lambda) * thermo_scale * (x_co / ref.co_ref);
        double a_ch4 = interp_vector(table.wavelength_um, table.alpha_ch4_cm_inv, lambda) * thermo_scale * (x_ch4 / ref.ch4_ref);
        double a_total = std::max(0.0, a_h2o + a_co2 + a_o3 + a_n2o + a_co + a_ch4);

        double u1 = a_h2o * 1.0e5 * h2o_scale;
        double delta = a_co2 * 1.0e5 * co2_scale;

        double tau_h2o_co2 = 1.0;
        if (path_type == 0) {
            tau_h2o_co2 = model.calculateTransmittance(range_km, altitude_km, lambda, u1, delta, h2o_alt_scale);
        } else {
            tau_h2o_co2 = model.calculateSlantTransmittance(h1_km, h2_km, ground_range_km, lambda, u1, delta, h2o_alt_scale);
        }

        double extra_optical_depth = 0.0;
        if (path_type == 0) {
            extra_optical_depth = (a_o3 + a_n2o + a_co + a_ch4) * 1.0e5 * range_km;
        } else {
            int steps = 80;
            double dy = h2_km - h1_km;
            double slant_range_km = std::sqrt(ground_range_km * ground_range_km + dy * dy);
            double ds_km = slant_range_km / steps;
            for (int i = 0; i < steps; ++i) {
                double frac = (i + 0.5) / steps;
                double h_km = h1_km + dy * frac;
                extra_optical_depth += extra_optical_depth_segment(table, ref, lambda, h_km, ds_km, atmosphere);
            }
        }

        double tau = tau_h2o_co2 * std::exp(-extra_optical_depth);

        std::cout << std::fixed << std::setprecision(4) << lambda << ","
                  << std::fixed << std::setprecision(6) << tau << ","
                  << std::scientific << std::setprecision(6) << a_h2o << ","
                  << std::scientific << std::setprecision(6) << a_co2 << ","
                  << std::scientific << std::setprecision(6) << a_o3 << ","
                  << std::scientific << std::setprecision(6) << a_n2o << ","
                  << std::scientific << std::setprecision(6) << a_co << ","
                  << std::scientific << std::setprecision(6) << a_ch4 << ","
                  << std::scientific << std::setprecision(6) << a_total << "\n";
    }

    std::cout << "===== END CURVE DATA =====\n";
    return 0;
}
