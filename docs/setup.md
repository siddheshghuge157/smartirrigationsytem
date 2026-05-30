
#  Setup Guide - Step by Step

This guide will walk you through setting up your Smart Irrigation System from scratch.

---

## Part 1: Install Arduino IDE

### Windows:
1. Go to https://www.arduino.cc/en/software
2. Download **Windows Installer** (not Windows app)
3. Run the installer
4. Accept all defaults
5. Finish installation

### Mac:
1. Go to https://www.arduino.cc/en/software
2. Download **macOS** version
3. Open DMG file
4. Drag Arduino to Applications folder
5. Open Arduino IDE

### Linux:
1. Download **Linux** version from Arduino website
2. Extract archive
3. Run `./install.sh`
4. Launch Arduino IDE

---

## 🔌 Part 2: Install ESP32 Board Support

1. **Open Arduino IDE**

2. **Go to:** File → Preferences

3. **Find:** "Additional Boards Manager URLs"

4. **Paste this URL:**
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

5. **Click OK**

6. **Go to:** Tools → Board → Boards Manager

7. **Search:** `esp32`

8. **Install:** "esp32 by Espressif Systems" (may take 5-10 minutes)

9. **Close** Boards Manager

10. **Select Board:** Tools → Board → ESP32 Arduino → **ESP32 Dev Module**

---

##  Part 3: Install Required Libraries

### Method 1: Arduino Library Manager (Recommended)

1. **Go to:** Sketch → Include Library → Manage Libraries

2. **Install these libraries one by one:**

   **Search:** `DHT sensor library`  
   **Install:** DHT sensor library by Adafruit  
   **Click Install All** when prompted for dependencies

   **Search:** `LiquidCrystal I2C`  
   **Install:** LiquidCrystal I2C by Frank de Brabander

   **Search:** `ArduinoJson`  
   **Install:** ArduinoJson by Benoit Blanchon (choose version 6.x)

3. **Close** Library Manager

---

### Method 2: Manual Installation (for ESPAsyncWebServer and AsyncTCP)

These libraries are not in the Library Manager, install manually:

#### ESPAsyncWebServer:

1. Go to: https://github.com/me-no-dev/ESPAsyncWebServer
2. Click green **"Code"** button
3. Click **"Download ZIP"**
4. Save to Downloads folder

#### AsyncTCP:

1. Go to: https://github.com/me-no-dev/AsyncTCP
2. Click green **"Code"** button
3. Click **"Download ZIP"**
4. Save to Downloads folder

#### Install Both ZIP Files:

1. **Open Arduino IDE**
2. **Go to:** Sketch → Include Library → Add .ZIP Library
3. **Select:** ESPAsyncWebServer-master.zip (from Downloads)
4. Click **Open**
5. **Repeat** for AsyncTCP-master.zip
6. **Restart Arduino IDE**

---

##  Part 4: Download Project Code

### Option A: Download from GitHub

1. Go to your GitHub repository (or the shared link)
2. Click green **"Code"** button
3. Click **"Download ZIP"**
4. **Extract** ZIP file to Documents folder
5. You should have folder: `smart-irrigation-system`

### Option B: Clone with Git (Advanced)

```bash
git clone https://github.com/YOUR_USERNAME/smart-irrigation-system.git
```

---

## Part 5: Open and Configure Project

1. **Navigate to:** `smart-irrigation-system/src/SmartIrrigation/`

2. **Double-click:** `SmartIrrigation.ino`

3. **Arduino IDE opens** with 3 tabs:
   - SmartIrrigation.ino
   - config.h
   - web_interface.h

4. **Verify all tabs loaded** (check top of Arduino IDE window)

---

##  Part 6: Configure for Your Setup

### Change WiFi Password (Recommended):

1. **Open tab:** `config.h`

2. **Find line:**
   ```cpp
   #define AP_PASSWORD         "12345678"
   ```

3. **Change to:**
   ```cpp
   #define AP_PASSWORD         "YourSecurePassword"
   ```

### Change LCD I2C Address (if needed):

1. **In config.h, find:**
   ```cpp
   #define LCD_ADDRESS     0x27
   ```

2. **If your LCD doesn't work, try:**
   ```cpp
   #define LCD_ADDRESS     0x3F
   ```

### Change Timezone (Optional):

1. **Find:**
   ```cpp
   #define TIMEZONE            "UTC0"
   ```

2. **Change to your timezone:**
   ```cpp
   // Examples:
   #define TIMEZONE "EST5EDT,M3.2.0,M11.1.0"  // Eastern Time USA
   #define TIMEZONE "PST8PDT,M3.2.0,M11.1.0"  // Pacific Time USA
   #define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"  // Central Europe
   #define TIMEZONE "IST-5:30"  // India
   #define TIMEZONE "AEST-10"  // Australia Eastern
   ```

---

##  Part 7: Connect ESP32 to Computer

1. **Connect ESP32** to computer via USB cable

2. **Wait** for drivers to install (Windows may take 1-2 minutes)

3. **In Arduino IDE, go to:** Tools → Port

4. **Select** your ESP32 port:
   - **Windows:** COM3, COM4, COM5, etc.
   - **Mac:** /dev/cu.usbserial-XXXX
   - **Linux:** /dev/ttyUSB0

5. **If no port appears:**
   - Windows: Install CP210x or CH340 drivers
   - Mac: Install drivers from Silicon Labs website
   - Linux: Add user to dialout group: `sudo usermod -a -G dialout $USER`

---

##  Part 8: Upload Code to ESP32

1. **Click** Verify button (✓ checkmark) to compile

