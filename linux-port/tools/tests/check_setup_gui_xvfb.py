"""Exercise the real import GUI in an isolated xvfb-run display; never starts a game."""
import argparse
import json
import os
from pathlib import Path
import sys
import time

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True)
    parser.add_argument('--dest', required=True)
    parser.add_argument('--report', required=True)
    parser.add_argument('--screenshot')
    args = parser.parse_args()
    if not os.environ.get('TITBIT_XVFB_TEST') or not os.environ.get('DISPLAY', '').startswith(':'):
        raise RuntimeError('Run with TITBIT_XVFB_TEST=1 under xvfb-run, never on the desktop')
    import tkinter as tk
    from tkinter import messagebox
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    import titbit_setup

    started = time.monotonic()
    stage = 0
    finished = False
    result = {'display': os.environ['DISPLAY'], 'source': args.source, 'destination': args.dest, 'steps': []}
    original_tk = tk.Tk

    def widgets(parent):
        for child in parent.winfo_children():
            yield child
            yield from widgets(child)

    def factory(*a, **kw):
        root = original_tk(*a, **kw)
        def finish(error=None):
            nonlocal finished
            if finished:
                return
            finished = True
            result.update(ok=error is None, seconds=round(time.monotonic()-started, 2))
            if error:
                result['error'] = str(error)
            Path(args.report).write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
            # Invoke the wizard's actual safe-close handler, including cancel/wait.
            handler = root.protocol('WM_DELETE_WINDOW')
            root.tk.eval(handler) if handler else root.destroy()

        messagebox.showerror = lambda title, message, **kw: finish(message)
        messagebox.showinfo = lambda title, message, **kw: finish(message)

        def step():
            nonlocal stage
            if finished:
                return
            try:
                if time.monotonic()-started > 180:
                    finish('Wizard timed out at stage '+str(stage))
                    return
                all_widgets = list(widgets(root))
                entries = [w for w in all_widgets if w.winfo_class() == 'TEntry']
                buttons = {str(w.cget('text')): w for w in all_widgets if w.winfo_class() == 'TButton'}
                tree = next(w for w in all_widgets if w.winfo_class() == 'Treeview')
                labels = [str(root.getvar(w.cget('textvariable'))) if w.cget('textvariable') else str(w.cget('text'))
                          for w in all_widgets if w.winfo_class() == 'TLabel']
                ready = any('Файлы готовы.' in text for text in labels)
                if stage == 0:
                    entries[0].delete(0, 'end'); entries[0].insert(0, args.source)
                    entries[1].delete(0, 'end'); entries[1].insert(0, args.dest)
                    buttons['Проверить источник'].invoke()
                    result['steps'].append('scan invoked')
                    stage = 1
                elif stage == 1 and len(tree.get_children()) == 19 and not buttons['Добавить файлы игры'].instate(['disabled']):
                    result['files'] = [tree.item(item, 'text') for item in tree.get_children()]
                    if args.screenshot:
                        from PIL import ImageGrab
                        ImageGrab.grab().save(args.screenshot)
                    result['steps'].append('19 files previewed; import invoked')
                    buttons['Добавить файлы игры'].invoke()
                    stage = 2
                elif stage == 2 and ready:
                    result['steps'].append('import completed; verify invoked')
                    buttons['Проверить готовую папку'].invoke()
                    stage = 3
                elif stage == 3 and ready:
                    result['steps'].append('verification completed')
                    finish()
                    return
                root.after(100, step)
            except Exception as exc:
                finish(exc)
        root.after(200, step)
        return root

    tk.Tk = factory
    code = titbit_setup.run_gui()
    return 0 if result.get('ok') else code or 1


if __name__ == '__main__':
    raise SystemExit(main())
