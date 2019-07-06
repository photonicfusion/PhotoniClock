#!/usr/bin/python

from __future__ import print_function
import sys
import signal
import time
import array
import random
import json
import argparse
import struct
import usb.core
import usb.util
import binascii
import subprocess
import os
import serial
import platform

__path__ = os.path.dirname(os.path.realpath(__file__))

TRANSMIT_RETRY_COUNT=3

POLL_SECTION_INDICATOR=0xFC
POLL_STATUS_INDICATOR=0xFD
SECTION_INDICATOR=0xFE
ESCAPE_INDICATOR=0xFF
TABLE_ADDRESS_SIZE=2
NUMBER_OF_CHANNELS=2
BUFFER_SIZE=300

STATUS_ERROR=0
STATUS_OK=1
STATUS_BUSY=2
STATUS_RETRY=3
STATUS_COMPLETE=4

crc_table = bytearray(256)
ser = serial.Serial()

def main():
    global verbose
    global ser

    parser = argparse.ArgumentParser(description='Write JSON file to on-board EEPROM using SPI or UART.')
    parser.add_argument('file', metavar='file', type=str, help='a JSON file to read from')
    parser.add_argument('-d', '--device', type=str, help='The UART device to use. Ex: COM3, /dev/ttyUSB1', default=None)
    parser.add_argument('-p', '--pagesize', type=int, help='Page size of EEPROM', default=32)
    parser.add_argument('-a', '--address_start', type=int, help='EEPROM memory address to write song values', default=0x100)
    parser.add_argument('-s', '--size_memory', type=int, help='Size of EEPROM in bytes', default=0x2000)
    parser.add_argument('-v', '--verbosity', action='count', default=0, help='Each use increases verbosity level')

    args = parser.parse_args()
    verbose = args.verbosity
    
    if verbose > 0:
        print(args)
    
    if args.address_start % 2:
        error_msg_exit("address_start must be multiple of 2: " + str(args.address_start))
        
    if args.address_start >= args.size_memory:
        error_msg_exit("address_start cannot be >= size_memory: " + str(args.address_start) + " / " + str(args.size_memory))

    if args.pagesize > args.size_memory:
        error_msg_exit("pagesize cannot be larger than size_memory: " + str(args.pagesize) + " / " + str(args.size_memory))
    
    try:
        file = open(args.file)
    except:
        error_msg_exit("Failed to open file: " + args.file)
    
    try:
        data = json.load(file)
    except:
        error_msg_exit("Failed to parse file: " + args.file)

    if not is_power2(args.pagesize):
        warning_msg("pagesize is not a power of two: " + str(args.pagesize))
        
    if args.address_start > (args.size_memory * 0.1):
        percent = 100 - (100 * args.address_start / args.size_memory)
        remaining = args.size_memory - args.address_start
        warning_msg("Only " + str(percent) + "% memory available with given address_start: " + str(remaining) + " / " + str(args.size_memory))
    
    data = process_data(data, args.address_start, args.size_memory)

    start_time = time.time()

    # Assume failure
    status = -1
    count = 0
    
    while True:
        count += 1
        
        if args.device:
            try:
                ser = serial.Serial(port=args.device, baudrate=115200, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, timeout=None, write_timeout=None)
            except:
                error_msg_exit("Could not open serial port: " + args.device + "\nCheck device permissions or run with sudo")
            if not ser.isOpen():
                error_msg_exit("Could not open serial port: " + args.device + "\nCheck device permissions or run with sudo")
            print("Using UART via FTDI cable")
        else:
            print("Using SPI via USBtinyISP")
        
        if args.device:
            status = transmit_UART(data, args.pagesize)
            ser.close()
        else:
            status = transmit_SPI(data, args.pagesize)
            
        # Break if successful
        if status == 0:
            break
            
        if count > TRANSMIT_RETRY_COUNT:
            error_msg_exit("Unable to flash EEPROM")

    elapsed_time = time.time() - start_time
    print("Completed in", "%.2f" % elapsed_time, "seconds\n")    
    time.sleep(1)
    
