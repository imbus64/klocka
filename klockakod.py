########################################
#                                      #
#          KLOCKA STYRNING             #
#        RASPBERRY PI ZERO 2W          #
#                                      #
#         HW: Melvin Olsson            #
#        SW: William Andersson         #
#                                      #
#           Version: 2.2               #
#            2024-11-02                #
########################################

######## BIBLIOTEK ########
import gpiozero  # Installera med: sudo apt install python3-gpiozero
from gpiozero import Button
import rpi_ws281x  # Installera med: pip3 install rpi_ws281x --break-system-packages
from rpi_ws281x import Color, PixelStrip, ws
import time
import datetime
import threading
import sys
import traceback
from enum import Enum
import itertools

######## VARIABLER ########

# LED-stripkonfiguration:
LED_COUNT = 152        # Antal LED-pixlar.
LED_PIN = 18           # GPIO-pin ansluten till pixlarna.
LED_FREQ_HZ = 800000   # LED-signalens frekvens i hertz (vanligtvis 800kHz).
LED_DMA = 5            # DMA-kanal för att generera signalen.
LED_BRIGHTNESS = 255   # Maximal ljusstyrka (0-255).
LED_INVERT = False     # Invertera signalen (används med NPN-transistor).
LED_CHANNEL = 0
LED_STRIP = ws.SK6812_STRIP  # Typ av LED-strip, anpassad för WWA.

# Platshållare för ingen LED
X = 199


class LedColor(Enum):
    RED = Color(128, 255, 0)
    ICE_COLD_WHITE = (Color(0, 255, 0),)  # Mycket kallvit
    COLD_WHITE = (Color(0, 255, 64),)  # Kallvit
    NEUTRAL_WHITE = (Color(0, 255, 128),)  # Neutralvit
    SOFT_WHITE = (Color(0, 192, 192),)  # Mjukvit
    WARM_WHITE = (Color(0, 128, 255),)  # Varmvit
    WARMER_WHITE = (Color(0, 64, 255),)  # Varm glödlampa
    WARMEST_WHITE = (Color(0, 0, 255),)  # Mycket varm (stearinljus)


# Cyclic iterable over LedColor, yielding a new color every iteration.
# This iterator wraps. Use as:
#   c = next(color_cycle) #-> c = RED
#   c = next(color_cycle) #-> c = WHATEVER
color_cycle = itertools.cycle(LedColor)

# Initiala inställningar
currentColor = LedColor.RED
currentBrightness = 200  # Standardljusstyrka (0-255)

# Knappinställningar
BUTTON_PIN = 26            # GPIO-pin där knappen är ansluten
BUTTON_HOLD_TIME = 1       # Tid i sekunder för att registrera ett knapphåll
BRIGHTNESS_STEP = 5        # Stegstorlek för ljusstyrkejustering
MIN_BRIGHTNESS = 10        # Minimal ljusstyrka
MAX_BRIGHTNESS = 255       # Maximal ljusstyrka
BOUNCE_TIME = 0.1         # Debounce-tid för knappen i sekunder

######## LED-MATRIS PINOUT ########

# LED-MATRISER (ingen ändring har gjorts här)
hour_leds = [
    [10, 11, 28, 29, 50, 51],          # ETT
    [12, 13, 26, 27, 52, 53],          # TVÅ
    [14, 15, 24, 25, 54, 55],          # TRE
    [92, 93, 114, 115, 128, 129, 140, 141],  # FYRA
    [112, 113, 130, 131, 138, 139],    # FEM
    [16, 17, 22, 23, 56, 57],          # SEX
    [116, 117, 126, 127, 142, 143],    # SJU
    [62, 63, 78, 79, 84, 85, 86, 87],  # ÅTTA
    [18, 19, 20, 21, 58, 59],          # NIO
    [118, 119, 124, 125, 144, 145],    # TIO
    [60, 61, 80, 81, 82, 83, 88, 89],  # ELVA
    [90, 91, 120, 121, 122, 123, 146, 147]   # TOLV
]

