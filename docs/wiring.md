# 🔌 Wiring Guide

## Pin Assignments Reference

**ESP32 GPIO Pins:**
GPIO 32 → Pump Control
GPIO 34 → Moisture Sensor
GPIO 27 → DHT11 Data
GPIO 23 → Status LED
GPIO 21 → I2C SDA (LCD)
GPIO 22 → I2C SCL (LCD)
GPIO 4 → Button UP
GPIO 2 → Button DOWN
GPIO 15 → Button LEFT
GPIO 13 → Button RIGHT
GPIO 12 → Button SELECT
GPIO 14 → Button BACK
3.3V → Sensor Power
5V → LCD Power
GND → Common Ground


---

## Component Connections

### 1. Pump Control Circuit

**Connections:**
1. **MOSFET Gate** → ESP32 GPIO 32
2. **MOSFET Source** → Common Ground (GND)
3. **MOSFET Drain** → Pump Negative (-) AND Diode Cathode (stripe side)
4. **Pump Positive (+)** → 12V Power Supply Positive
5. **Diode Anode** → 12V Power Supply Positive

**Important Notes:**
- Diode polarity matters! The stripe goes to +12V
- MOSFET has 3 pins: Gate, Drain, Source (check datasheet for your specific MOSFET)
- Use thick wires for pump connections (18-22 AWG recommended)
- The diode protects against voltage spikes when pump turns off

---

### 2. Capacitive Moisture Sensor

**Connections:**
1. **Sensor VCC** → ESP32 3.3V
2. **Sensor SIGNAL/AOUT** → ESP32 GPIO 34
3. **Sensor GND** → ESP32 GND

**Important Notes:**
- GPIO 34 is ADC-only pin (cannot be used as output)
- Some sensors work on 5V - check your sensor specifications
- Insert sensor vertically into soil, 2-3 cm deep
- Keep electronics part above soil to prevent corrosion

---

### 3. DHT11 Temperature/Humidity Sensor

**Connections:**
1. **DHT11 VCC** → ESP32 3.3V
2. **DHT11 DATA** → ESP32 GPIO 27
3. **DHT11 GND** → ESP32 GND

**Important Notes:**
- Some DHT11 modules have 3 pins, some have 4 pins
- If 4 pins, the 3rd pin (NC) is not connected
- Most modules have built-in pullup resistor
- If your module doesn't have resistor, add 10kΩ between VCC and DATA

---

### 4. LCD Display with I2C

**Connections:**
1. **LCD VCC** → ESP32 5V
2. **LCD GND** → ESP32 GND
3. **LCD SDA** → ESP32 GPIO 21
4. **LCD SCL** → ESP32 GPIO 22

**Important Notes:**
- LCD requires 5V, not 3.3V
- Default I2C address is usually 0x27 or 0x3F
- If display doesn't work, try changing address in code
- Adjust contrast using small potentiometer on I2C backpack (blue box on back of LCD)

---

### 5. Control Buttons

**All buttons follow the same pattern:**

**Button UP:**
1. **One side of button** → ESP32 GPIO 4
2. **Other side of button** → ESP32 GND

**Button DOWN:**
1. **One side of button** → ESP32 GPIO 2
2. **Other side of button** → ESP32 GND

**Button LEFT:**
1. **One side of button** → ESP32 GPIO 15
2. **Other side of button** → ESP32 GND

**Button RIGHT:**
1. **One side of button** → ESP32 GPIO 13
2. **Other side of button** → ESP32 GND

**Button SELECT:**
1. **One side of button** → ESP32 GPIO 12
2. **Other side of button** → ESP32 GND

**Button BACK:**
1. **One side of button** → ESP32 GPIO 14
2. **Other side of button** → ESP32 GND

**Important Notes:**
- No external resistors needed (code uses internal pullup resistors)
- Button orientation doesn't matter
- Use any normally-open tactile switch
- All buttons share common ground connection

---

### 6. Status LED

**Connections:**
1. **ESP32 GPIO 23** → **220Ω Resistor** → **LED Long Leg (Anode/+)**
2. **LED Short Leg (Cathode/-)** → ESP32 GND

**Important Notes:**
- LED polarity matters! Long leg is positive (+)
- If legs are same length, flat side of LED is negative (-)
- 220Ω resistor prevents LED from burning out
- Any color LED works (red, green, blue, etc.)

---

### 7. Power Supply

**Option A: Single 5V USB Power Supply (Simplest)**

1. **USB Power Bank/Adapter 5V** → ESP32 USB Port or 5V Pin
2. **Power GND** → ESP32 GND
3. **ESP32 5V Pin** → LCD VCC
4. **ESP32 3.3V Pin** → Sensors VCC
5. **Separate 12V Supply Positive** → Pump Circuit
6. **12V Supply GND** → Common Ground with ESP32

**Option B: Battery with Regulator**

1. **7.4V Battery Positive** → 5V Regulator Input
2. **5V Regulator Output** → ESP32 5V Pin
3. **5V Regulator Output** → LCD VCC
4. **5V Regulator GND** → Common Ground
5. **ESP32 3.3V Pin** → Sensors VCC
6. **Separate 12V for Pump** → Pump Circuit
7. **All GND connections** → Common Ground

