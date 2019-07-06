Instructions:
--------------------------------------------------------------------------------
Replace Arduino's default HardwareSerial files with the ones provided in this
folder. On Windows, the Arduino directory to look for is:
%LocalAppData%\Arduino15\packages\arduino\hardware\avr\1.6.20\cores\arduino
-or-
%ProgramFiles(x86)%\Arduino\hardware\arduino\avr\cores\arduino

Description:
--------------------------------------------------------------------------------
Arduino's default HardwareSerial library does not support interrupt callback
functions. These files have been modified to provide callback support on UART
receive via an onReceive() function.

To use, simply call Serial.onReceive(<function>) where <function> is the name
of the function to be called immediately after a byte has been received in the
UART buffer. Note that since this is interrupt driven, all system interrupts
are disabled by default when the callback is invoked. This means methods that
depend on timers or interrupts to function such as delay() or the Wire library
will not work within the scope of the UART callback unless interrupts are first
re-enabled using the sei() command.
