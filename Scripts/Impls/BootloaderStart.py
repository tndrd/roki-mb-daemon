import sys
import glob
import serial
import RPi.GPIO as GPIO
import time

Motherboard_PCB_Name = "PCB_NAME = MOTHERBOARD-V.1.0\r\n"  #Ответ от платы Motherboard
BlueCoin_PCB_Name    = "PCB_NAME = BlueCoin-V.1.0\r\n"     #Ответ от платы BlueCoin

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
STM32_RST_PIN = 6
GPIO.setup(STM32_RST_PIN,GPIO.OUT) # пин RST для STM32
GPIO.output(STM32_RST_PIN, 0)    # устанавливаем пин RST для STM32 в лог 0
GPIO.output(STM32_RST_PIN, 1)    # устанавливаем пин RST для STM32 в лог 0
time.sleep(0.2)
GPIO.output(STM32_RST_PIN, 0)    # устанавливаем пин RST для STM32 в лог 0


def serial_ports():
    """
    Функция возвращает список, имеющихся COM портов в системе
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

if __name__ == '__main__':
    
    time.sleep(0.2)
    
    COM_List = serial_ports()
    COM_Motherboard = None
    print("List of available serial ports: ", COM_List)    
    # Проверяем на каком COM порту сидит наша плата
    for COM_Index in range(len(COM_List)):
        try:
            print(f"Asking {COM_List[COM_Index]}...")
            ser = serial.Serial(COM_List[COM_Index], 921600, timeout = 0.5, write_timeout=0.5)
            ser.write("CMD:GET_PCB_NAME\r\n".encode('ascii'))
            response = ser.readline().decode(encoding = 'UTF-8')        
        except:
            #print("except")
            continue
        if (response == Motherboard_PCB_Name):
            print(response[0:len(response) - 4], "is opened on", COM_List[COM_Index], "device")          
            COM_Motherboard = serial.Serial(COM_List[COM_Index], 921600,  timeout = 0.5)
    
    if (COM_Motherboard is None):
      print("Motherboard not found")
      sys.exit(1)

    COM_Motherboard.write("CMD:GET_CONNECTION_STATE\r\n".encode('ascii'))
    response = COM_Motherboard.readline().decode(encoding = 'UTF-8')
    print("Motherboard_PCB", response)
    COM_Motherboard.write("CMD:START_FW\r\n".encode('ascii'))
    response = COM_Motherboard.readline().decode(encoding = 'UTF-8')
    print("Motherboard: ", response)

    sys.exit(0)