**Option C: Single 12V Battery (Most Efficient)**

1. **12V Battery Positive** → Buck Converter Input
2. **Buck Converter Output (set to 5V)** → ESP32 5V Pin
3. **Buck Converter Output** → LCD VCC
4. **12V Battery Positive** → Pump Circuit (direct)
5. **All GND connections** → Common Ground
6. **ESP32 3.3V Pin** → Sensors VCC

**Power Requirements:**
- ESP32: 500mA (with WiFi active)
- LCD: 100mA
- DHT11: 2.5mA
- Moisture Sensor: 5mA
- LED: 20mA
- Pump: 500-2000mA (varies by pump model)

**Total: Approximately 1-2.5A @ 5V + Pump current @ 12V**

---

## Complete Wiring Checklist

**Before powering on, verify:**

- [ ] All ground (GND) connections are connected together
- [ ] ESP32 has proper 5V power supply
- [ ] LCD connected to 5V (not 3.3V)
- [ ] Sensors connected to 3.3V (not 5V)
- [ ] MOSFET Gate connected to GPIO 32
- [ ] MOSFET Source connected to GND
- [ ] MOSFET Drain connected to Pump Negative
- [ ] Diode stripe (cathode) pointing toward +12V
- [ ] LED resistor in place (220Ω)
- [ ] LED polarity correct (long leg to resistor)
- [ ] All button connections to correct GPIO pins
- [ ] Moisture sensor on GPIO 34 (ADC-only pin)
- [ ] I2C connections: SDA to GPIO 21, SCL to GPIO 22
- [ ] DHT11 data pin to GPIO 27
- [ ] No loose wires or short circuits
- [ ] Pump can access water (won't run dry)

---

## Troubleshooting

### ESP32 won't power on
- Check 5V power supply voltage with multimeter
- Verify correct polarity (+ and -)
- Try connecting via USB cable instead
- Check if power LED on ESP32 lights up

### Pump doesn't run
- Test pump directly with 12V battery to confirm it works
- Check MOSFET orientation (Gate, Drain, Source)
- Verify GPIO 32 connection is secure
- Check diode is not short-circuited
- Measure voltage at MOSFET gate when pump should be ON (should be 3.3V)

### Moisture sensor returns wrong values
- Check if sensor needs 5V instead of 3.3V
- Verify GPIO 34 connection
- Insert sensor properly in soil
- Calibrate using the calibration sketch
- Check sensor isn't damaged (no visible corrosion)

### LCD shows nothing
- Adjust contrast potentiometer on I2C backpack (turn slowly)
- Try I2C address 0x3F in code instead of 0x27
- Verify SDA and SCL not swapped
- Confirm LCD is getting 5V power
- Run I2C scanner code to detect address

### LCD shows garbled text
- Adjust contrast potentiometer
- Check baud rate in Serial Monitor (should be 115200)
- Verify all ground connections
- Try different I2C address

### DHT11 returns NaN (Not a Number)
- Check 3.3V power connection
- Verify data pin connection to GPIO 27
- Sensor may need warm-up time (wait 2 seconds after power on)
- Try adding 10kΩ pullup resistor between VCC and DATA if not present

### Buttons don't respond
- Verify code has INPUT_PULLUP mode enabled
- Check button connected between GPIO and GND (not GPIO and VCC)
- Test button continuity with multimeter
- Try pressing button harder (some buttons need firm press)
- Check button orientation (try rotating 90 degrees)

### Web page won't load
- Check Serial Monitor for IP address
- Ensure phone/computer connected to "SmartIrrigation" WiFi
- Disable mobile data on phone
- Try both 192.168.4.1 and IP shown in Serial Monitor
- Check if ESP32 is in AP mode (should show "AP Started" in Serial Monitor)
- Clear browser cache and try again

### WiFi connection fails
- Check SSID and password are correct
- Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
- Move ESP32 closer to router
- Check router allows new device connections
- Try restarting ESP32

---

## Safety Warnings

 **IMPORTANT:**
- Never run pump without water (damages motor)
- Keep all electronics away from water
- Use waterproof enclosure if used outdoors
- Double-check all connections before applying power
- Don't exceed 12V on pump circuit
- Ensure proper ventilation in enclosure
- Disconnect power before making wiring changes
- Use proper wire gauge for pump (at least 22 AWG)

---
## Testing Individual Components

**Test before assembling everything:**

### Test ESP32:
Upload "Blink" example, verify LED blinks

### Test LCD:
Upload I2C scanner, verify address detected
Upload LCD "Hello World" example

### Test DHT11:
Upload DHT11 example from library, check Serial Monitor

### Test Moisture Sensor:
Upload analog read example, verify values change when touching sensor

### Test Buttons:
Upload digital read example, verify button press detected

### Test MOSFET:
Connect LED through MOSFET, test with digitalWrite() commands

### Test Pump:
Connect directly to 12V, verify it pumps water

