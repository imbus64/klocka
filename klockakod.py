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
from datetime import datetime
import threading
import traceback
from enum import Enum
import logging
import itertools


# Simple logging setup
logging.basicConfig(level=logging.DEBUG)  # , format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

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


class LedHours(Enum):
    ETT = ([10, 11, 28, 29, 50, 51],)  # ETT
    TVA = ([12, 13, 26, 27, 52, 53],)  # TVÅ
    TRE = ([14, 15, 24, 25, 54, 55],)  # TRE
    FYRA = ([92, 93, 114, 115, 128, 129, 140, 141],)  # FYRA
    FEM = ([112, 113, 130, 131, 138, 139],)  # FEM
    SEX = ([16, 17, 22, 23, 56, 57],)  # SEX
    SJU = ([116, 117, 126, 127, 142, 143],)  # SJU
    ATTA = ([62, 63, 78, 79, 84, 85, 86, 87],)  # ÅTTA
    NIO = ([18, 19, 20, 21, 58, 59],)  # NIO
    TIO = ([118, 119, 124, 125, 144, 145],)  # TIO
    ELVA = ([60, 61, 80, 81, 82, 83, 88, 89],)  # ELVA
    TOLV = ([90, 91, 120, 121, 122, 123, 146, 147],)  # TOLV

    @classmethod
    def get_nth_hour(cls, n: int):
        """Returns the Nth hour (N is a 0-based index)."""
        return list(cls)[n % len(cls)].value[0]


class LedWords(Enum):
    KLOCKAN_AR = ([0, 1, 38, 39, 40, 41, 70, 71, 72, 73, 96, 97, 98, 99, 104, 105, 134, 135],)  # KLOCKAN ÄR
    FEM_I = ([2, 3, 36, 37, 42, 43, 108, 109],)  # FEM I
    TIO_I = ([100, 101, 102, 103, 106, 107, 108, 109],)  # TIO I
    KVART_I = ([4, 5, 34, 35, 44, 45, 68, 69, 74, 75, 108, 109],)  # KVART I
    TJUGO_I = ([6, 7, 32, 33, 46, 47, 66, 67, 76, 77, 108, 109],)  # TJUGO I
    FEMI_I_HALV = ([2, 3, 36, 37, 42, 43, 108, 109, 94, 95, 110, 111, 132, 133, 136, 137],)  # FEM I HALV
    HALV = ([94, 95, 110, 111, 132, 133, 136, 137],)  # HALV
    FEM_OVER_HALV = ([2, 3, 36, 37, 42, 43, 8, 9, 30, 31, 48, 49, 64, 65, 94, 95, 110, 111, 132, 133, 136, 137],)
    FEM_OVER = ([2, 3, 36, 37, 42, 43, 8, 9, 30, 31, 48, 49, 64, 65],)  # FEM ÖVER
    TIO_OVER = ([100, 101, 102, 103, 106, 107, 8, 9, 30, 31, 48, 49, 64, 65],)  # TIO ÖVER
    KVART_OVER = ([4, 5, 34, 35, 44, 45, 68, 69, 74, 75, 8, 9, 30, 31, 48, 49, 64, 65],)  # KVART ÖVER
    TJUGO_OVER = ([6, 7, 32, 33, 46, 47, 66, 67, 76, 77, 8, 9, 30, 31, 48, 49, 64, 65],)  # TJUGO ÖVER


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

# Define the time intervals and their corresponding words
time_intervals = [
    (range(0, 5), []),  # "KLOCKAN ÄR" + timme
    (range(5, 10), LedWords.FEM_OVER),  # "Fem Över" + timme
    (range(10, 15), LedWords.TIO_OVER),  # "Tio Över" + timme
    (range(15, 20), LedWords.KVART_OVER),  # "Kvart Över" + timme
    (range(20, 25), LedWords.TJUGO_OVER),  # "Tjugo Över" + timme
    (range(25, 30), LedWords.FEMI_I_HALV),  # "Fem I Halv" + nästa timme
    (range(30, 35), LedWords.HALV),  # "Halv" + nästa timme
    (range(35, 40), LedWords.FEM_OVER_HALV),  # "Fem Över Halv" + nästa timme
    (range(40, 45), LedWords.TJUGO_I),  # "Tjugo I" + nästa timme
    (range(45, 50), LedWords.KVART_I),  # "Kvart I" + nästa timme
    (range(50, 55), LedWords.TIO_I),  # "Tio I" + nästa timme
    (range(55, 60), LedWords.FEM_I),  # "Fem I" + nästa timme
]

######## INITIALISERING ########

