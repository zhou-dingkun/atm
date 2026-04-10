import argparse
import csv
import math
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass
class Case:
    name: str
    atm_input: Path
    tape5: Path
    coords_x: int
    coords_y: int


def parse_cases(csv_path: Path, base_dir: Path) -> list[Case]:
    cases: list[Case] = []
    with csv_path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            atm_input = Path(row["atm_input"].strip())
            tape5 = Path(row["tape5"].strip())
            if not atm_input.is_absolute():
                atm_input = base_dir / atm_input
            if not tape5.is_absolute():
                tape5 = base_dir / tape5
            cases.append(
                Case(
                    name=row["name"].strip(),
                    atm_input=atm_input,
                    tape5=tape5,
                    coords_x=int(row["coords_x"].strip()),
                    coords_y=int(row["coords_y"].strip()),
                )
            )
    return cases


def extract_first_number(text: str) -> float | None:
    match = re.search(r"(?i)(?:[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?|[-+]?inf|nan)", text)
    if match:
        token = match.group(0).lower()
        if token == "nan":
            return float("nan")
        if token in {"inf", "+inf"}:
            return float("inf")
        if token == "-inf":
            return float("-inf")
        return float(token)
    return None


def build_input_for_5_8um(input_path: Path) -> str:
    lines = [line.strip() for line in input_path.read_text(encoding="utf-8").splitlines() if line.strip()]
    if len(lines) < 10:
        raise RuntimeError(f"Invalid atm input format: {input_path}")

    # 5-8um 专用程序期望顺序：
    # RH/T, path_type, geometry, f0, co2, step, h2o_scale, co2_scale, h2o_alt_scale, data_path
    rh_t = lines[0]
    path_type = lines[1]

    cursor = 2
    if path_type == "0":
        geometry = lines[cursor]
        cursor += 1
    else:
        geometry = lines[cursor]
        cursor += 1

    f0 = lines[cursor]
    co2 = lines[cursor + 1]
    cursor += 2

    # 兼容两种格式：
    # A) 通用版：... scale_by_mix + "start end step"
    # B) 专用版：... 直接给 step
    step_line = lines[cursor]
    next_line = lines[cursor + 1] if cursor + 1 < len(lines) else ""
    if len(step_line.split()) == 1 and len(next_line.split()) == 1:
        # 已是专用格式
        step_um = step_line
        cursor += 1
    else:
        # 通用格式，跳过 scale_by_mix，解析 start/end/step
        cursor += 1
        band_line = lines[cursor]
        band_parts = band_line.split()
        if len(band_parts) < 3:
            raise RuntimeError(f"Invalid band line in atm input: {input_path}")
        step_um = band_parts[2]
        cursor += 1

    h2o_scale = lines[cursor]
    co2_scale = lines[cursor + 1]
    h2o_alt_scale = lines[cursor + 2]
    cursor += 3

    data_path = "-"
    if cursor < len(lines):
        data_path = lines[cursor]

    normalized = [
        rh_t,
        path_type,
        geometry,
        f0,
        co2,
        step_um,
        h2o_scale,
        co2_scale,
        h2o_alt_scale,
        data_path,
        "",
    ]
    return "\n".join(normalized)


