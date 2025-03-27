import sys
import glob
import serial
import time
import RPi.GPIO as GPIO

print("Resetting chip...")

try:
  GPIO.setmode(GPIO.BCM)
  GPIO.setwarnings(False)
  STM32_RST_PIN = 6
  GPIO.setup(STM32_RST_PIN, GPIO.OUT)  # пин RST для STM32
  GPIO.output(STM32_RST_PIN, 1)      # устанавливаем пин RST для STM32 в лог 0
  time.sleep(0.2)
  GPIO.output(STM32_RST_PIN, 0)      # устанавливаем пин RST для STM32 в лог 0
  time.sleep(0.2)
except Exception as e:
  print(e)
  sys.exit(1)

sys.exit(0)