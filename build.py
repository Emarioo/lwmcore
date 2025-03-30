#!/usr/bin/python3

import os, sys, platform

# CONFIG
INCL = "-Iinclude"
SRC = "main.c emulator.c assembler.c util.c blotter.c"
OUT = "lwm.exe" if platform.system() == "Windows" else "lwm"

# COMPILE
SRC = " ".join([ "src/" + f for f in SRC.split(" ") ])
err = os.system(f"gcc -std=c17 {INCL} {SRC} -g -o {OUT}")

# EXECUTE
if err == 0:
	# os.system(f"{OUT} examples/sample.asm -e")
	os.system(f"{OUT} examples/data.partialworld")
	# os.system(f"{OUT} examples/sample.asm --rom data.partialworld")