word_leds = [
    [0, 1, 38, 39, 40, 41, 70, 71, 72, 73, 96, 97, 98, 99, 104,
     105, 134, 135],  # KLOCKAN ÄR
    [2, 3, 36, 37, 42, 43, 108, 109],  # FEM I
    [100, 101, 102, 103, 106, 107, 108, 109],  # TIO I
    [4, 5, 34, 35, 44, 45, 68, 69, 74, 75, 108, 109],  # KVART I
    [6, 7, 32, 33, 46, 47, 66, 67, 76, 77, 108, 109],  # TJUGO I
    [2, 3, 36, 37, 42, 43, 108, 109, 94, 95, 110, 111,
     132, 133, 136, 137],  # FEM I HALV
    [94, 95, 110, 111, 132, 133, 136, 137],  # HALV
    [2, 3, 36, 37, 42, 43, 8, 9, 30, 31, 48, 49, 64, 65,
     94, 95, 110, 111, 132, 133, 136, 137],  # FEM ÖVER HALV
    [2, 3, 36, 37, 42, 43, 8, 9, 30, 31, 48, 49, 64, 65],  # FEM ÖVER
    [100, 101, 102, 103, 106, 107, 8, 9, 30, 31, 48, 49, 64, 65],  # TIO ÖVER
    [4, 5, 34, 35, 44, 45, 68, 69, 74, 75, 8, 9, 30, 31,
     48, 49, 64, 65],  # KVART ÖVER
    [6, 7, 32, 33, 46, 47, 66, 67, 76, 77, 8, 9, 30, 31,
     48, 49, 64, 65]  # TJUGO ÖVER
]

# Små lampor som visar minut
minut_led_nr1 = 151
minut_led_nr2 = 150
minut_led_nr3 = 149
minut_led_nr4 = 148

minute_leds = [
    [minut_led_nr1],  # Minut 1
    [minut_led_nr1, minut_led_nr2],  # Minut 2
    [minut_led_nr1, minut_led_nr2, minut_led_nr3],  # Minut 3
    [minut_led_nr1, minut_led_nr2, minut_led_nr3, minut_led_nr4]  # Minut 4
]

######## INITIALISERING ########

strip = PixelStrip(LED_COUNT, LED_PIN, LED_FREQ_HZ, LED_DMA, LED_INVERT,
                   LED_BRIGHTNESS, LED_CHANNEL, LED_STRIP)

clockStarted = False
currentTime = datetime.datetime.now()
press_time = 0  # Variabel för att lagra tidpunkten när knappen trycks ned
brightnessIncreasing = True # Startar med att öka ljusstyrkan när knappen hålls inne

######## FUNKTIONER ########

### LED-FUNKTIONER ###
def SetColor(fStrip, fColor):
    for i in range(fStrip.numPixels()):
        fStrip.setPixelColor(i, fColor)
    fStrip.show()


def ClearLeds(fStrip):
    for i in range(fStrip.numPixels()):
        fStrip.setPixelColor(i, Color(0, 0, 0))
    fStrip.show()


def UpdateTime(fStrip, fTime, fColor, fBrightness):
    ClearLeds(fStrip)                   # Rensa LEDs innan ändring
    fStrip.setBrightness(fBrightness)   # Ställ in önskad ljusstyrka

    # Tänd "KLOCKAN ÄR"
    for iLed in word_leds[0]:
        if iLed != X:
            fStrip.setPixelColor(iLed, fColor)

    # Hämta aktuell timme och minut
    minute = fTime.minute
    hour = fTime.hour % 12  # Konvertera till 12-timmarsformat
    if hour == 0:
        hour = 12  # Hantera midnatt som 12

    # Justera timme baserat på minut
    display_hour = hour
    if minute >= 25:
        display_hour += 1
        if display_hour > 12:
            display_hour = 1  # Återställ till 1 efter 12

    # Mappa timme till index i hour_leds
    hour_index = (display_hour - 1)  # Nollbaserat index

    # Bestäm vilka ord som ska tändas baserat på aktuell minut
    if minute >= 0 and minute < 5:
        # "KLOCKAN ÄR" + timme
        words_to_light = []
    elif minute >= 5 and minute < 10:
        # "Fem Över" + timme
        words_to_light = word_leds[8]
    elif minute >= 10 and minute < 15:
        # "Tio Över" + timme
        words_to_light = word_leds[9]
    elif minute >= 15 and minute < 20:
        # "Kvart Över" + timme
        words_to_light = word_leds[10]
    elif minute >= 20 and minute < 25:
        # "Tjugo Över" + timme
        words_to_light = word_leds[11]
    elif minute >= 25 and minute < 30:
        # "Fem I Halv" + nästa timme
        words_to_light = word_leds[5]
    elif minute >= 30 and minute < 35:
        # "Halv" + nästa timme
        words_to_light = word_leds[6]
    elif minute >= 35 and minute < 40:
        # "Fem Över Halv" + nästa timme
        words_to_light = word_leds[7]
    elif minute >= 40 and minute < 45:
        # "Tjugo I" + nästa timme
        words_to_light = word_leds[4]
    elif minute >= 45 and minute < 50:
        # "Kvart I" + nästa timme
        words_to_light = word_leds[3]
    elif minute >= 50 and minute < 55:
        # "Tio I" + nästa timme
        words_to_light = word_leds[2]
    elif minute >= 55 and minute < 60:
        # "Fem I" + nästa timme
        words_to_light = word_leds[1]
    else:
        words_to_light = []

    # Tänd valda ord
    for iLed in words_to_light:
        if iLed != X:
            fStrip.setPixelColor(iLed, fColor)

    # Tänd timmen
    for iLed in hour_leds[hour_index % 12]:
        if iLed != X:
            fStrip.setPixelColor(iLed, fColor)

    # Tänd minut-LEDs för extra minuter (1-4)
    extra_minutes = minute % 5
    if extra_minutes > 0:
        for i in range(extra_minutes):
            led_index = minute_leds[extra_minutes - 1][i]
            if led_index != X:
                fStrip.setPixelColor(led_index, fColor)

    # Uppdatera LEDs
    fStrip.show()

