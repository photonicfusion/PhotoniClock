
 Description
================================================================================
Tools for flashing firmware and updating EEPROM of the PhotoniClock



 Setup
================================================================================
1. Install avrdude if using Linux distribution: "apt-get install avrdude"

2. Install pyusb via pip: "pip install pyusb"
    
3. Install pyserial via pip: "pip install pyserial"

4. Install mido via pip: "pip install mido"

NOTE: If you encounter an error saying "owned by OS", you should uninstall the
existing module using apt-get and reinstall using pip. This ensures
compatibility with the python scripts provided.



 Usage
================================================================================
NOTE: Ensure all script files (*.sh) in "avrdude" directory are marked
executable if using Linux distribution: "chmod +x ./avrdude/*.sh"

The FlashProduction.py script assumes a file named "PhotoniClock.hex" exists
in the same directory if no firmware path is specified via the -f option flag.
You may optionally utilize this by copying your firmware HEX file into the same
directory as the python scripts, renaming the hex file to "PhotoniClock.hex"
and omit the -f option flag when executing FlashProduction.py.

Run "python FlashProduction.py -h" for help

**Example A** - Parse MIDI files, upload conversion to EEPROM, and flash production firmware
* python FlashProduction.py -e -d /dev/ttyUSB0 -f /path/to/firmware.hex

**Example B** - Flash production firmware
* python FlashProduction.py -d /dev/ttyUSB0 -f /path/to/firmware.hex



 Python module requirements:
================================================================================
pyusb
pyserial
mido

