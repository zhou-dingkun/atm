import argparse
import csv
import random
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


def write_tape5(
    path: Path,
    altitude_km: float,
    range_km: float,
    start_um: float,
    end_um: float,
    dv_cm: float,
    template_lines: list[str] | None,
) -> None:
    v1 = 1.0e4 / end_um
    v2 = 1.0e4 / start_um
    line3 = (
        f"{altitude_km:10.3f}{altitude_km:10.3f}{0.0:10.3f}"
        f"{range_km:10.3f}{0.0:10.3f}{6371.0:10.3f}{0:5d}"
    )
    line4 = f"{v1:10.3f}{v2:10.3f}{dv_cm:10.3f}"
    line5 = f"{0:5d}"

    def align_to_template(template_line: str | None, content: str) -> str:
        if not template_line:
            return content
        target_len = len(template_line)
        if target_len <= len(content):
            return content
        return content.ljust(target_len)
    if template_lines and len(template_lines) >= 5:
        lines = template_lines[:]
        lines[2] = align_to_template(template_lines[2], line3)
        lines[3] = align_to_template(template_lines[3], line4)
        lines[4] = align_to_template(template_lines[4], line5)
    else:
        lines = [
            "    6    1    0    0    0    0    0    0    0    0    0    1    0   0.000   0.80",
            "    5    1    1    0    0    0     0.000     0.000     0.000     0.000     0.000",
            line3,
            line4,
            line5,
        ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_atm_input(
    path: Path,
    altitude_km: float,
    range_km: float,
    start_um: float,
    end_um: float,
    step_um: float,
) -> None:
    content = "\n".join(
        [
            "0.46 15",
            "0",
            f"{altitude_km:.3f} {range_km:.3f}",
            "6.8",
            "400",
            "1",
            f"{start_um:.3f} {end_um:.3f} {step_um:.3f}",
            "-1",
            "-1",
            "-1",
            "-",
            "-",
            "",
        ]
    )
    path.write_text(content, encoding="utf-8")


def generate_cases(
    root: Path,
    count: int,
    seed: int,
    alt_min: float,
    alt_max: float,
    range_min: float,
    range_max: float,
    bands: list[tuple[float, float]],
    step_min: float,
    step_max: float,
) -> None:
    random.seed(seed)
    cases = []
    tape5_dir = root / "lowtran7" / "generated"
    tape5_dir.mkdir(parents=True, exist_ok=True)

    template_path = root / "lowtran7" / "example" / "Tape5"
    if not template_path.exists():
        template_path = root / "lowtran7" / "Tape5"
    template_lines = None
    if template_path.exists():
        template_lines = template_path.read_text(encoding="utf-8").splitlines()

    for idx in range(1, count + 1):
        altitude_km = round(random.uniform(alt_min, alt_max), 3)
        range_km = round(random.uniform(range_min, range_max), 3)
        start_um, end_um = random.choice(bands)
        step_um = round(random.uniform(step_min, step_max), 3)
        step_um = max(step_um, 0.01)

        atm_input = root / f"atm_input_case{idx}.txt"
        tape5 = tape5_dir / f"Tape5_case{idx}"
        write_atm_input(atm_input, altitude_km, range_km, start_um, end_um, step_um)
        write_tape5(tape5, altitude_km, range_km, start_um, end_um, 5.0, template_lines)

        cases.append((f"case{idx}", atm_input.name, str(tape5), 800, 600))

    csv_path = root / "compare_cases.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["name", "atm_input", "tape5", "coords_x", "coords_y"])
        writer.writerows(cases)



def run_atm(atm_exe: Path, input_path: Path) -> float:
    data = input_path.read_text(encoding="utf-8") + "\n"
    proc = subprocess.run(
        [str(atm_exe)],
        input=data,
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(proc.stdout + "\n" + proc.stderr)

    key = "Band-average transmittance:"
    for line in proc.stdout.splitlines():
        if key in line:
            value = line.split(":", 1)[1].strip()
            return float(value)
    raise RuntimeError("Band-average transmittance not found in atm output")


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
            m = re.search(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?", line)
            if m:
                return float(m.group(0))

    # 回退：某些运行场景下 lowtran_auto 只提示完成，不回显平均透过率。
    tape6 = lowtran_auto.resolve().parent.parent / "lowtran7" / "TAPE6"
    if tape6.exists():
        text = tape6.read_text(encoding="utf-8", errors="ignore")
        for line in text.splitlines():
            if key in line:
                m = re.search(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?", line)
                if m:
                    return float(m.group(0))

    raise RuntimeError(
        "AVERAGE TRANSMITTANCE not found in lowtran output/TAPE6\n"
        f"stdout:\n{proc.stdout}\n"
        f"stderr:\n{proc.stderr}"
    )


def configure_plot_font(plt) -> None:
    try:
        import matplotlib.font_manager as fm
    except Exception:
        return

    candidates = [
        "Microsoft YaHei",
        "SimHei",
        "SimSun",
        "Noto Sans CJK SC",
        "Source Han Sans CN",
        "Arial Unicode MS",
    ]
    available = {f.name for f in fm.fontManager.ttflist}
    selected = next((name for name in candidates if name in available), None)

    # 优先使用可用中文字体，避免标题/注释出现缺字告警。
    if selected is not None:
        plt.rcParams["font.sans-serif"] = [selected, *candidates]
    else:
        plt.rcParams["font.sans-serif"] = candidates
    plt.rcParams["axes.unicode_minus"] = False


def plot_results(rows: list[tuple[str, float, float, float]], output_path: Path, show: bool) -> None:
    try:
        import matplotlib.pyplot as plt
    except Exception:
        print("matplotlib not available, skipping plot")
        return

    configure_plot_font(plt)

    names = [r[0] for r in rows]
    atm_vals = [r[1] for r in rows]
    low_vals = [r[2] for r in rows]
    err_vals = [r[3] for r in rows]
    avg_err = sum(err_vals) / len(err_vals) if err_vals else 0.0

    fig, (ax_scatter, ax_err) = plt.subplots(1, 2, figsize=(11, 5), gridspec_kw={"width_ratios": [1.5, 1]})

    ax_scatter.plot(low_vals, atm_vals, "o", label="样本点")
    min_v = min(min(low_vals), min(atm_vals))
    max_v = max(max(low_vals), max(atm_vals))
    if abs(max_v - min_v) < 1e-12:
        max_v = min_v + 1.0
    ax_scatter.plot([min_v, max_v], [min_v, max_v], "--", label="参考线 y=x")
    ax_scatter.set_xlabel("Lowtran 平均透过率")
    ax_scatter.set_ylabel("ATM 平均透过率")
    ax_scatter.set_title(f"ATM 与 Lowtran 对比（平均相对误差={avg_err * 100:.2f}%）")
    ax_scatter.grid(True, alpha=0.3)
    ax_scatter.legend()

    for i, name in enumerate(names):
        ax_scatter.annotate(
            f"{name}\n误差={err_vals[i] * 100:.2f}%",
            (low_vals[i], atm_vals[i]),
            textcoords="offset points",
            xytext=(4, 4),
            fontsize=8,
        )

    idx = list(range(len(names)))
    err_percent = [e * 100.0 for e in err_vals]
    ax_err.bar(idx, err_percent, alpha=0.8)
    ax_err.axhline(avg_err * 100.0, linestyle="--", linewidth=1.2, label=f"平均误差={avg_err * 100:.2f}%")
    ax_err.set_xticks(idx)
    ax_err.set_xticklabels(names, rotation=45, ha="right")
    ax_err.set_ylabel("相对误差 (%)")
    ax_err.set_title("各样本相对误差")
    ax_err.grid(True, axis="y", alpha=0.3)
    ax_err.legend()

    fig.tight_layout()
    fig.savefig(output_path, dpi=150)
    if show:
        plt.show()

    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--generate", action="store_true", help="Generate random cases")
    parser.add_argument("--count", type=int, default=10, help="Number of random cases")
    parser.add_argument("--seed", type=int, default=7, help="Random seed for generation")
    parser.add_argument("--alt-min", type=float, default=0.2, help="Min altitude (km)")
    parser.add_argument("--alt-max", type=float, default=8.0, help="Max altitude (km)")
    parser.add_argument("--range-min", type=float, default=0.5, help="Min range (km)")
    parser.add_argument("--range-max", type=float, default=2.0, help="Max range (km)")
    parser.add_argument("--step-min", type=float, default=0.05, help="Min wavelength step (um)")
    parser.add_argument("--step-max", type=float, default=0.2, help="Max wavelength step (um)")
    parser.add_argument("--plot", dest="plot", action="store_true", default=True, help="Generate plot image (default)")
    parser.add_argument("--no-plot", dest="plot", action="store_false", help="Disable plot image")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("compare_plot.png"),
        help="Plot image output path",
    )
    parser.add_argument(
        "--cases-file",
        type=Path,
        default=Path("compare_cases.csv"),
        help="Case CSV file path (default: compare_cases.csv)",
    )
    parser.add_argument("--show", action="store_true", help="Show plot window")
    args = parser.parse_args()

    root = Path(__file__).resolve().parent
    csv_path = args.cases_file
    if not csv_path.is_absolute():
        csv_path = root / csv_path
    atm_exe = root / "src" / "atm.exe"
    lowtran_auto = root / "src" / "lowtran_auto.exe"

    if args.generate:
        generate_cases(
            root,
            args.count,
            args.seed,
            args.alt_min,
            args.alt_max,
            args.range_min,
            args.range_max,
            [(3.0, 5.0), (8.0, 12.0)],
            args.step_min,
            args.step_max,
        )

    if not csv_path.exists():
        raise SystemExit(f"Missing case file: {csv_path}")
    if not atm_exe.exists():
        raise SystemExit(f"Missing atm.exe: {atm_exe}")
    if not lowtran_auto.exists():
        raise SystemExit(f"Missing lowtran_auto.exe: {lowtran_auto}")

    cases = parse_cases(csv_path, root)
    rows = []
    for case in cases:
        atm_val = run_atm(atm_exe, case.atm_input)
        low_val = run_lowtran(lowtran_auto, case.tape5, case.coords_x, case.coords_y)
        rel_err = abs(atm_val - low_val) / low_val if low_val != 0 else 0.0
        rows.append((case.name, atm_val, low_val, rel_err))

    print("name,atm,lowtran,rel_error")
    for name, atm_val, low_val, rel_err in rows:
        print(f"{name},{atm_val:.6f},{low_val:.6f},{rel_err:.6f}")

    if args.plot:
        output_path = args.output
        if not output_path.is_absolute():
            output_path = root / output_path
        plot_results(rows, output_path, args.show)


if __name__ == "__main__":
    main()