### KNAPPHANTERING ###
button = Button(BUTTON_PIN, pull_up=True, hold_time=BUTTON_HOLD_TIME, bounce_time=BOUNCE_TIME)

def adjust_brightness():
    global currentBrightness, brightnessIncreasing
    brightnessIncreasing = not brightnessIncreasing

    # NOTE: Get brightness from strip object instead of global?

    step = BRIGHTNESS_STEP if brightnessIncreasing else -BRIGHTNESS_STEP

    while button.is_pressed:
        # Update brightness with boundary checks
        currentBrightness = max(min(currentBrightness + step, MAX_BRIGHTNESS), MIN_BRIGHTNESS)

        # Uppdatera ljusstyrkan på strippen
        strip.setBrightness(currentBrightness)
        strip.show()

        time.sleep(0.05)  # Justera för mjuk övergång

    # När justeringen är klar, uppdatera klockan med aktuell ljusstyrka
    currentTime = datetime.datetime.now()
    UpdateTime(strip, currentTime, currentColor, currentBrightness)

def on_button_pressed():
    global press_time
    press_time = time.time()
    print("Knapp nedtryckt vid {:.2f}".format(press_time))

def on_button_released():
    global currentColorIndex, currentColor
    release_time = time.time()
    hold_duration = release_time - press_time
    print("Knapp släppt vid {:.2f}, hålltid: {:.2f} sekunder".format(release_time, hold_duration))
    if hold_duration < BUTTON_HOLD_TIME:
        # Knappen klickades, ändra färg
        # currentColorIndex = (currentColorIndex + 1) % len(color_options)
        currentColor = next(color_cycle)
        # Uppdatera displayen med den nya färgen
        currentTime = datetime.datetime.now()
        UpdateTime(strip, currentTime, currentColor, currentBrightness)
        print("Färg ändrad till alternativ {}".format(currentColor.name))
    else:
        # Knappen hölls ned längre än hold_time
        print("Knappen hölls ned längre än hold_time")

def on_button_held():
    print("Knappen hålls ned, startar ljusstyrkejustering")
    threading.Thread(target=adjust_brightness).start()

# Callbacks for button
button.when_pressed = on_button_pressed
button.when_released = on_button_released
button.when_held = on_button_held

# NOTE: Asyncio?

######## HUVUDLOOP ########
while True:  # Yttre loop för att hantera omstarter och fel
    try:
        ######## KLOCKSTART ########
        if not clockStarted:  # Starta klockan
            strip.begin()  # Initiera LEDs
            ClearLeds(strip)

            ######## KLOCKA UPPDATERA TID FÖRSTA START ########
            currentTime = datetime.datetime.now()
            UpdateTime(strip, currentTime, currentColor, currentBrightness)
            old_minute = currentTime.minute

            clockStarted = True
            print("Klockan är igång")

        while clockStarted:  # Klockans loop
            # Hämta aktuell tid
            currentTime = datetime.datetime.now()

            # Om en ny minut har passerat, uppdatera klockan
            if currentTime.minute != old_minute:
                UpdateTime(strip, currentTime, currentColor, currentBrightness)
                print("Uppdaterad tid: {}:{}:{}".format(
                    currentTime.hour, currentTime.minute, currentTime.second))
                old_minute = currentTime.minute

            time.sleep(1)  # Vänta en sekund innan nästa kontroll

    except Exception as e:
        print("Ett fel inträffade: ", e)
        traceback.print_exc()  # Skriv ut detaljerad felinformation
        print("Startar om loopen...")
        clockStarted = False
        time.sleep(2)  # Vänta lite innan omstart

    time.sleep(1)  # Vänta 1 sekund innan nästa iteration av den yttre loopen