def process_data(data, address_start, size_memory):
    list = []
    table = bytearray(address_start)
    values = bytearray()
    offset = address_start
    entries = 0
    sections = 0
    
    for index in range(0, len(data)):
        list.append(data[index]['Filename'])
        
        if verbose > 0:
            print("")
        print("Parsing:", list[index])
        
        for c in range(0, NUMBER_OF_CHANNELS):
            try:
                # Read channel data
                channel = data[index]["Channel_" + str(chr(c + ord('A')))]
            except:
                if c > 0:
                    if verbose > 0:
                        warning_msg("Cannot find Channel_" + str(chr(c + ord('A'))))
                        warning_msg("Copying previous channel offset for Channel_" + str(chr(c + ord('A'))))
                    # Copy previous offset into table
                    for b in range(0, TABLE_ADDRESS_SIZE):
                        table[(TABLE_ADDRESS_SIZE * (entries + 1)) + b] = table[(TABLE_ADDRESS_SIZE * entries) + b]
                    entries += 1
                    continue
                else:
                    error_msg_exit("Cannot find Channel_A")
            
            # Table section
            # Get individual bytes
            offset_bytes = struct.unpack("4B", struct.pack("I", offset))
            if verbose > 1:
                print("Channel_" + str(chr(c + ord('A'))), "table offset is:", offset)
            if verbose > 0:
                print("Entry:", entries, "- Read", len(channel), "bytes from Channel_" + str(chr(c + ord('A'))))
            # Adjust offset by number of channel bytes
            offset += len(channel)
            if offset > size_memory:
                error_msg_exit("Max bytes has been exceeded: " + str(offset) + " / " + str(size_memory))
            entries += 1
            # Leave room for song count and ending offset
            if ((entries + 1) * TABLE_ADDRESS_SIZE) >= address_start:
                error_msg_exit("Max entries has been exceeded: " + str(entries) + " / " + str((address_start - (TABLE_ADDRESS_SIZE * 2)) / TABLE_ADDRESS_SIZE))
            # Copy offset into table
            for b in range(0, TABLE_ADDRESS_SIZE):
                table[(TABLE_ADDRESS_SIZE * entries) + b] = offset_bytes[b]
            
            # Values section
            split_channel = bytearray()
            count=0
            for chunk in chunks(channel, BUFFER_SIZE - 4):
                # Calculate channel CRC
                crc = calc_crc(chunk)
                if verbose > 1:
                    print("Section", sections + 1, "- Channel_" + str(chr(c + ord('A'))), "chunk", count, "(", len(chunk), "bytes ) CRC is:", hex(crc))
                chunk.append(crc)
                # Add escape characters and extend to channel
                split_channel.extend(escape_array(chunk))
                split_channel.append(SECTION_INDICATOR)
                sections += 1
                count += 1

            # Add channel to values
            values.extend(split_channel)
    
    entries += 1
    # Get individual bytes
    offset_bytes = struct.unpack("4B", struct.pack("I", offset))
    # Copy offset into table
    for b in range(0, TABLE_ADDRESS_SIZE):
        table[(TABLE_ADDRESS_SIZE * entries) + b] = offset_bytes[b]
        
    # Get individual bytes
    song_count = struct.unpack("4B", struct.pack("I", int(entries / NUMBER_OF_CHANNELS)))
    # Copy song count into start of table
    for b in range(0, TABLE_ADDRESS_SIZE):
        table[b] = song_count[b]
    
    # Calculate table CRC
    crc = calc_crc(table)
    if verbose > 1:
        print("\nSection", 0, "- Table CRC is:", hex(crc))
    table.append(crc)
    
    # Increment now (add indicator after escaping)
    sections += 1
    
    # Insert number of sections as first two bytes    
    # Get individual bytes
    section_bytes = struct.unpack("4B", struct.pack("I", sections))
    
    # Copy section count into start of table
    # Section count is for tracking and is not written to EEPROM
    for b in range(0, TABLE_ADDRESS_SIZE):
        table.insert(0, section_bytes[TABLE_ADDRESS_SIZE - b - 1])

    # Add escape characters
    table = escape_array(table)

    # Add table section indicator
    table.append(SECTION_INDICATOR)

    # Append values to table
    table.extend(values)
    
    print("\nParsed", len(data), "songs (", sections, "sections )")
    print("Table bytes:", address_start, "  Song bytes:", offset - address_start, "  Total bytes:", offset, "\n")
    return table
    
