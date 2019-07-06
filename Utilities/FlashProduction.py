#!/usr/bin/python

from __future__ import print_function
import sys
import time
import os
import argparse
import platform
import uuid

__path__ = os.path.dirname(os.path.realpath(__file__))

def main():

    if not platform.system() == 'Windows':
        if not os.geteuid()==0:
            error_message('This script must be run as root.')
        if which('avrdude') == None:
            error_message('Could not locate avrdude. Please install avrdude.\nOn Debian-based distros: sudo apt-get install avrdude')

    parser = argparse.ArgumentParser(description='PhotoniClock firmware and EEPROM flash utility.')
    parser.add_argument('-d', '--device', required=True, type=str, help='The UART device to use. Ex: COM3, /dev/ttyUSB1', default=None)
    parser.add_argument('-e', '--eeprom', action="store_true", help='Specify if EEPROM should be flashed with MIDI', default=False)
    parser.add_argument('-f', '--firmware', type=str, help='Firmware HEX file', default=__path__+"/PhotoniClock.hex")
    parser.add_argument('-m', '--midi', type=str, help='MIDI directory', default=__path__+"/MIDI")
    parser.add_argument('-v', '--verbosity', action='count', help='Each use increases verbosity level', default=0)

    args = parser.parse_args()
    verbose = args.verbosity
    verbosity = ""
    device = ""

    check_prerequisites()
    set_file_permissions()
    
    if not os.path.exists(args.firmware):
        error_message("Failed to open Firmware: " + args.firmware)
    
    if not os.path.exists(args.midi):
        error_message("Failed to open MIDI: " + args.midi)
    
    if verbose > 0:
        print(args)
        verbosity = " -"
        for v in range(0, verbose):
            verbosity += "v"

    start_time = time.time()
    
    if args.device and args.device != "USBTiny":
        device = " -d " + args.device
    
    try:
        if not args.eeprom:
            print_message("Flashing production firmware to target...")
            if not flash_hex(args.firmware, args.device):
                print_message("Production complete")
            else:
                error_message("Flashing production firmware")
        else:
            try:
                print_message("Generating MIDI conversion JSON...")
                unique_filename = str(uuid.uuid4())
                if not os.system("python " + __path__ + "/midi2notes.py -O -j -o " + unique_filename + ".json " + args.midi + "/*.mid" + verbosity):
                    print_message("Flashing preload firmware to target...")
                    if not flash_hex(__path__ + "/avrdude/hex/preload.hex", args.device):
                        print_message("Transferring JSON to on-board EEPROM...")
                        if not os.system("python " + __path__ + "/notes2eeprom.py -p 64 -a 256 -s 32768 " + unique_filename + ".json" + device + verbosity):
                            os.remove(unique_filename + ".json")
                            print_message("Flashing production firmware to target...")
                            if not flash_hex(args.firmware, args.device):
                                print_message("Production complete")
                            else:
                                error_message("Flashing production firmware")
                        else:
                            error_message("EEPROM transfer")
                    else:
                        error_message("Flashing preload firmware")
                else:
                    error_message("MIDI conversion")
            except:
                os.remove(unique_filename + ".json")
                error_message("Terminating...")
    except:
        error_message("Process could not complete")
        
    elapsed_time = time.time() - start_time
    print("Completed in", "%.2f" % elapsed_time, "seconds\n")

def check_prerequisites():
    is_missing_usb = False
    is_missing_midi = False
    is_missing_serial = False

    try:
        import usb.core
    except ImportError:
        is_missing_usb = True

    try:
        import mido
    except ImportError:
        is_missing_midi = True

    try:
        import serial
    except ImportError:
        is_missing_serial = True
    
    if is_missing_usb or is_missing_midi or is_missing_serial:
        message = "Missing prerequisite, aborting...\n"
        message += "Please run:\n\t"
        if not platform.system() == 'Windows':
            message += "sudo "
        message += "pip install "        
        message += ("", "pyusb ")[is_missing_usb]
        message += ("", "mido ")[is_missing_midi]
        message += ("", "pyserial ")[is_missing_serial]
        
        error_message(message)
        
def set_file_permissions():
    if not platform.system() == 'Windows':
        os.chmod(__path__ + "/avrdude/flash.sh", 0o744)
        os.chmod(__path__ + "/avrdude/reset.sh", 0o744)
        
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

def flash_hex(hex, device):
    if platform.system() == 'Windows':
        if device:
            return flash([__path__ + "\\avrdude\\flash.bat", hex, device])
        else:
            return flash([__path__ + "\\avrdude\\flash.bat", hex])
        
    else:
        return flash([__path__ + "/avrdude/flash.sh", hex, device])
    
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
    try:
        os.remove(unique_filename + ".json")
    except:
        pass
    sys.exit(-1)

if __name__ == "__main__":
    main()
