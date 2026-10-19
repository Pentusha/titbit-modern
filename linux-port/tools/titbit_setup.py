"""Import original game data, from a folder or disc image, without mounting it.

The GUI needs only Python's standard library; the packaged exe needs no Python.
The command-line modes deliberately do not initialize Tk or open a game window.
"""
from __future__ import annotations

import argparse
import dataclasses
import json
import os
from pathlib import Path
import queue
import subprocess
import sys
import threading
import time

import titbit_assets as assets


APP = "Титбит · Подготовка игры"
BUNDLE = Path(sys.executable).resolve().parent if getattr(sys, "frozen", False) else Path(__file__).resolve().parents[1]
DEFAULT_DEST = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share')) / 'titbit' / 'game'


def json_default(value):
    if dataclasses.is_dataclass(value):
        return dataclasses.asdict(value)
    if isinstance(value, Path):
        return str(value)
    if isinstance(value, (set, tuple)):
        return list(value)
    return str(value)


def size_text(size):
    size = float(size)
    for unit in ("Б", "КиБ", "МиБ", "ГиБ"):
        if size < 1024 or unit == "ГиБ":
            return f"{size:,.1f} {unit}".replace(",", " ")
        size /= 1024


def run_cli(args):
    plan = None
    code = 0
    try:
        if args.inspect:
            result = plan = assets.inspect_source(args.inspect)
        elif args.import_source:
            if not args.dest:
                raise ValueError("Для импорта укажите папку назначения: --dest ПАПКА")
            plan = assets.inspect_source(args.import_source)
            result = assets.install_assets(plan, args.dest)
        else:
            result = assets.verify_install(args.verify)
    except (ValueError, OSError) as exc:
        result = {"error": str(exc)}
        code = 1
    finally:
        if plan is not None:
            plan.close()
    payload = json.dumps(result, ensure_ascii=False, indent=2, default=json_default)
    try:
        if args.report:
            Path(args.report).write_text(payload + "\n", encoding="utf-8")
    except OSError as exc:
        payload = json.dumps({"error": "Не удалось записать отчёт: " + str(exc)}, ensure_ascii=False)
        code = 1
    stream = sys.stderr if code else sys.stdout
    if stream is not None:
        print(payload, file=stream)
    return code


