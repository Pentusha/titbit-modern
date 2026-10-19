#!/usr/bin/env python3
"""Translate recovered editable RC string-table entries to a native C table."""
import pathlib
import re
import sys

source, output = map(pathlib.Path, sys.argv[1:])
entries = re.findall(r'^\s*(\d+)\s+("[^\n]*")\s*$', source.read_text(encoding="utf-8"), re.MULTILINE)
output.write_text("/* Generated from the source-owned titbit.rc string tables. */\n" +
                  "".join(f"{{{number}, {text}}},\n" for number, text in entries), encoding="utf-8")
