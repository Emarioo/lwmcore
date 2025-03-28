#!/usr/bin/python3

import os

INCL = "-Iinclude"
SRC = "src/main.c src/emulator.c src/assembler.c"
OUT = "lwm.exe"

err = os.system(f"gcc {INCL} {SRC} -g -o {OUT}")

if err == 0:
	os.system("lwm.exe")