def run_atm_5_8um(atm_exe: Path, input_path: Path) -> float:
    data = build_input_for_5_8um(input_path)
    proc = subprocess.run(
        [str(atm_exe)],
        input=data,
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(proc.stdout + "\n" + proc.stderr)

    # 兼容:
    # - Band-average transmittance:
    # - Band-average transmittance (5-8um):
    key = "Band-average transmittance"
    for line in proc.stdout.splitlines():
        if key in line:
            value = extract_first_number(line.split(":", 1)[-1])
            if value is not None:
                return value

    raise RuntimeError(
        "Band-average transmittance not found in 5-8um ATM output\n"
        f"stdout:\n{proc.stdout}\n"
        f"stderr:\n{proc.stderr}"
    )


def run_lowtran(lowtran_auto: Path, tape5: Path, coords_x: int, coords_y: int) -> float:
    proc = subprocess.run(
        [
            str(lowtran_auto),
            "--tape5",
            str(tape5),
            "--coords",
            str(coords_x),
            str(coords_y),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(proc.stdout + "\n" + proc.stderr)

    key = "AVERAGE TRANSMITTANCE"
    for line in proc.stdout.splitlines():
        if key in line:
            value = extract_first_number(line)
            if value is not None:
                return value

    # 回退读取 TAPE6
    tape6 = lowtran_auto.resolve().parent.parent / "lowtran7" / "TAPE6"
    if tape6.exists():
        text = tape6.read_text(encoding="utf-8", errors="ignore")
        for line in text.splitlines():
            if key in line:
                value = extract_first_number(line)
                if value is not None:
                    return value

    raise RuntimeError(
        "AVERAGE TRANSMITTANCE not found in lowtran output/TAPE6\n"
        f"stdout:\n{proc.stdout}\n"
        f"stderr:\n{proc.stderr}"
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Compare dedicated 5-8um ATM executable with lowtran7"
    )
    parser.add_argument(
        "--cases-file",
        type=Path,
        default=Path("compare_cases_5_8um.csv"),
        help="Case CSV file path (default: compare_cases_5_8um.csv)",
    )
    parser.add_argument(
        "--atm-exe",
        type=Path,
        default=Path("src/transmittance_5_8um.exe"),
        help="Dedicated 5-8um ATM executable path",
    )
    parser.add_argument(
        "--lowtran-auto",
        type=Path,
        default=Path("src/lowtran_auto.exe"),
        help="lowtran automation executable path",
    )
    parser.add_argument(
        "--save-csv",
        type=Path,
        default=Path("compare_result_5_8um.csv"),
        help="Output CSV path for comparison results",
    )
    args = parser.parse_args()

    root = Path(__file__).resolve().parent

    csv_path = args.cases_file
    if not csv_path.is_absolute():
        csv_path = root / csv_path

    atm_exe = args.atm_exe
    if not atm_exe.is_absolute():
        atm_exe = root / atm_exe

    lowtran_auto = args.lowtran_auto
    if not lowtran_auto.is_absolute():
        lowtran_auto = root / lowtran_auto

    save_csv = args.save_csv
    if not save_csv.is_absolute():
        save_csv = root / save_csv

    if not csv_path.exists():
        raise SystemExit(f"Missing case file: {csv_path}")
    if not atm_exe.exists():
        raise SystemExit(
            "Missing dedicated 5-8um executable: "
            f"{atm_exe}\n"
            "Build example:\n"
            "cl.exe /Fe:src\\transmittance_5_8um.exe src\\transmittance_5_8um.cpp src\\Atmosphere.cpp src\\LowtranModel.cpp"
        )
    if not lowtran_auto.exists():
        raise SystemExit(f"Missing lowtran_auto executable: {lowtran_auto}")

    cases = parse_cases(csv_path, root)
    rows: list[tuple[str, float, float, float]] = []

    for case in cases:
        atm_val = float("nan")
        low_val = float("nan")

        try:
            atm_val = run_atm_5_8um(atm_exe, case.atm_input)
        except Exception as ex:
            print(f"WARN [{case.name}] ATM failed: {ex}")

        try:
            low_val = run_lowtran(lowtran_auto, case.tape5, case.coords_x, case.coords_y)
        except Exception as ex:
            print(f"WARN [{case.name}] LOWTRAN failed: {ex}")

        if math.isfinite(atm_val) and math.isfinite(low_val) and low_val != 0:
            rel_err = abs(atm_val - low_val) / abs(low_val)
        else:
            rel_err = float("nan")
        rows.append((case.name, atm_val, low_val, rel_err))

    print("name,atm_5_8um,lowtran,rel_error")
    for name, atm_val, low_val, rel_err in rows:
        print(f"{name},{atm_val:.6f},{low_val:.6f},{rel_err:.6f}")

    with save_csv.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["name", "atm_5_8um", "lowtran", "rel_error"])
        writer.writerows(rows)

    valid_err = [r[3] for r in rows if math.isfinite(r[3])]
    avg_err = sum(valid_err) / len(valid_err) if valid_err else float("nan")
    print(f"Average relative error (finite rows): {avg_err:.6f}")
    print(f"Saved: {save_csv}")


if __name__ == "__main__":
    main()