2. **Wait** for compilation (may take 1-2 minutes first time)

3. **If compilation successful**, click Upload button (→ arrow)

4. **Wait** for upload (30-60 seconds)

5. **If upload fails:**
   - **Hold BOOT button** on ESP32 while uploading
   - Try different USB cable
   - Check correct port selected
   - Reduce upload speed: Tools → Upload Speed → 115200

6. **When you see "Hard resetting via RTS pin..."** - Upload successful! ✅

---

## Part 9: Test the System

### Open Serial Monitor:

1. **Click** magnifying glass icon (top right) OR Tools → Serial Monitor

2. **Set baud rate** to `115200` (bottom right dropdown)

3. **You should see:**
   ```
   === SMART IRRIGATION SYSTEM ===
   Config loaded
   AP Started
   AP IP: 192.168.4.1
   Web server started
   Setup complete!
   ```

4. **Every 2 seconds you'll see:**
   ```
   Moisture: 45% | Temp: 24.5C | Humidity: 60%
   ```

### Test LCD Display:

1. **LCD should show:**
   ```
   Line 1: Menu items
   Line 2: Sensor data
   ```

2. **If LCD is blank:**
   - Adjust contrast (small blue potentiometer on back)
   - Change I2C address in config.h

### Test Buttons:

1. **Press UP/DOWN** - menu should change
2. **Press SELECT** - should enter menu
3. **Press BACK** - should exit menu

### Test Web Interface:

1. **On phone/computer:**
   - Open WiFi settings
   - Connect to: `SmartIrrigation`
   - Password: `12345678` (or what you set)

2. **Open browser**
   - Go to: `http://192.168.4.1`
   - You should see the web dashboard

3. **You should see:**
   - Live sensor readings
   - Pump status
   - All controls

---

##  Part 10: Calibrate Moisture Sensor

### Run Calibration Sketch:

1. **Open:** `examples/calibration/moisture_calibration.ino`

2. **Upload** to ESP32

3. **Open Serial Monitor** (115200 baud)

4. **Test DRY reading:**
   - Hold sensor in air
   - Note the value (example: 2700)

5. **Test WET reading:**
   - Dip sensor in water (don't submerge electronics!)
   - Note the value (example: 1200)

6. **Update config.h:**
   ```cpp
   #define MOISTURE_RAW_WET    1200  // Your wet value
   #define MOISTURE_RAW_DRY    2700  // Your dry value
   ```

7. **Re-upload** main SmartIrrigation.ino

---

##  Part 11: Install in Garden

### Prepare Installation:

1. **Choose location** near plants that need watering

2. **Place moisture sensor** in soil (2-3 cm deep, vertically)

3. **Mount ESP32 and electronics** in waterproof box

4. **Place pump** in water reservoir

5. **Run tubing** from pump to plants

6. **Connect all wiring** according to wiring guide

7. **Power on system**

### Set Up Schedules:

1. **Connect to web interface**

2. **Go to Schedules section**

3. **Enable Schedule 1:**
   - Check "Enabled"
   - Set time: 06:00 (morning watering)

4. **Enable Schedule 2:**
   - Check "Enabled"
   - Set time: 18:00 (evening watering)

5. **Click "SAVE ALL SETTINGS"**

### Set Operation Mode:

1. **In web interface**, select **Mode: AUTO**

2. **System will now:**
   - Check soil moisture every 2 seconds
   - Water at scheduled times if soil is dry
   - Execute cycle & soak watering

---

## Part 12: Verification Checklist

**Before leaving system unattended:**

- [ ] All sensors reading correct values
- [ ] Pump turns on when commanded
- [ ] Pump turns off after cycle completes
- [ ] LCD displaying information
- [ ] Web interface accessible
- [ ] Schedules set correctly
- [ ] Mode set to AUTO
- [ ] Moisture thresholds configured
- [ ] Water reservoir has water
- [ ] All connections secure
- [ ] Electronics in waterproof enclosure
- [ ] Power supply stable
- [ ] Fertilizer reminder set

---

##  Common Setup Problems

### "Board not found" error:
- Install ESP32 board support (Part 2)
- Restart Arduino IDE

### "Library not found" error:
- Install all required libraries (Part 3)
- Check library names match exactly
- Restart Arduino IDE

### Upload fails:
- Hold BOOT button during upload
- Check USB cable (try different cable)
- Check correct port selected
- Lower upload speed to 115200

### Can't connect to WiFi:
- Check WiFi name is exactly "SmartIrrigation"
- Check password is correct
- Disable mobile data on phone
- Forget network and reconnect

### Sensors show 0 or NaN:
- Check wiring connections
- Verify 3.3V power for sensors
- Wait 2 seconds after power on
- Try example sketches for each sensor

### Pump won't turn on:
- Check 12V power supply connected
- Verify MOSFET wiring
- Test pump directly with 12V
- Check mode is MANUAL or AUTO

---

##  Getting Help

**Check these resources:**

1. **Serial Monitor** - Shows detailed error messages
2. **GitHub Issues** - Search for similar problems
3. **Arduino Forum** - ESP32 section
4. **Reddit** - r/arduino, r/esp32

**When asking for help, provide:**
- Error message from Serial Monitor
- Arduino IDE version
- ESP32 board version
- Which step failed
- What you've already tried

---

##  Success!

Your Smart Irrigation System is now set up and running!

**Next steps:**
- Monitor system for first few days
- Adjust moisture thresholds as needed
- Fine-tune watering schedules
- Add fertilizer when reminded

**Enjoy your automated garden! 🌱**
