#!/usr/bin/python3

import os, sys, platform

INCL = "-Iinclude"
SRC = "src/main.c src/emulator.c src/assembler.c src/util.c"
OUT = "lwm.exe" if platform.system() == "Windows" else "lwm"

err = os.system(f"gcc -std=c17 {INCL} {SRC} -g -o {OUT}")

if err == 0:
	os.system(f"{OUT} examples/sample.asm -e")
