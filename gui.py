import subprocess
import tkinter as tk
from tkinter import ttk, messagebox
from pathlib import Path


def build_input(values: dict) -> str:
    lines = [
        f"{values['rh']} {values['temp_c']}",
        values["path_type"],
    ]
    if values["path_type"] == "0":
        lines.append(f"{values['alt_km']} {values['range_km']}")
    else:
        lines.append(f"{values['h1_km']} {values['h2_km']} {values['ground_km']}")
    lines.extend(
        [
            values["f0"],
            values["co2_ppm"],
            values["scale_by_mix"],
            f"{values['start_um']} {values['end_um']} {values['step_um']}",
            values["h2o_scale"],
            values["co2_scale"],
            values["h2o_alt_scale"],
            values["h2o_csv"],
            values["co2_csv"],
        ]
    )
    return "\n".join(lines) + "\n"


def run_model():
    values = {k: v.get().strip() for k, v in inputs.items()}
    if not values["rh"] or not values["temp_c"]:
        messagebox.showerror("输入错误", "请填写 RH 和 Temperature。")
        return

    exe_path = Path(values["exe_path"]).expanduser()
    if not exe_path.exists():
        messagebox.showerror("路径错误", f"找不到可执行文件: {exe_path}")
        return

    try:
        payload = build_input(values)
        completed = subprocess.run(
            [str(exe_path)],
            input=payload,
            text=True,
            capture_output=True,
            check=False,
        )
    except Exception as exc:
        messagebox.showerror("运行失败", str(exc))
        return

    if completed.returncode != 0:
        output_text.delete("1.0", tk.END)
        output_text.insert(tk.END, completed.stdout)
        output_text.insert(tk.END, "\n[stderr]\n" + completed.stderr)
        messagebox.showwarning("程序返回非零", f"exit code={completed.returncode}")
        return

    output_text.delete("1.0", tk.END)
    output_text.insert(tk.END, completed.stdout)


root = tk.Tk()
root.title("Atmospheric Transmittance GUI")

main = ttk.Frame(root, padding=10)
main.grid(row=0, column=0, sticky="nsew")
root.columnconfigure(0, weight=1)
root.rowconfigure(0, weight=1)

inputs = {}


def add_row(label, key, default, row):
    ttk.Label(main, text=label).grid(row=row, column=0, sticky="w", padx=4, pady=2)
    entry = ttk.Entry(main)
    entry.insert(0, default)
    entry.grid(row=row, column=1, sticky="ew", padx=4, pady=2)
    inputs[key] = entry


main.columnconfigure(1, weight=1)

add_row("atm.exe 路径", "exe_path", r"c:\Users\zdk\Desktop\project\atm\src\atm.exe", 0)
add_row("RH (0-1)", "rh", "0.46", 1)
add_row("Temperature (C)", "temp_c", "15", 2)
add_row("路径类型 (0=水平,1=斜程)", "path_type", "0", 3)
add_row("Altitude_km (水平)", "alt_km", "0.2", 4)
add_row("Range_km (水平)", "range_km", "1.0", 5)
add_row("H1_km (斜程)", "h1_km", "0.0", 6)
add_row("H2_km (斜程)", "h2_km", "1.0", 7)
add_row("GroundRange_km (斜程)", "ground_km", "0.0", 8)
add_row("f0 (g/m^3)", "f0", "6.8", 9)
add_row("CO2 ppm", "co2_ppm", "400", 10)
add_row("按混合比缩放 (1/0)", "scale_by_mix", "1", 11)
add_row("start_um", "start_um", "3", 12)
add_row("end_um", "end_um", "5", 13)
add_row("step_um", "step_um", "0.05", 14)
add_row("H2O_scale (-1默认)", "h2o_scale", "-1", 15)
add_row("CO2_scale (-1默认)", "co2_scale", "-1", 16)
add_row("H2O_alt_scale (-1默认)", "h2o_alt_scale", "-1", 17)
add_row("H2O CSV 路径", "h2o_csv", "-", 18)
add_row("CO2 CSV 路径", "co2_csv", "-", 19)

run_button = ttk.Button(main, text="运行", command=run_model)
run_button.grid(row=20, column=0, columnspan=2, sticky="ew", padx=4, pady=6)

output_text = tk.Text(main, height=18, wrap="word")
output_text.grid(row=21, column=0, columnspan=2, sticky="nsew", padx=4, pady=4)
main.rowconfigure(21, weight=1)

root.mainloop()
