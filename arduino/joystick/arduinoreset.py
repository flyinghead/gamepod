import sys
import serial

ser = serial.Serial(sys.argv[1],
                     baudrate=9600,
                     bytesize=serial.EIGHTBITS,
                     parity=serial.PARITY_NONE,
                     stopbits=serial.STOPBITS_ONE,
                     timeout=1,
                     xonxoff=0,
                     rtscts=0
                     )
ser.close()
ser.baudrate=1200
ser.open()
# Toggle DTR to reset Arduino
ser.setDTR(False)
ser.close()
