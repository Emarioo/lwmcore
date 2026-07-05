#!/usr/bin/env python3

import os, sys, subprocess, glob, shlex, shutil, platform

LWM = "lwm"

ROOT = os.path.dirname(os.path.dirname(__file__)).replace('\\','/')

def collect_tests():
    tests = []
    for test_dir in glob.glob(f"{ROOT}/tests/programs/*"):
        tests.append(test_dir)
    return tests

class FailException:
    def __init__(self):
        pass

def cmd(c: str, silent: bool = False):
    c = c.replace("\\", "/")
    
    # print(c)
    proc = subprocess.run(shlex.split(c), text=True, stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    # res = os.system(c)
    if proc.returncode != 0:
        raise FailException()
        # print(c)
        # exit(1)
    # proc = subprocess.run(shlex.split(c), shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    # if (proc.returncode != 0 or len(proc.stdout) > 0) and not silent:
    #     print(c)
    #     print(proc.stdout, end="")
    # return proc.returncode


def compile_native_program(output_file, files, flags):
    INT = f"{ROOT}/int"
    os.makedirs(INT, exist_ok=True)
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    # COMMON_FLAGS = f"-g -I{ROOT}/include -I{ROOT}/src"
    # EXE = f"{output_path}/barf{'.exe' if platform.system()=="Windows" else ''}"

    OBJECTS = [ INT + "/" + os.path.basename(f).replace('.c', '.o') for f in files ]

    if platform.system() == "Windows":
        flags += " -DOS_WINDOWS"
    if platform.system() == "Linux":
        flags += " -DOS_LINUX"

    for obj, src in zip(OBJECTS, files):
        cmd(f"gcc -c {flags} {src} -o {obj}")

    cmd(f"gcc {flags} -o {output_file} {ROOT}/src/platform/platform.c {' '.join(OBJECTS)}")
    
    
def compile_artifact(output_file, files, flags):
    INT = f"{ROOT}/int"
    os.makedirs(INT, exist_ok=True)
    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
    OBJECTS = [ INT + "/" + os.path.basename(f).replace('.c', '-ba.o') for f in files ]

    for obj, src in zip(OBJECTS, files):
        cmd(f"gcc -c {flags} {src} -o {obj}")
    
    cmd(f"barf -c -o {output_file} {' '.join(OBJECTS)}")

def run_test(test_dir):
    print(f"Running {test_dir}")

    # c_files = glob.glob(f"{test_dir}/*.c", recursive=True)

    name = os.path.basename(test_dir)

    with open(test_dir, "r") as f:
        text = f.read()
    
    KEYWORD = "EXPECTED:"
    start_index = text.find(KEYWORD)
    if start_index == -1:
        print(f"\033[32mFAILED\033[0m {name} could not find '{KEYWORD}")
        return False
    
    start_index += len(KEYWORD)
    
    end_index = text.find("*/", start_index)
    if start_index == -1:
        print(f"\033[32mFAILED\033[0m {name} could not find '*/'")
        return False
    expected = text[start_index:end_index].strip()
    
    proc = subprocess.run(shlex.split(f"{LWM} {test_dir} -e -q"), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    result = proc.stdout.strip()

    if result != expected:
        print("\033[31mFAILED\033[0m")
        print("--- stdout ---")
        print(result)
        print("--- expected ---")
        print(expected)
        return False

    print("\033[32mPASSED\033[0m", name)
    return True

def clean_tests(test_dirs):
    for d in test_dirs:
        INT = f"{d}/int"
        if os.path.exists(INT):
            shutil.rmtree(INT)

def main(args):
    
    tests_to_run = []
    should_clean_tests = False
    argi = 1
    while argi < len(args):
        arg = args[argi]
        argi+=1
        if arg == '-h':
            print(f"Usage:")
            print(f"  {__file__}                    Run all tests")
            print(f"  {__file__} [TEST_CASES...]    Run specific tests")
            exit(0)
        elif arg == '--clean' or arg == '-c':
            should_clean_tests = True
        elif arg[0] == '-':
            print(f"\033[31mERROR:\033[0m Unknown flag '{arg}'")
            exit(1)
        else:
            tests_to_run.append(arg)

    all_tests = collect_tests()

    if should_clean_tests:
        clean_tests(all_tests)

    tests = all_tests
    if len(tests_to_run) != 0:
        tests = [ t for t in all_tests if t in tests_to_run ]
        if len(tests) == 0:
            print("ERROR: Test case arguments matched no tests.")
            print(f"  Arguments: {tests_to_run}")
            print(f"  All tests: {all_tests}")
            exit(1)

    total_tests = len(tests)
    passed_tests = 0

    for test_dir in tests:
        test_dir = test_dir.replace('\\','/')
        try:
            res = run_test(test_dir)
            if res:
                passed_tests += 1
            
        except FailException as ex:
            pass
    
    if passed_tests == total_tests:
        print(f"\033[32mSUCCESS\033[0m {passed_tests}/{total_tests}")
    else:
        print(f"\033[31mFAILURE\033[0m {passed_tests}/{total_tests}")


if __name__ == "__main__":
    main(sys.argv)

