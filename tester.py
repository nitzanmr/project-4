import os
import subprocess
from subprocess import call
from prettytable import PrettyTable
import requests
import json
import sys
import signal

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
                f"--error-exitcode=1 -v --log-file=valgrind-out.txt ./{EXECUTABLE} 8062",
                stdout=out_file, text=True, shell=True, timeout=60)

        except subprocess.TimeoutExpired:
            print("Timeout occurred.")
            return False

    if check_leaks():
        return False

    return valgrind.returncode == -2

def check_init_server():
    status = subprocess.run(f"./{EXECUTABLE} 8067",shell=True)
    print(status.returncode)
    if status.returncode  != -2:
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

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')


if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    # print('Press Ctrl+C')
    # signal.pause()
    compilation_status = setup()
    t_check_init = check_init_server()
    t_valgrind = valgrind_test()
    t = PrettyTable(['Test', 'Result'])
    t.add_row(['init',t_check_init])
    t.add_row(['valgrind',t_valgrind])
    t.align['Test'] = 'l'
    print(t)