def run_gui():
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk

    class Setup:
        def __init__(self, root):
            self.root = root
            self.events = queue.Queue()
            self.cancel = threading.Event()
            self.worker = None
            self.task_active = False
            self.closing = False
            self.plan = None
            self.source = tk.StringVar()
            self.destination = tk.StringVar(value=str(DEFAULT_DEST))
            self.status = tk.StringVar(value="Выберите вашу копию игры или образ диска.")
            self.summary = tk.StringVar(value="Файлы ещё не проверены")
            root.title(APP)
            root.geometry("820x690")
            root.minsize(720, 600)
            root.protocol("WM_DELETE_WINDOW", self.close)
            style = ttk.Style(root)
            if "vista" in style.theme_names():
                style.theme_use("vista")
            style.configure("Title.TLabel", font=("DejaVu Sans", 19, "bold"))
            style.configure("Sub.TLabel", foreground="#52616b")
            frame = ttk.Frame(root, padding=22)
            frame.pack(fill="both", expand=True)
            ttk.Label(frame, text="Подготовим Титбит к запуску", style="Title.TLabel").pack(anchor="w")
            ttk.Label(frame, text="Оригинальные уровни, модели, музыка и видео — из вашей копии игры.",
                      style="Sub.TLabel").pack(anchor="w", pady=(5, 18))

            inputs = ttk.LabelFrame(frame, text="1. Откуда взять файлы", padding=12)
            inputs.pack(fill="x")
            inputs.columnconfigure(0, weight=1)
            self.source_entry = ttk.Entry(inputs, textvariable=self.source)
            self.source_entry.grid(row=0, column=0, sticky="ew", columnspan=3)
            self.folder_button = ttk.Button(inputs, text="Папка игры / DVD…", command=self.choose_folder)
            self.folder_button.grid(row=1, column=0, sticky="w", pady=(8, 0))
            self.image_button = ttk.Button(inputs, text="Образ диска…", command=self.choose_image)
            self.image_button.grid(row=1, column=1, padx=8, pady=(8, 0))
            self.scan_button = ttk.Button(inputs, text="Проверить источник", command=self.scan)
            self.scan_button.grid(row=1, column=2, sticky="e", pady=(8, 0))
            ttk.Label(inputs, text="ISO, IMG, CUE, CCD или установленная игра. Образ подключать не нужно.",
                      style="Sub.TLabel").grid(row=2, column=0, columnspan=3, sticky="w", pady=(8, 0))

            target = ttk.LabelFrame(frame, text="2. Куда скопировать", padding=12)
            target.pack(fill="x", pady=(12, 0))
            target.columnconfigure(0, weight=1)
            self.target_entry = ttk.Entry(target, textvariable=self.destination)
            self.target_entry.grid(row=0, column=0, sticky="ew")
            self.target_button = ttk.Button(target, text="Выбрать…", command=self.choose_target)
            self.target_button.grid(row=0, column=1, padx=(8, 0))
            ttk.Label(target, text="Папка данных для игры и Studio. Существующие сохранения не изменяются.",
                      style="Sub.TLabel").grid(row=1, column=0, columnspan=2, sticky="w", pady=(6, 0))

            preview = ttk.Frame(frame)
            preview.pack(fill="both", expand=True, pady=(14, 0))
            ttk.Label(preview, textvariable=self.summary).pack(anchor="w", pady=(0, 6))
            self.files = ttk.Treeview(preview, columns=("size",), show="tree headings", height=6)
            self.files.heading("#0", text="Файлы игры")
            self.files.heading("size", text="Размер")
            self.files.column("#0", width=510)
            self.files.column("size", width=110, anchor="e", stretch=False)
            self.files.pack(fill="both", expand=True)
            footer = ttk.Frame(frame)
            footer.pack(side="bottom", fill="x", before=preview)
            self.progress = ttk.Progressbar(footer, mode="determinate")
            self.progress.pack(fill="x", pady=(12, 5))
            ttk.Label(footer, textvariable=self.status, wraplength=750).pack(anchor="w")
            self.log = tk.Text(footer, height=3, state="disabled", wrap="word", relief="flat",
                               background="#f1f3f4", font=("DejaVu Sans", 9))
            self.log.pack(fill="x", pady=(8, 10))
            actions = ttk.Frame(footer)
            actions.pack(fill="x")
            self.install_button = ttk.Button(actions, text="Добавить файлы игры", command=self.install, state="disabled")
            self.install_button.pack(side="left")
            self.verify_button = ttk.Button(actions, text="Проверить готовую папку", command=self.verify)
            self.verify_button.pack(side="left", padx=8)
            self.cancel_button = ttk.Button(actions, text="Отмена", command=self.request_cancel, state="disabled")
            self.cancel_button.pack(side="right")
            launch = ttk.Frame(footer)
            launch.pack(fill="x", pady=(12, 0))
            self.launch_buttons = []
            for title, name in (("Запустить игру", "titbit"), ("Открыть Studio", "studio")):
                button = ttk.Button(launch, text=title, command=lambda n=name: self.launch(n))
                button.pack(side="left", padx=(0, 8))
                self.launch_buttons.append((button, name))
            self.controls = [self.source_entry, self.target_entry, self.folder_button, self.image_button,
                             self.scan_button, self.target_button, self.verify_button]
            self.source.trace_add("write", self.source_changed)
            self.destination.trace_add("write", lambda *_: self.refresh_launches())
            self.refresh_launches()
            root.after(100, self.poll)

        def source_changed(self, *_):
            if self.plan is not None:
                self.plan.close()
            self.plan = None
            self.install_button.configure(state="disabled")
            self.files.delete(*self.files.get_children())
            self.summary.set("Файлы ещё не проверены")

        def choose_folder(self):
            value = filedialog.askdirectory(parent=self.root, title="Папка установленной игры или диска")
            if value:
                self.source.set(value)
                self.scan()

        def choose_image(self):
            value = filedialog.askopenfilename(parent=self.root, title="Образ оригинального диска", filetypes=[
                ("Образы диска", "*.iso *.ISO *.img *.IMG *.cue *.CUE *.ccd *.CCD"), ("Все файлы", "*.*")])
            if value:
                self.source.set(value)
                self.scan()

        def choose_target(self):
            value = filedialog.askdirectory(parent=self.root, title="Папка Titbit Modern", mustexist=False)
            if value:
                self.destination.set(value)

        def append(self, text):
            self.log.configure(state="normal")
            self.log.insert("end", str(text) + "\n")
            self.log.see("end")
            self.log.configure(state="disabled")

        def busy(self, state):
            self.task_active = state
            for item in self.controls:
                item.configure(state="disabled" if state else "normal")
            self.install_button.configure(state="normal" if not state and self.plan else "disabled")
            self.cancel_button.configure(state="normal" if state else "disabled")
            for button, _ in self.launch_buttons:
                button.configure(state="disabled")
            if not state:
                self.refresh_launches()

        def begin(self, kind, action):
            if self.worker and self.worker.is_alive():
                return
            self.cancel.clear()
            self.busy(True)
            self.progress.configure(value=0)
            self.status.set({"scan": "Читаем каталог файлов…", "install": "Добавляем файлы игры…",
                             "verify": "Проверяем файлы игры…"}[kind])
            def work():
                last = 0.0
                def progress(name, done, total):
                    nonlocal last
                    now = time.monotonic()
                    if now - last > 0.1 or done == total:
                        self.events.put(("progress", (str(name), done, total)))
                        last = now
                try:
                    result = action(progress)
                    self.events.put(("done", (kind, result)))
                except Exception as exc:
                    message = str(exc) if isinstance(exc, (ValueError, OSError)) else (
                        "Не удалось обработать источник. Проверьте целостность образа и распакуйте пакет заново.")
                    self.events.put(("error", message))
            self.worker = threading.Thread(target=work, name="Titbit asset setup", daemon=False)
            self.worker.start()

        def scan(self):
            source = self.source.get().strip()
            if not source:
                messagebox.showinfo(APP, "Сначала выберите папку игры или образ диска.", parent=self.root)
                return
            self.source_changed()
            self.begin("scan", lambda _: assets.inspect_source(source))

        def install(self):
            if not self.plan:
                return
            dest = self.destination.get().strip()
            if not dest:
                messagebox.showinfo(APP, "Выберите папку назначения.", parent=self.root)
                return
            plan = self.plan
            self.begin("install", lambda progress: assets.install_assets(plan, dest, progress=progress, cancel=self.cancel))

        def verify(self):
            dest = self.destination.get().strip()
            if not dest:
                messagebox.showinfo(APP, "Выберите папку с файлами игры.", parent=self.root)
                return
            self.begin("verify", lambda progress: assets.verify_install(dest, progress=progress, cancel=self.cancel))

        def request_cancel(self):
            self.cancel.set()
            self.cancel_button.configure(state="disabled")
            self.status.set("Отменяем операцию. Дождитесь завершения текущего чтения…")

        def poll(self):
            try:
                while True:
                    event, data = self.events.get_nowait()
                    if event == "progress":
                        name, done, total = data
                        self.progress.configure(maximum=max(total, 1), value=done)
                        self.status.set(f"{name} · {size_text(done)} / {size_text(total)}")
                    elif event == "done":
                        kind, result = data
                        if kind == "scan":
                            self.plan = result
                            self.files.delete(*self.files.get_children())
                            total = 0
                            for name, item in result.files.items():
                                size = getattr(item, "size", 0)
                                total += size
                                self.files.insert("", "end", text=name, values=(size_text(size),))
                            self.summary.set(f"{result.description} · {len(result.files)} файлов · {size_text(total)}")
                            for warning in result.warnings:
                                self.append(warning)
                            self.status.set("Файлы найдены. При добавлении проверим контрольные суммы SHA256.")
                        else:
                            self.progress.configure(maximum=100, value=100)
                            self.status.set("Файлы готовы. Можно запускать игру и Studio.")
                            self.append("Проверка завершена." if kind == "verify" else "Файлы добавлены; сохранения не изменены.")
                        self.busy(False)
                    elif event == "error":
                        cancelled = self.cancel.is_set()
                        self.status.set("Операция отменена." if cancelled else "Операция не завершена.")
                        self.append(data)
                        self.busy(False)
                        if not cancelled and not self.closing:
                            messagebox.showerror(APP, data, parent=self.root)
            except queue.Empty:
                pass
            if self.closing and (not self.worker or not self.worker.is_alive()):
                if self.plan is not None:
                    self.plan.close()
                self.root.destroy()
                return
            self.root.after(100, self.poll)

        def find_executable(self, name):
            destination = Path(self.destination.get()).expanduser()
            if not any((destination / filename).is_file() for filename in ('QUEST.ezd', 'quest.ezd')):
                return None
            if name == 'studio':
                return Path(sys.executable) if getattr(sys, 'frozen', False) else BUNDLE / 'tools' / 'start_studio.py'
            candidates = [Path(os.environ['TITBIT_LINUX_GAME'])] if os.environ.get('TITBIT_LINUX_GAME') else []
            candidates += [Path(self.destination.get()).expanduser() / 'titbit', BUNDLE / 'titbit', BUNDLE / 'build' / 'titbit']
            for path in candidates:
                if path.is_file() and os.access(path, os.X_OK):
                    return path.resolve()
            return None

        def refresh_launches(self):
            running = self.task_active
            for button, name in self.launch_buttons:
                button.configure(state="normal" if not running and self.find_executable(name) else "disabled")

        def launch(self, name):
            path = self.find_executable(name)
            if path is None:
                return
            try:
                # Explicit button click only. No automatic launches after importing.
                destination = Path(self.destination.get()).expanduser().resolve()
                if name != 'studio':
                    command = [str(path)]
                elif getattr(sys, 'frozen', False):
                    command = [str(path), 'studio', '--archive', str(destination)]
                else:
                    command = [sys.executable, str(path), '--archive', str(destination)]
                env = dict(os.environ, TITBIT_GAME=str(destination))
                subprocess.Popen(command, cwd=str(destination), env=env)
            except OSError as exc:
                messagebox.showerror(APP, str(exc), parent=self.root)

        def close(self):
            if self.worker and self.worker.is_alive():
                self.closing = True
                self.request_cancel()
                self.status.set("Завершаем операцию перед закрытием…")
            else:
                if self.plan is not None:
                    self.plan.close()
                self.root.destroy()

    root = tk.Tk()
    Setup(root)
    root.mainloop()
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=APP)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--inspect", metavar="SOURCE", help="Проверить папку или образ; вывести JSON")
    mode.add_argument("--import", dest="import_source", metavar="SOURCE", help="Добавить данные из папки или образа")
    mode.add_argument("--verify", metavar="TARGET", help="Проверить подготовленную папку; вывести JSON")
    parser.add_argument("--dest", metavar="TARGET", help="Папка назначения для --import")
    parser.add_argument("--report", metavar="FILE", help="Сохранить JSON-отчёт; работает и в оконном EXE")
    args = parser.parse_args(argv)
    if args.report and not (args.inspect or args.import_source or args.verify):
        parser.error("Для --report укажите --inspect, --import или --verify")
    for stream in (sys.stdout, sys.stderr):
        if stream is not None and hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")
    if args.inspect or args.import_source or args.verify:
        return run_cli(args)
    return run_gui()


if __name__ == "__main__":
    raise SystemExit(main())