def transmit_UART(data, pagesize):
    global ser
    
    bytes = 0
    previous_byte = 0
    section = bytearray()
    sections = []
    device = None
    
    print("Resetting target...")
    time.sleep(3) # Wait for device to leave reset
    
    for b in data:
        section.append(b)
        if (b != SECTION_INDICATOR and b != ESCAPE_INDICATOR) or (previous_byte == ESCAPE_INDICATOR):
            bytes += 1
        if b == SECTION_INDICATOR and previous_byte != ESCAPE_INDICATOR:
            sections.append(section)
            section = bytearray()
        previous_byte = b
    
    print("Writing to EEPROM...")

    # Begin by polling device
    device_status = get_device_status_UART(device)
    
    # Wait until device is ready
    retry = 0
    while device_status != STATUS_OK:
        retry += 1
        if retry > 10:
            return error_msg("Cannot communicate with device")
        device_status = get_device_status_UART(device)
        
    device_section = 0
    
    while device_section < len(sections):

        if verbose > 1:
            print("Writing section:", device_section)
            if verbose > 2:
                for h in sections[device_section]:
                    sys.stdout.write(hex(h) + ' ')
            sys.stdout.write('\n\n')
            sys.stdout.flush()

        # Write data out to device
        ser.write(sections[device_section])
        time.sleep(0.15); # This is a well tuned & tested value
        # Check if device is ready for next section
        device_status = get_device_status_UART(device)
        
        while device_status != STATUS_OK and device_status != STATUS_COMPLETE:
            
            if device_status == STATUS_RETRY:
                device_section = get_device_section_UART(device)
                warning_msg("Retrying section " + str(device_section))
                ser.write(sections[device_section])
                time.sleep(0.1)
                
            if device_status == STATUS_ERROR:
                return error_msg("Failed on section " + str(device_section))
                
            if device_status == STATUS_BUSY:
                warning_msg("Waiting...")
                time.sleep(0.1)
            
            device_status = get_device_status_UART(device)
        device_section = get_device_section_UART(device)

    retry = 0
    while device_status != STATUS_COMPLETE:
        retry += 1
        if retry > 50:
            return error_msg("Incomplete transfer")
        device_status = get_device_status_UART(device)
    
    # Subtract CRC and section count (first two bytes)
    print("\nTransferred", device_section, "sections (", bytes - device_section - 2, "bytes )")
    return 0 # success
    
def get_device_status_UART(device):
    global ser
    device_status = 0
    timeout = time.time() + 1 #timeout one second from now
    
    # Get device status
    byte = bytearray([POLL_STATUS_INDICATOR])
    time.sleep(0.01) # This is a well tuned & tested value
    ser.write(byte)
    
    while True:
        if ser.in_waiting:
            device_status = ord(ser.read())
            ser.reset_input_buffer()
            break
        if time.time() > timeout:
            break;

    print_device_status(device_status)
    return device_status
    
def get_device_section_UART(device):
    global ser
    device_section = 0
    timeout = time.time() + 1 #timeout one second from now

    # Get device section
    byte = bytearray([POLL_SECTION_INDICATOR])
    time.sleep(0.01) # This is a well tuned & tested value
    ser.write(byte)
    
    while True:
        if ser.in_waiting:
            device_section = ord(ser.read())
            ser.reset_input_buffer()
            break
        if time.time() > timeout:
            break;
    
    print_device_section(device_section)
    return device_section
    
def transmit_SPI(data, pagesize):
    print("Resetting target...")
    try:
        device=eeprom()
        
        if platform.system() == 'Windows':
            reset_target = [__path__ + "\\avrdude\\reset.bat"]
        else:
            reset_target = [__path__ + "/avrdude/reset.sh"]
        
        subprocess.Popen(reset_target, shell=False)
        time.sleep(4)
        print("Writing to EEPROM...")
        return transmit_helper_SPI(data, pagesize)
    except FormatException as e:
        device=eeprom()
        device.power_off()
        return error_msg(e)
    except IOError as e:
        print(e)
        device=eeprom()
        device.power_off()
        return error_msg(e)
    except KeyboardInterrupt:
        device=eeprom()
        device.power_off()
        return error_msg("User interrupted")
    
