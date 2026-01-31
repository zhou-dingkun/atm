#include <iostream>
#include <iomanip>
#include "LowtranModel.h"

int main() {
    std::cout << "=== Atmospheric Transmittance (LOWTRAN-style, simplified) ===\n";
    std::cout << "Enter: h1_km h2_km ground_range_km visibility_km\n";
    std::cout << "Example: 0.1 1.0 5.0 23\n\n";

    double h1_km = 0.0;
    double h2_km = 0.0;
    double ground_range_km = 0.0;
    double visibility_km = 23.0;

    if (!(std::cin >> h1_km >> h2_km >> ground_range_km >> visibility_km)) {
        return 0;
    }

    std::cout << "\nEnter wavelength range: start_um end_um step_um\n";
    std::cout << "Example: 8 12 0.5\n\n";

    double start_um = 8.0;
    double end_um = 12.0;
    double step_um = 0.5;
    if (!(std::cin >> start_um >> end_um >> step_um)) {
        return 0;
    }

    LowtranTransmittance model;

    std::cout << "\nGeometry:\n";
    std::cout << "  Start Altitude: " << h1_km << " km\n";
    std::cout << "  End Altitude:   " << h2_km << " km\n";
    std::cout << "  Ground Range:   " << ground_range_km << " km\n";
    std::cout << "  Visibility:     " << visibility_km << " km\n\n";

    std::cout << std::setw(15) << "Wavelength(um)" << std::setw(15) << "Transmittance" << "\n";
    std::cout << std::string(30, '-') << "\n";

    for (double lambda = start_um; lambda <= end_um + 1e-12; lambda += step_um) {
        double trans = model.calculateSlantTransmittance(
            h1_km, h2_km, ground_range_km, lambda, visibility_km);
        std::cout << std::setw(15) << std::fixed << std::setprecision(2) << lambda
                  << std::setw(15) << std::fixed << std::setprecision(4) << trans << "\n";
    }

    return 0;
}
