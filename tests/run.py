#!/usr/bin/env python3

import os, sys, subprocess, glob, shlex, shutil, platform, re

LWM = "bin/lwm"

ROOT = os.path.dirname(os.path.dirname(__file__)).replace('\\','/')

TEST_CONFIGS = {
    "all": [
        "tests/configs/lwm16.cfg",
        "tests/configs/lwm32.cfg",
        "tests/configs/lwm64.cfg",
    ],
    "min_lwm32": [
        "tests/configs/lwm32.cfg",
        "tests/configs/lwm64.cfg",
    ],
    "min_lwm64": [
        "tests/configs/lwm64.cfg",
    ],
}


def collect_tests():
    tests = []
    for test_dir in glob.glob(f"{ROOT}/tests/instructions/*"):
        if not os.path.basename(test_dir).startswith("_"):
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


# Unused at the moment
def run_stdout_test(test_dir):
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


def run_test(test_dir):
    # print(f"Running {test_dir}")

    # c_files = glob.glob(f"{test_dir}/*.c", recursive=True)

    name = os.path.basename(test_dir)

    with open(test_dir, "r") as f:
        source_text = f.read()

    m = re.search(r'TEST_CONFIG = ([\w\d_]+)', source_text)
    if m is None or len(m.groups()) <= 0:
        print(f"\033[31mFAILED\033[0m {test_dir} is missing TEST_CONFIG")
        return False
    
    testConfig = m.groups()[0]
    if testConfig not in TEST_CONFIGS:
        print(f"\033[31mFAILED\033[0m Test config '{m.groups()[0]}' could not be found.")
        return False
    
    anyFailure = False

    configs = TEST_CONFIGS[testConfig]
    for platformConfig in configs:
        basePlatformConfig = os.path.basename(platformConfig)
        if not os.path.exists(platformConfig):
            print(f"\033[31mFAILED\033[0m {platformConfig} ({m.groups()[0]}) could not be found.")
            return False
        proc = subprocess.run(shlex.split(f"{LWM} {test_dir} -e -q -p {platformConfig}"), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        out_text = proc.stdout.strip()

        coverage = re.search(r'Test coverage: (\d+)/(\d+)', out_text)
        result = re.search(r'Test result: (\d+)/(\d+)', out_text)

        failed = coverage is None or result is None or len(coverage.groups()) != 2 or len(result.groups()) != 2
        if not failed:
            failed = int(coverage.groups()[0]) != int(coverage.groups()[1]) or int(result.groups()[0]) != int(result.groups()[1])

        if failed:
            print(f"\033[31mFAILED\033[0m {name} {basePlatformConfig}")
            print("--- stdout ---")
            print(out_text)
            anyFailure = True
            continue
        
        print(f"\033[32mPASSED\033[0m {name} {basePlatformConfig}")
        verbose = False
        if verbose:
            print("--- stdout ---")
            print(out_text)

    return not anyFailure

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
        tests = [ t for t in all_tests if any([ar for ar in tests_to_run if ar in t]) ]
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