def transmit_helper_SPI(data, pagesize):
    device=eeprom()
    device.power_on()
    time.sleep(0.1)

    bytes = 0
    previous_byte = 0
    section = bytearray()
    sections = []
        
    for b in data:
        section.append(b)
        if (b != SECTION_INDICATOR and b != ESCAPE_INDICATOR) or (previous_byte == ESCAPE_INDICATOR):
            bytes += 1
        if b == SECTION_INDICATOR and previous_byte != ESCAPE_INDICATOR:
            sections.append(section)
            section = bytearray()
        previous_byte = b
    
    # Begin by polling device
    device_status = get_device_status_SPI(device)
    
    # Wait until device is ready
    retry = 0
    while device_status != STATUS_OK:
        retry += 1
        if retry > 50:
            return error_msg("Cannot communicate with device")
        device_status = get_device_status_SPI(device)
        
    device_section = get_device_section_SPI(device)
        
    while device_section < len(sections):

        if verbose > 1:
            print("Writing section:", device_section)
            if verbose > 2:
                for h in sections[device_section]:
                    sys.stdout.write(hex(h) + ' ')
            sys.stdout.write('\n\n')
            sys.stdout.flush()

        device.write_block(sections[device_section])
        time.sleep(0.04) # This is a well tuned & tested value
        device_status = get_device_status_SPI(device)
        
        while device_status != STATUS_OK and device_status != STATUS_COMPLETE:
            
            if device_status == STATUS_RETRY:
                device_section = get_device_section_SPI(device)
                warning_msg("Retrying section " + str(device_section))
                device.write_block(sections[device_section])
                time.sleep(0.1)
                
            if device_status == STATUS_ERROR:
                return error_msg("Failed on section " + str(device_section))
                
            if device_status == STATUS_BUSY:
                warning_msg("Waiting...")
                time.sleep(0.1)
            
            device_status = get_device_status_SPI(device)
        device_section = get_device_section_SPI(device)

    retry = 0
    while device_status != STATUS_COMPLETE:
        retry += 1
        if retry > 50:
            return error_msg("Incomplete transfer")
        device_status = get_device_status_SPI(device)
    
    # Subtract CRC and section count (first two bytes)
    print("\nTransferred", device_section, "sections (", bytes - device_section - 2, "bytes )")
    device.power_off()
    return 0 # success
    
def get_device_status_SPI(device):
    # Get device status
    time.sleep(0.04) # This is a well tuned & tested value
    device_status = device.read_byte(POLL_STATUS_INDICATOR)
    time.sleep(0.04) # This is a well tuned & tested value
    device_status = device.read_byte(POLL_STATUS_INDICATOR)
    
    print_device_status(device_status)
    return device_status
    
def get_device_section_SPI(device):
    # Get device section
    time.sleep(0.04) # This is a well tuned & tested value
    device_section = device.read_byte(POLL_SECTION_INDICATOR)
    time.sleep(0.04) # This is a well tuned & tested value
    device_section = device.read_byte(POLL_SECTION_INDICATOR)

    print_device_section(device_section)
    return device_section
    
def print_device_status(device_status):
    if verbose > 1:
        if device_status == STATUS_OK:
            status = "OK"
        elif device_status == STATUS_BUSY:
            status = "BUSY"
        elif device_status == STATUS_RETRY:
            status = "RETRY"
        elif device_status == STATUS_ERROR:
            status = "ERROR"
        elif device_status == STATUS_COMPLETE:
            status = "COMPLETE"
        else:
            status = "UNKNOWN"
        print("Device status:", status, "(", hex(device_status), ")")
    
def print_device_section(device_section):
    if verbose > 2:
        print("Device section:", device_section)
    
def error_msg_exit(message):
    error_msg(message)
    sys.exit(-1)

def error_msg(message):
    print("\nERROR:", message)
    return -1
    
def warning_msg(message):
    print("\nWARNING:", message)
    
def is_power2(num):
	return num != 0 and ((num & (num - 1)) == 0)

def escape_array(array):
    copy = bytearray()
    for b in array:
        if b == ESCAPE_INDICATOR or b == SECTION_INDICATOR or b == POLL_STATUS_INDICATOR or b == POLL_SECTION_INDICATOR:
            copy.append(ESCAPE_INDICATOR)
        copy.append(b)
    array = copy[:]
    return array
    
def chunks(l, n):
    """Yield successive n-sized chunks from l."""
    for i in range(0, len(l), n):
        yield l[i:i + n]
    
def crc_precompute_table():
    global crc_table
    polynomial = 0x1D # Maximize Hamming distance
    for dividend in range(0, 256):
        remainder = dividend
        for bit in range(0, 8):
            if ((remainder & 0x01) != 0):
                remainder = (remainder >> 1) ^ polynomial
            else:
                remainder >>= 1
        crc_table[dividend] = remainder

def calc_crc(list):
    crc = 0
    for byte in list:
        crc = (crc_table[(byte ^ crc) & 0xFF] ^ (crc << 8) & 0xFF)
    return crc

