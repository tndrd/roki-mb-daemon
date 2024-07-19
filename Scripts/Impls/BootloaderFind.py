import sys
import glob
import serial
import time
import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
STM32_RST_PIN = 6
GPIO.setup(STM32_RST_PIN, GPIO.OUT)  # пин RST для STM32
GPIO.output(STM32_RST_PIN, 1)      # устанавливаем пин RST для STM32 в лог 0
time.sleep(0.2)
GPIO.output(STM32_RST_PIN, 0)      # устанавливаем пин RST для STM32 в лог 0

#file = open(firmware_path, "rb")  # открываем файл прошивки
Sector_Num = 11  # Кол-во секторов для стирания в STM32 на плате Motherboard
PCB_Name = "PCB_NAME = MOTHERBOARD-V.1.0\r\n"  # Ответ от платы
# Ответ от платы о окончании процесса стирания флэш памяти
Full_Erase = "FULL ERASE OK\r\n"
Acknowledge = "OK\r\n"  # Ответ подтверждения об успешности операции


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

    # Проверяем на каком COM порту сидит наша плата
    for COM_Index in range(len(COM_List)):
        ser = serial.Serial(COM_List[COM_Index],
                            921600,  timeout=0.5, write_timeout=0.5)
        try:
            ser.write("CMD:GET_PCB_NAME\r\n".encode('ascii'))
            response = ser.readline().decode(encoding='UTF-8')
        except:
            continue
        if (response == PCB_Name):
            mb_port = COM_List[COM_Index]
            print(mb_port, end="")
            break
        elif (COM_Index >= (len(COM_List) - 1)):
            sys.exit(1)

    sys.exit(0)