strip = PixelStrip(LED_COUNT, LED_PIN, LED_FREQ_HZ, LED_DMA, LED_INVERT,
                   LED_BRIGHTNESS, LED_CHANNEL, LED_STRIP)

clockStarted = False
currentTime = datetime.now()
press_time = 0  # Variabel för att lagra tidpunkten när knappen trycks ned
brightnessIncreasing = True # Startar med att öka ljusstyrkan när knappen hålls inne

######## FUNKTIONER ########

### LED-FUNKTIONER ###
def SetColor(fStrip: PixelStrip, fColor: LedColor):
    for i in range(fStrip.numPixels()):
        fStrip.setPixelColor(i, fColor)
    fStrip.show()


def ClearLeds(fStrip):
    for i in range(fStrip.numPixels()):
        fStrip.setPixelColor(i, Color(0, 0, 0))
    fStrip.show()


def UpdateTime(fStrip: PixelStrip, fTime: datetime, fColor: LedColor, fBrightness: int) -> None:
    ClearLeds(fStrip)  # Rensa LEDs innan ändring
    fStrip.setBrightness(fBrightness)  # Ställ in önskad ljusstyrka

    # Tänd "KLOCKAN ÄR"
    for iLed in LedWords.KLOCKAN_AR.value:
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
    hour_index = display_hour - 1  # Nollbaserat index

    # Iterate over the intervals and find the corresponding words to light
    for time_range, words in time_intervals:
        if minute in time_range:
            words_to_light = words
            break
    else:
        words_to_light = []

    # Tänd valda ord
    for iLed in words_to_light:
        if iLed != X:
            fStrip.setPixelColor(iLed, fColor)

    # Tänd timmen
    for iLed in LedHours.get_nth_hour(hour_index):
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
    currentTime = datetime.now()
    UpdateTime(strip, currentTime, currentColor, currentBrightness)

def on_button_pressed():
    global press_time
    press_time = time.time()
    logger.info("Knapp nedtryckt vid {:.2f}".format(press_time))

def on_button_released():
    global currentColorIndex, currentColor
    release_time = time.time()
    hold_duration = release_time - press_time
    logger.info("Knapp släppt vid {:.2f}, hålltid: {:.2f} sekunder".format(release_time, hold_duration))
    if hold_duration < BUTTON_HOLD_TIME:
        # Knappen klickades, ändra färg
        currentColor = next(color_cycle)
        # Uppdatera displayen med den nya färgen
        currentTime = datetime.now()
        UpdateTime(strip, currentTime, currentColor, currentBrightness)
        logger.info("Färg ändrad till alternativ {}".format(currentColor.name))
    else:
        # Knappen hölls ned längre än hold_time
        logger.info("Knappen hölls ned längre än hold_time")

def on_button_held():
    logger.info("Knappen hålls ned, startar ljusstyrkejustering")
    threading.Thread(target=adjust_brightness).start()

# Callbacks for button
button.when_pressed = on_button_pressed
button.when_released = on_button_released
button.when_held = on_button_held

# NOTE: Asyncio?

######## HUVUDLOOP ########
while True:  # Yttre loop för att hantera omstarter och fel

    # Make sure this is initialized here for now
    old_minute = datetime.now().minute

    try:
        ######## KLOCKSTART ########
        if not clockStarted:  # Starta klockan
            strip.begin()  # Initiera LEDs
            ClearLeds(strip)

            ######## KLOCKA UPPDATERA TID FÖRSTA START ########
            currentTime = datetime.now()
            UpdateTime(strip, currentTime, currentColor, currentBrightness)
            old_minute = currentTime.minute

            clockStarted = True
            logger.info("Klockan är igång")

        while clockStarted:  # Klockans loop
            # Hämta aktuell tid
            currentTime = datetime.now()

            # Om en ny minut har passerat, uppdatera klockan
            if currentTime.minute != old_minute:
                UpdateTime(strip, currentTime, currentColor, currentBrightness)
                logger.info("Uppdaterad tid: {}:{}:{}".format(currentTime.hour, currentTime.minute, currentTime.second))
                old_minute = currentTime.minute

            time.sleep(1)  # Vänta en sekund innan nästa kontroll

    except Exception as e:
        logger.error("Ett fel inträffade: ", e)
        logger.error(traceback.format_exc()) # Skriv ut detaljerad felinformation
        logger.info("Startar om loopen...")
        clockStarted = False
        time.sleep(2)  # Vänta lite innan omstart

    time.sleep(1)  # Vänta 1 sekund innan nästa iteration av den yttre loopen