class usbtiny:

  def __init__(self):
    self.USBTINY_ECHO = 0          #echo test
    self.USBTINY_READ = 1          #read port B pins
    self.USBTINY_WRITE = 2         #write byte to port B
    self.USBTINY_CLR = 3           #clear PORTB bit, value=bit number (0..7)
    self.USBTINY_SET = 4           #set PORTB bit, value=bit number (0..7)
    self.USBTINY_POWERUP = 5       #apply power and enable buffers, value=sck-period, index=RESET
    self.USBTINY_POWERDOWN = 6     #remove power from chip, disable buffers
    self.USBTINY_SPI = 7           #spi command, value=c1c0, index=c3c2
    self.USBTINY_POLL_BYTES = 8    #set poll bytes for write, value=p1p2
    self.USBTINY_FLASH_READ = 9    #read flash, index=address, USB_IN reads data
    self.USBTINY_FLASH_WRITE = 10  #write flash, index=address,value=timeout, USB_OUT writes data
    self.USBTINY_EEPROM_READ = 11  #read eeprom, index=address, USB_IN reads data
    self.USBTINY_EEPROM_WRITE = 12 #write eeprom, index=address,value=timeout, USB_OUT writes data
    self.USBTINY_DDRWRITE = 13     #set port direction, value=DDRB register value
    self.USBTINY_SPI1 = 14         #single byte SPI command, value=command
    # these values came from avrdude (http://www.nongnu.org/avrdude/)
    self.USBTINY_RESET_LOW = 0     #for POWERUP command
    self.USBTINY_RESET_HIGH = 1    #for POWERUP command
    self.USBTINY_SCK_MIN = 1       #min sck-period for POWERUP
    self.USBTINY_SCK_MAX = 250     #max sck-period for POWERUP
    self.USBTINY_SCK_DEFAULT = 10   #default sck-period to use for POWERUP
    self.USBTINY_CHUNK_SIZE = 128
    self.USBTINY_USB_TIMEOUT = 500 #timeout value for writes
    # search for usbtiny
    self.dev=usb.core.find(idVendor=0x1781,idProduct=0x0c9f)
    if self.dev==None:
      error_msg_exit("USBtiny programmer not connected")
      sys.exit(1)
    self.dev.set_configuration()
    return

  def _usb_control(self,req,val,index,retlen=0):
    return self.dev.ctrl_transfer(usb.util.CTRL_IN|usb.util.CTRL_RECIPIENT_DEVICE|usb.util.CTRL_TYPE_VENDOR,req,val,index,retlen)
    
  def power_on(self):
    self._usb_control(self.USBTINY_POWERUP, self.USBTINY_SCK_DEFAULT, self.USBTINY_RESET_HIGH )

  def power_off(self):
    self._usb_control(self.USBTINY_POWERDOWN,0,0)

  def write(self,portbbits):
    self._usb_control(self.USBTINY_WRITE,portbbits,0)
    
  def read(self):
    return self._usb_control(self.USBTINY_READ,0,0,1)
  
  def spi1(self,b):
    return self._usb_control(self.USBTINY_SPI1,b,0,1)
  
  def spi4(self,d1d0,d3d2):
    return self._usb_control(self.USBTINY_SPI,d1d0,d3d2,4)
    
  def clr(self,bit):
    self._usb_control(self.USBTINY_CLR,bit,0)

  def set(self,bit):
    self._usb_control(self.USBTINY_SET,bit,0)

class eeprom:

  def __init__(self):
    self.dev=usbtiny()
    self.CS_BIT=4
    self.SI_BIT=5
    self.SO_BIT=6
    self.CLK_BIT=7
    
  def power_on(self):
    self.dev.power_on()
  
  def power_off(self):
    self.dev.power_off()
    
  def cs_low(self):
    self.dev.clr(self.CS_BIT)

  def cs_high(self):
    self.dev.set(self.CS_BIT)
    
  def spi1(self,c):
    d=self.dev.spi1(c)
    return d[0]

  def write_byte(self,b):
    self.spi1(b)
  
  def read_byte(self,b):
    return self.spi1(b)

  def write_block(self,data):
    for c in data:
      self.write_byte(c)
    self.cs_high()
  
  def read_block(self,count):
    d=[]
    while count:
      d.append(self.read_byte())
      count=count-1
    self.cs_high()
    return d

class FormatException(Exception):
  def __init__(self, value):
     self.value = value
  def __str__(self):
     return str(self.value)

     
crc_precompute_table()
if __name__ == "__main__":
    main()
