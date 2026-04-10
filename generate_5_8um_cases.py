import csv
from dataclasses import dataclass
from pathlib import Path


@dataclass
class StandardCase:
    name: str
    altitude_km: float
    range_km: float
    rh: float
    temperature_c: float
    f0: float
    co2_ppm: float
    step_um: float


def write_atm_input(case: StandardCase, out_path: Path) -> None:
    content = "\n".join(
        [
            f"{case.rh:.2f} {case.temperature_c:.1f}",
            "0",
            f"{case.altitude_km:.3f} {case.range_km:.3f}",
            f"{case.f0:.2f}",
            f"{case.co2_ppm:.1f}",
            "1",
            f"5.000 8.000 {case.step_um:.3f}",
            "-1",
            "-1",
            "-1",
            "-",
            "-",
            "",
        ]
    )
    out_path.write_text(content, encoding="utf-8")


def write_tape5(case: StandardCase, out_path: Path, template_lines: list[str] | None) -> None:
    v1 = 1.0e4 / 8.0
    v2 = 1.0e4 / 5.0
    dv_cm = 5.0

    line3 = (
        f"{case.altitude_km:10.3f}{case.altitude_km:10.3f}{0.0:10.3f}"
        f"{case.range_km:10.3f}{0.0:10.3f}{6371.0:10.3f}{0:5d}"
    )
    line4 = f"{v1:10.3f}{v2:10.3f}{dv_cm:10.3f}"
    line5 = f"{0:5d}"

    if template_lines and len(template_lines) >= 5:
        lines = template_lines[:]
        lines[2] = line3.ljust(max(len(template_lines[2]), len(line3)))
        lines[3] = line4.ljust(max(len(template_lines[3]), len(line4)))
        lines[4] = line5.ljust(max(len(template_lines[4]), len(line5)))
    else:
        lines = [
            "    6    1    0    0    0    0    0    0    0    0    0    1    0   0.000   0.80",
            "    5    1    1    0    0    0     0.000     0.000     0.000     0.000     0.000",
            line3,
            line4,
            line5,
        ]
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def build_standard_cases() -> list[StandardCase]:
    # 覆盖低层典型高度和路径长度，保持 RH/T/f0/CO2 一致，便于可重复对比。
    base = {
        "rh": 0.46,
        "temperature_c": 15.0,
        "f0": 6.8,
        "co2_ppm": 400.0,
        "step_um": 0.05,
    }
    grid = [
        (0.2, 0.5),
        (0.2, 1.0),
        (0.2, 2.0),
        (0.5, 0.5),
        (0.5, 1.0),
        (0.5, 2.0),
        (1.0, 0.5),
        (1.0, 1.0),
        (2.0, 1.0),
        (4.0, 2.0),
    ]
    cases: list[StandardCase] = []
    for idx, (altitude_km, range_km) in enumerate(grid, start=1):
        cases.append(
            StandardCase(
                name=f"case5_8_{idx:02d}",
                altitude_km=altitude_km,
                range_km=range_km,
                rh=base["rh"],
                temperature_c=base["temperature_c"],
                f0=base["f0"],
                co2_ppm=base["co2_ppm"],
                step_um=base["step_um"],
            )
        )
    return cases


def main() -> None:
    root = Path(__file__).resolve().parent
    atm_dir = root / "testdata_5_8um" / "atm_inputs"
    tape5_dir = root / "testdata_5_8um" / "Tape5"
    atm_dir.mkdir(parents=True, exist_ok=True)
    tape5_dir.mkdir(parents=True, exist_ok=True)

    template_path = root / "lowtran7" / "example" / "Tape5"
    if not template_path.exists():
        template_path = root / "lowtran7" / "Tape5"
    template_lines = None
    if template_path.exists():
        template_lines = template_path.read_text(encoding="utf-8").splitlines()

    cases = build_standard_cases()

    compare_csv = root / "compare_cases_5_8um.csv"
    profile_csv = root / "testdata_5_8um" / "case_profile_5_8um.csv"

    with compare_csv.open("w", newline="", encoding="utf-8") as f_compare, profile_csv.open(
        "w", newline="", encoding="utf-8"
    ) as f_profile:
        compare_writer = csv.writer(f_compare)
        profile_writer = csv.writer(f_profile)

        compare_writer.writerow(["name", "atm_input", "tape5", "coords_x", "coords_y"])
        profile_writer.writerow(
            [
                "name",
                "rh",
                "temperature_c",
                "path_type",
                "altitude_km",
                "range_km",
                "f0_g_m3",
                "co2_ppm",
                "band_start_um",
                "band_end_um",
                "step_um",
                "h2o_scale",
                "co2_scale",
                "h2o_alt_scale",
            ]
        )

        for case in cases:
            atm_input = atm_dir / f"{case.name}.txt"
            tape5 = tape5_dir / f"Tape5_{case.name}"
            write_atm_input(case, atm_input)
            write_tape5(case, tape5, template_lines)

            compare_writer.writerow([case.name, str(atm_input), str(tape5), 800, 600])
            profile_writer.writerow(
                [
                    case.name,
                    f"{case.rh:.2f}",
                    f"{case.temperature_c:.1f}",
                    "horizontal",
                    f"{case.altitude_km:.3f}",
                    f"{case.range_km:.3f}",
                    f"{case.f0:.2f}",
                    f"{case.co2_ppm:.1f}",
                    "5.000",
                    "8.000",
                    f"{case.step_um:.3f}",
                    "-1",
                    "-1",
                    "-1",
                ]
            )

    print("Generated standardized 5-8um datasets:")
    print(f"- {compare_csv}")
    print(f"- {profile_csv}")
    print(f"- {atm_dir}")
    print(f"- {tape5_dir}")


if __name__ == "__main__":
    main()
