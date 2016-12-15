#!/usr/bin/env python
import time
import colorsys

import rainbowhat


RAINBOW_BRIGHTNESS = 255

HUE_COLD = 225
HUE_HOT = 0

T_COLD = 10
T_HOT = 30

print("""Displays the temperature or pressure.

    Use buttons to change mode.
    Press Ctrl+C  exit.

""")

mode = 0

@rainbowhat.touch.A.press()
def touch_a(channel):
    global mode
    mode = 1

@rainbowhat.touch.B.press()
def touch_b(channel):
    global mode
    mode = 2

@rainbowhat.touch.C.press()
def touch_c(channel):
    global mode
    mode = 0

def set_rainbow(press, temp):
    temp = max(temp,T_COLD)
    temp = min(temp,T_HOT)

    scale = (temp - T_COLD) / float(T_HOT - T_COLD)
    hue = (HUE_COLD + scale * (HUE_HOT - HUE_COLD)) / 360.0

    r, g, b = [int(c * 100) for c in  colorsys.hsv_to_rgb(hue, 1.0, 1.0)]

    rainbowhat.rainbow.set_pixel(press, r, g, b, 0.2)
    rainbowhat.rainbow.show()
    rainbowhat.rainbow.set_pixel(press, 0, 0, 0, 0.2)

try:
    while True:
        tempAdjust = 10
        temperature = rainbowhat.weather.temperature() - tempAdjust
        pressure = rainbowhat.weather.pressure() / 100
        dial = 103 - int (pressure / 10 + 0.5)
        if dial < 0:
            dial = 0
        if dial > 6:
            dial = 6
        set_rainbow(dial, temperature)

#       need to clear leading digits
        rainbowhat.display.clear()
        if mode == 1:
            rainbowhat.display.print_float(temperature, 0)
        if mode == 2:
            rainbowhat.display.print_float(pressure, 0)
        rainbowhat.display.show()

        time.sleep(1)

except KeyboardInterrupt:
    pass


rainbowhat.display.clear()
rainbowhat.display.show()
