"""Drive the real Tk setup on an explicitly named noninteractive Windows desktop.

Never starts a game. Parent process must use STARTUPINFO.lpDesktop and pass the
exact desktop name with --desktop; this script refuses the visible desktop.
"""
import argparse
import ctypes
from ctypes import wintypes as wt
import json
import os
from pathlib import Path
import struct
import sys
import time


def assert_hidden_desktop(expected):
    user = ctypes.WinDLL('user32', use_last_error=True)
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    user.GetThreadDesktop.argtypes = [wt.DWORD]
    user.GetThreadDesktop.restype = wt.HANDLE
    user.GetUserObjectInformationW.argtypes = [wt.HANDLE, ctypes.c_int, ctypes.c_void_p, wt.DWORD, ctypes.POINTER(wt.DWORD)]
    handle = user.GetThreadDesktop(kernel.GetCurrentThreadId())
    name = ctypes.create_unicode_buffer(512)
    needed = wt.DWORD()
    if not user.GetUserObjectInformationW(handle, 2, name, ctypes.sizeof(name), ctypes.byref(needed)):
        raise OSError(ctypes.get_last_error(), 'Cannot identify desktop')
    if name.value.casefold() == 'default' or name.value != expected:
        raise RuntimeError('Refusing to open Tk on an unexpected or visible desktop: '+name.value)


def screenshot(root, path):
    user = ctypes.WinDLL('user32', use_last_error=True)
    gdi = ctypes.WinDLL('gdi32', use_last_error=True)
    user.GetAncestor.argtypes = [wt.HWND, wt.UINT]
    user.GetAncestor.restype = wt.HWND
    user.GetWindowDC.argtypes = [wt.HWND]
    user.GetWindowDC.restype = wt.HDC
    user.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
    user.PrintWindow.argtypes = [wt.HWND, wt.HDC, wt.UINT]
    user.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
    gdi.CreateCompatibleDC.argtypes = [wt.HDC]
    gdi.CreateCompatibleDC.restype = wt.HDC
    gdi.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
    gdi.CreateCompatibleBitmap.restype = wt.HBITMAP
    gdi.SelectObject.argtypes = [wt.HDC, wt.HANDLE]
    gdi.SelectObject.restype = wt.HANDLE
    gdi.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, wt.UINT, wt.UINT, ctypes.c_void_p, ctypes.c_void_p, wt.UINT]
    gdi.DeleteObject.argtypes = [wt.HANDLE]
    gdi.DeleteDC.argtypes = [wt.HDC]
    root.update_idletasks()
    hwnd = user.GetAncestor(root.winfo_id(), 2)
    rect = wt.RECT()
    if not user.GetWindowRect(hwnd, ctypes.byref(rect)):
        raise OSError(ctypes.get_last_error(), 'GetWindowRect failed')
    width, height = rect.right-rect.left, rect.bottom-rect.top
    dc = user.GetWindowDC(hwnd)
    memory = gdi.CreateCompatibleDC(dc)
    bitmap = gdi.CreateCompatibleBitmap(dc, width, height)
    old = gdi.SelectObject(memory, bitmap)
    try:
        if not user.PrintWindow(hwnd, memory, 2):
            raise OSError(ctypes.get_last_error(), 'PrintWindow failed')
        gdi.SelectObject(memory, old)
        pixels = ctypes.create_string_buffer(width*height*4)
        info = ctypes.create_string_buffer(struct.pack('<IiiHHIIiiII', 40, width, height, 1, 32, 0,
                                                       len(pixels), 0, 0, 0, 0))
        if gdi.GetDIBits(memory, bitmap, 0, height, pixels, info, 0) != height:
            raise OSError(ctypes.get_last_error(), 'GetDIBits failed')
        Path(path).write_bytes(struct.pack('<2sIHHI', b'BM', 54+len(pixels), 0, 0, 54)+info.raw[:-1]+pixels.raw)
    finally:
        gdi.SelectObject(memory, old)
        gdi.DeleteObject(bitmap)
        gdi.DeleteDC(memory)
        user.ReleaseDC(hwnd, dc)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--desktop', required=True)
    parser.add_argument('--source', required=True)
    parser.add_argument('--dest', required=True)
    parser.add_argument('--report', required=True)
    parser.add_argument('--screenshot')
    args = parser.parse_args()
    if os.name != 'nt':
        raise RuntimeError('Windows desktop isolation is required')
    assert_hidden_desktop(args.desktop)
    import tkinter as tk
    from tkinter import messagebox
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    import titbit_setup

    started = time.monotonic()
    stage = 0
    finished = False
    result = {'desktop': args.desktop, 'source': args.source, 'destination': args.dest, 'steps': []}
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
                        screenshot(root, args.screenshot)
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
