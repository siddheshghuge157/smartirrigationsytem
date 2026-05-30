#  Smart Irrigation System

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Arduino](https://img.shields.io/badge/Arduino-Compatible-green.svg)
![ESP32](https://img.shields.io/badge/ESP32-Supported-red.svg)

So I built this because my plants kept dying. Turns out, yelling at them doesn't work. This is an ESP32-based watering system I hacked together that actually uses some clever "Cycle & Soak" technique to water properly instead of flooding everything. It's got a web dashboard, an LCD menu (with those clicky buttons), and it even nags me when it's time to add fertilizer. Not bad for a weekend project.

![My messy web dashboard](docs/images/web-interface.png)

---

##  What it can do

###  Watering control
- **Cycle & Soak** – instead of pouring water all at once, it does short bursts with pauses. The ground actually soaks it up instead of running off.
- **3 modes** – OFF (for when I'm messing with the wiring), MANUAL (just turn the pump on), and AUTO (the whole point).
- **Safety shutoff** – learned this the hard way when a pump ran dry for 20 minutes. Never again.
- **4 watering times** – set up to 4 times a day. I use morning and evening, but you do you.

###  Monitoring
- **Capacitive moisture sensor** – the good kind, not those resistive ones that rust after a week.
- **DHT11** for temperature & humidity. It's not super accurate, but good enough to know if it's crazy hot.
- **Stats** – tracks how many times the pump kicked in and for how long. I like data.
- **Status LED** – blinks green when everything's fine, or goes red if something's wrong.

###  "Smart" things
- **Fertilizer reminder** – every 7 days it tells me to feed the plants. Without it I'd forget.
- **Moisture-based watering** – only waters when the soil actually gets dry, not just because the clock says so.
- **Thresholds you can tweak** – I set "dry" to 30% and "wet" to 70% after some guesswork.
- **Settings saved to flash** – so no losing configs when the power dips.

### Interfaces
- **Web dashboard** – accessible over WiFi. It's not going to win any design awards, but it's functional and updates live.
- **LCD with buttons** – I stuck six little buttons (UP, DOWN, LEFT, RIGHT, SELECT, BACK) to navigate a menu. Feels like a 90s thermostat, but in a good way.
- **WiFi modes** – starts as an access point so you can configure it, then can join your home WiFi if you want.
- **Simple REST API** – you can hook it up to Node-RED or whatever.

---

##  Parts list

Here's what I used. I spent maybe rs 3500 total because I had some stuff lying around.

| Component

| ESP32 Dev Board 
| Capacitive Soil Moisture Sensor 
| DHT11 Sensor 
| 16x2 LCD + I2C
| IRFZ44N MOSFET 
| 12V Water Pump 
| 1N4007 Diode 
| Tactile Push Buttons x6
| LED 
| 220Ω Resistor 
| 7.4V Battery 
| 5V Regulator 
| Breadboard & wires 


---

## Software you'll need

Open Arduino IDE, then **Sketch → Include Library → Manage Libraries** and get:

1. **DHT sensor library** by Adafruit
2. **Adafruit Unified Sensor** (a dependency)
3. **LiquidCrystal I2C** by Frank de Brabander
4. **ArduinoJson** (v6.x) by Benoit Blanchon

For the web server, you need to install these manually (put them in your `Arduino/libraries/` folder):

5. **ESPAsyncWebServer** – [github.com/me-no-dev/ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
6. **AsyncTCP** – [github.com/me-no-dev/AsyncTCP](https://github.com/me-no-dev/AsyncTCP)

---

## Quick start

### 1. Wire it up
See [WIRING.md](docs/WIRING.md) for the diagram. Here’s the pinout I used:

Pump Control: GPIO 32
Moisture Sensor: GPIO 34
DHT11: GPIO 27
Status LED: GPIO 23
I2C SDA: GPIO 21
I2C SCL: GPIO 22
Buttons: GPIO 4,2,15,13,12,14


### 2. Software Installation

1. Download this repository (green "Code" button → Download ZIP)
2. Extract the ZIP file
3. Open `src/SmartIrrigation/SmartIrrigation.ino` in Arduino IDE
4. Install required libraries (see above)
5. Select board: `Tools → Board → ESP32 Dev Module`
6. Select port: `Tools → Port → (your ESP32 port)`
7. Click Upload!

### 3. First Run

1. **Power on** - ESP32 creates WiFi: `SmartIrrigation` (password: `12345678`)
2. **Connect** your phone to this WiFi
3. **Open browser** to `http://192.168.4.1`
4. **Configure** your settings
5. **Done!** 

---

## Usage

### Web Dashboard
- Access at `http://192.168.4.1`
- Real-time sensor data
- Full configuration control
- Statistics and graphs

### LCD Menu
Navigate with buttons:
- **UP/DOWN**: Navigate menu
- **SELECT**: Enter/Confirm
- **BACK**: Exit/Cancel
- **LEFT/RIGHT**: Adjust values

---

## Documentation

- [**Wiring Guide**](docs/WIRING.md) - Detailed connections
- [**Bill of Materials**](hardware/BOM.md) - Component list

---

##  Configuration

### Default Settings
Moisture Dry: 30%
Moisture Wet: 70%
Water Time: 30 seconds
Soak Time: 60 seconds
Cycles: 3
Mode: AUTO
Schedule 1: 06:00 AM 
Schedule 2: 06:00 PM 


---

##  Troubleshooting

### LCD not working?
- Try I2C address `0x3F` instead of `0x27`

### Moisture readings wrong?
- Calibrate in code (see calibration example)

### Web page won't load?
- Connect to `SmartIrrigation` WiFi first
- Try `192.168.4.1` in browser

---

##  License

This project is licensed under the MIT License.

---

##  Acknowledgments

- Adafruit for sensor libraries
- ESP32 community

---

**Made for plants and automation**