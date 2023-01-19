import os
import subprocess
from subprocess import call
from prettytable import PrettyTable
import requests
import json
import sys
EXECUTABLE = "ex4"
C_FILES = "chatServer.c"
H_FILES = "chatServer.h"   


def check_leaks():
    leaks = True
    with open("valgrind-out.txt", 'r') as valgrind_log:
        try:

            val_lines = valgrind_log.readlines()

        except UnicodeDecodeError:
            return False

        for line in val_lines:
            if "no leaks are possible" in line:
                leaks = False
        if leaks:
            print("valgrind found memory leaks, check your code for allocation/freeing errors and run the tests again")
            return True
    return False


def valgrind_test():
    print("running valgrind with full check and debug mode")

    with open("valgrind-out.txt", 'w') as out_file:
        try:
            valgrind = subprocess.run(
                "valgrind --leak-check=full --tool=memcheck --show-leak-kinds=all --track-origins=yes --verbose "
                f"--error-exitcode=1 -v --log-file=valgrind-out.txt ./{EXECUTABLE} 8070 5 2",
                stdout=out_file, text=True, shell=True, timeout=60)

        except subprocess.TimeoutExpired:
            print("Timeout occurred.")
            return False

    if check_leaks():
        return False

    return valgrind.returncode == 0

def check_init_server():
    status = subprocess.run(f"./{EXECUTABLE} ",shell=True)
    if status.returncode  != 0:
        print("[-]FAILED! test tasks bigger then number of threads")
        return False

    print("[+] Passed test tasks bigger then number of threads")
    return True

def setup():
    if os.path.isfile(EXECUTABLE):
        os.remove(EXECUTABLE)

    with open("stdout_compilation.txt", 'w') as out_file:
        c = subprocess.run(
            f'gcc -Wall {C_FILES} {H_FILES} -o {EXECUTABLE} -lpthread',
            stderr=out_file,
            stdout=out_file,
            shell=True,
        )
    with open("stdout_compilation.txt") as out_file:
        res = out_file.read()
        return_val = None
        if bytes(res, 'utf-8') == b'':
            print("Ex. compiled successfully.")
            return_val = "Compiled"

        if "warning: " in res:
            print("Warnings during compilation")
            return_val = "Warnings"

        if "error: " in res:
            print("\nSomething didn't go right when compiling your C source "
                "please check stdout_compilation.txt\n")
            return_val = "Error"

        return return_val
if __name__ == "__main__":
    compilation_status = setup()
    
    t = PrettyTable(['Test', 'Result'])
    t.align['Test'] = 'l'
    print(t)