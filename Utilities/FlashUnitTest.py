#!/usr/bin/python

from __future__ import print_function
import sys
import signal
import time
import os
import argparse
import platform

__path__ = os.path.dirname(os.path.realpath(__file__))

def main():

    if not platform.system() == 'Windows':
        if not os.geteuid()==0:
            error_message('This script must be run as root.')
        if which('avrdude') == None:
            error_message('Could not locate avrdude. Please install avrdude.\nOn Debian-based distros: sudo apt-get install avrdude')
            
    set_file_permissions()

    parser = argparse.ArgumentParser(description='PhotoniClock bootloader flash utility.')
    args = parser.parse_args()
    start_time = time.time()
        
    try:
        print_message("Flashing Bootloader")
        if not flash_bootloader():
            print_message("Flashing Unit Test")
            if flash_unit_test():
                error_message("Failed to flash Unit Test")
        else:
            error_message("Failed to flash bootloader")
    except:
        error_message("Process could not complete")
    
    elapsed_time = time.time() - start_time
    print("Completed in", "%.2f" % elapsed_time, "seconds\n")
    
def set_file_permissions():
    if not platform.system() == 'Windows':
        os.chmod(__path__ + "/avrdude/flash.sh", 0o744)
        os.chmod(__path__ + "/avrdude/bootloader.sh", 0o744)

def which(cmd):
    path = os.environ.get("PATH", os.defpath)
    if not path:
        return None
    
    path = path.split(os.pathsep)
    files = [cmd]

    seen = set()
    for dir in path:
        normdir = os.path.normcase(dir)
        if normdir not in seen:
            seen.add(normdir)
            for thefile in files:
                name = os.path.join(dir, thefile)
                if os.path.exists(name) and not os.path.isdir(name):
                    return name
    return None

def flash_bootloader():
    if platform.system() == 'Windows':
        return flash([__path__ + "\\avrdude\\bootloader.bat"])
    else:
        return flash([__path__ + "/avrdude/bootloader.sh"])

def flash_unit_test():
    if platform.system() == 'Windows':
        return flash([__path__ + "\\avrdude\\flash.bat", __path__ + "\\avrdude\\hex\\unittest.hex", "USBTiny"])
    else:
        return flash([__path__ + "/avrdude/flash.sh", __path__ + "/avrdude/hex/unittest.hex", "USBTiny"])

def flash(command):
    import subprocess as sp
    child = sp.Popen(command)
    child.communicate()[0]
    child.wait()
    return child.returncode
    
def print_message(message):
    print("\n=============================================================")
    print(" ", message)
    print("=============================================================\n")

def error_message(message):
    print("ERROR:", message, "\n")
    sys.exit(-1)
    
if __name__ == "__main__":
    main()
