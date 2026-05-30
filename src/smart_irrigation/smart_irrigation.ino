/*
  SMART IRRIGATION SYSTEM - COMPLETE
  Features: Cycle&Soak, Moisture/Temp/Humidity monitoring, Scheduling,
            LCD Menu, Web Interface, WiFi, Fertilizer Reminder, Statistics
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

// ==================== PIN DEFINITIONS ====================
#define PIN_PUMP        32
#define PIN_MOISTURE    34
#define PIN_DHT         27
#define PIN_LED         23
#define BTN_UP          4
#define BTN_DOWN        2
#define BTN_LEFT        15
#define BTN_RIGHT       13
#define BTN_SELECT      12
#define BTN_BACK        14
#define PIN_SDA         21
#define PIN_SCL         22

// ==================== CONSTANTS ====================
#define PUMP_MAX_RUNTIME    300000
#define DEBOUNCE_DELAY      50
#define LCD_REFRESH_MS      500
#define SENSOR_READ_MS      2000

// ==================== CONFIGURATION STRUCTURE ====================
struct Config {
    int moistureDry;
    int moistureWet;
    int cycleOnTime;
    int cycleSoakTime;
    int cycleRepeat;
    struct Schedule {
        bool enabled;
        int hour;
        int minute;
        bool executed;
    } schedules[4];
    int mode; // 0=OFF, 1=MANUAL, 2=AUTO
    unsigned long fertLastReset;
    char staSSID[32];
    char staPassword[64];
    bool useSTAMode;
} config;

struct Stats {
    unsigned long totalWateringTime;
    int wateringCount;
    float avgMoisture;
    int readingsCount;
} stats = {0, 0, 0, 0};

// ==================== OBJECTS ====================
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(PIN_DHT, DHT11);
AsyncWebServer server(80);
Preferences preferences;

// ==================== GLOBAL VARIABLES ====================
float temperature = 0, humidity = 0;
int moisturePercent = 0, moistureRaw = 0;
bool pumpRunning = false;
unsigned long pumpStartTime = 0, lastSensorRead = 0, lastLCDUpdate = 0;
int currentCycle = 0;
bool inSoakPeriod = false;
unsigned long cycleStartTime = 0;
struct tm timeinfo;
int currentHour = 0, currentMinute = 0;

// ==================== MENU SYSTEM ====================
enum MenuState {
    MENU_MAIN, MENU_MODE, MENU_MOISTURE_THLDS, MENU_CYCLE_SETTINGS,
    MENU_SCHEDULES, MENU_FERTILIZER, MENU_WIFI_CONFIG, 
    MENU_STATISTICS, MENU_MANUAL_CONTROL
};
MenuState currentMenu = MENU_MAIN;
int menuIndex = 0, subMenuIndex = 0;
bool inSubMenu = false, editMode = false;
int editValue = 0, editDigit = 0;

struct Button {
    int pin;
    bool lastState;
    unsigned long lastDebounce;
} buttons[6] = {
    {BTN_UP, HIGH, 0}, {BTN_DOWN, HIGH, 0}, {BTN_LEFT, HIGH, 0},
    {BTN_RIGHT, HIGH, 0}, {BTN_SELECT, HIGH, 0}, {BTN_BACK, HIGH, 0}
};

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== SMART IRRIGATION SYSTEM ===");
    
    pinMode(PIN_PUMP, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_MOISTURE, INPUT);
    for (int i = 0; i < 6; i++) pinMode(buttons[i].pin, INPUT_PULLUP);
    digitalWrite(PIN_PUMP, LOW);
    digitalWrite(PIN_LED, LOW);
    
    Wire.begin(PIN_SDA, PIN_SCL);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Smart Irrigator");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    
    dht.begin();
    loadConfig();
    initWiFi();
    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", "UTC0", 1);
    tzset();
    initWebServer();
    
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_LED, HIGH); delay(200);
        digitalWrite(PIN_LED, LOW); delay(200);
    }
    lcd.clear();
    Serial.println("Setup complete!");
}

// ==================== MAIN LOOP ====================
void loop() {
    unsigned long now = millis();
    
    if (now - lastSensorRead >= SENSOR_READ_MS) {
        lastSensorRead = now;
        readSensors();
        updateStatistics();
    }
    
    updateTime();
    handleButtons();
    
    if (now - lastLCDUpdate >= LCD_REFRESH_MS) {
        lastLCDUpdate = now;
        updateLCD();
    }
    
    handleIrrigation();
    
    if (pumpRunning && (now - pumpStartTime > PUMP_MAX_RUNTIME)) {
        stopPump();
        Serial.println("SAFETY: Pump stopped");
    }
    
    updateLED();
    delay(10);
}

// ==================== SENSOR FUNCTIONS ====================
void readSensors() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
        humidity = h;
        temperature = t;
    }
    
    moistureRaw = analogRead(PIN_MOISTURE);
    moisturePercent = map(constrain(moistureRaw, 1200, 2700), 2700, 1200, 0, 100);
    
    Serial.printf("Moisture: %d%% | Temp: %.1fC | Humidity: %.1f%%\n", 
                  moisturePercent, temperature, humidity);
}

void updateStatistics() {
    stats.avgMoisture = (stats.avgMoisture * stats.readingsCount + moisturePercent) 
                        / (stats.readingsCount + 1);
    stats.readingsCount++;
    if (stats.readingsCount > 1000) stats.readingsCount = 500;
}

// ==================== TIME FUNCTIONS ====================
void updateTime() {
    if (!getLocalTime(&timeinfo)) return;
    
    int newHour = timeinfo.tm_hour;
    int newMinute = timeinfo.tm_min;
    
    if (currentHour == 23 && newHour == 0) {
        for (int i = 0; i < 4; i++) {
            config.schedules[i].executed = false;
        }
        saveConfig();
    }
    currentHour = newHour;
    currentMinute = newMinute;
}

unsigned long getFertilizerDaysLeft() {
    if (config.fertLastReset == 0) return 7;
    unsigned long elapsed = millis() - config.fertLastReset;
    unsigned long daysElapsed = elapsed / (24UL * 60 * 60 * 1000);
    return (daysElapsed >= 7) ? 0 : 7 - daysElapsed;
}

// ==================== PUMP CONTROL ====================
void startPump() {
    if (pumpRunning) return;
    digitalWrite(PIN_PUMP, HIGH);
    pumpRunning = true;
    pumpStartTime = millis();
    stats.wateringCount++;
    Serial.println(">>> PUMP STARTED");
}

void stopPump() {
    if (!pumpRunning) return;
    digitalWrite(PIN_PUMP, LOW);
    pumpRunning = false;
    unsigned long runtime = millis() - pumpStartTime;
    stats.totalWateringTime += runtime;
    Serial.printf("<<< PUMP STOPPED (ran %lu sec)\n", runtime / 1000);
}

// ==================== IRRIGATION LOGIC ====================
void handleIrrigation() {
    if (config.mode == 0) {
        if (pumpRunning && !inSoakPeriod) stopPump();
        return;
    }
    if (config.mode == 1) return; // Manual mode
    
    if (config.mode == 2) { // Auto mode
        for (int i = 0; i < 4; i++) {
            if (config.schedules[i].enabled && 
                !config.schedules[i].executed &&
                currentHour == config.schedules[i].hour &&
                currentMinute == config.schedules[i].minute) {
                
                if (moisturePercent < config.moistureDry) {
                    Serial.printf("Schedule %d triggered!\n", i);
                    startCycleAndSoak();
                    config.schedules[i].executed = true;
                    saveConfig();
                }
            }
        }
        handleCycleAndSoak();
    }
}

void startCycleAndSoak() {
    currentCycle = 0;
    inSoakPeriod = false;
    cycleStartTime = millis();
    startPump();
    Serial.println("=== CYCLE & SOAK STARTED ===");
}

void handleCycleAndSoak() {
    if (currentCycle >= config.cycleRepeat) return;
    
    unsigned long elapsed = (millis() - cycleStartTime) / 1000;
    
    if (!inSoakPeriod) {
        if (elapsed >= config.cycleOnTime) {
            stopPump();
            inSoakPeriod = true;
            cycleStartTime = millis();
            Serial.printf("Cycle %d: Soak period\n", currentCycle + 1);
        }
    } else {
        if (elapsed >= config.cycleSoakTime) {
            currentCycle++;
            if (currentCycle < config.cycleRepeat) {
                inSoakPeriod = false;
                cycleStartTime = millis();
                startPump();
                Serial.printf("Cycle %d/%d\n", currentCycle + 1, config.cycleRepeat);
            } else {
                Serial.println("=== CYCLE COMPLETE ===");
            }
        }
    }
}

// ==================== LED INDICATOR ====================
void updateLED() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    int interval = pumpRunning ? 250 : (config.mode == 2 ? 1000 : 0);
    
    if (interval > 0 && millis() - lastBlink > interval) {
        ledState = !ledState;
        digitalWrite(PIN_LED, ledState);
        lastBlink = millis();
    } else if (interval == 0) {
        digitalWrite(PIN_LED, LOW);
    }
}

// ==================== CONFIGURATION ====================
void loadConfig() {
    preferences.begin("irrigation", false);
    
    config.moistureDry = preferences.getInt("moistDry", 30);
    config.moistureWet = preferences.getInt("moistWet", 70);
    config.cycleOnTime = preferences.getInt("cycleOn", 30);
    config.cycleSoakTime = preferences.getInt("cycleSoak", 60);
    config.cycleRepeat = preferences.getInt("cycleRep", 3);
    config.mode = preferences.getInt("mode", 2);
    config.fertLastReset = preferences.getULong("fertReset", 0);
    
    for (int i = 0; i < 4; i++) {
        String p = "sch" + String(i);
        config.schedules[i].enabled = preferences.getBool((p + "en").c_str(), i < 2);
        config.schedules[i].hour = preferences.getInt((p + "h").c_str(), i == 0 ? 6 : 18);
        config.schedules[i].minute = preferences.getInt((p + "m").c_str(), 0);
        config.schedules[i].executed = preferences.getBool((p + "ex").c_str(), false);
    }
    
    preferences.getString("staSSID", config.staSSID, 32);
    preferences.getString("staPass", config.staPassword, 64);
    config.useSTAMode = preferences.getBool("useSTAMode", false);
    
    stats.totalWateringTime = preferences.getULong("statTime", 0);
    stats.wateringCount = preferences.getInt("statCount", 0);
    
    preferences.end();
    Serial.println("Config loaded");
}

void saveConfig() {
    preferences.begin("irrigation", false);
    
    preferences.putInt("moistDry", config.moistureDry);
    preferences.putInt("moistWet", config.moistureWet);
    preferences.putInt("cycleOn", config.cycleOnTime);
    preferences.putInt("cycleSoak", config.cycleSoakTime);
    preferences.putInt("cycleRep", config.cycleRepeat);
    preferences.putInt("mode", config.mode);
    preferences.putULong("fertReset", config.fertLastReset);
    
    for (int i = 0; i < 4; i++) {
        String p = "sch" + String(i);
        preferences.putBool((p + "en").c_str(), config.schedules[i].enabled);
        preferences.putInt((p + "h").c_str(), config.schedules[i].hour);
        preferences.putInt((p + "m").c_str(), config.schedules[i].minute);
        preferences.putBool((p + "ex").c_str(), config.schedules[i].executed);
    }
    
    preferences.putString("staSSID", config.staSSID);
    preferences.putString("staPass", config.staPassword);
    preferences.putBool("useSTAMode", config.useSTAMode);
    
    preferences.putULong("statTime", stats.totalWateringTime);
    preferences.putInt("statCount", stats.wateringCount);
    
    preferences.end();
    Serial.println("Config saved");
}

// ==================== WIFI ====================
void initWiFi() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("SmartIrrigation", "12345678");
    Serial.println("AP Started");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    if (config.useSTAMode && strlen(config.staSSID) > 0) {
        Serial.printf("Connecting to %s...\n", config.staSSID);
        WiFi.begin(config.staSSID, config.staPassword);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nSTA Connected!");
            Serial.print("STA IP: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("\nSTA Failed");
        }
    }
}

// ==================== WEB SERVER ====================
void initWebServer() {
    // Main page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", getWebPage());
    });
    
    // Status endpoint
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<1024> doc;
        doc["moisture"] = moisturePercent;
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;
        doc["pumpRunning"] = pumpRunning;
        doc["mode"] = config.mode;
        doc["fertDaysLeft"] = getFertilizerDaysLeft();
        doc["currentCycle"] = currentCycle;
        doc["totalCycles"] = config.cycleRepeat;
        doc["inSoak"] = inSoakPeriod;
        doc["wateringCount"] = stats.wateringCount;
        doc["totalTime"] = stats.totalWateringTime / 1000;
        doc["avgMoisture"] = stats.avgMoisture;
        doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
        doc["staIP"] = WiFi.localIP().toString();
        doc["apIP"] = WiFi.softAPIP().toString();
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });
    
    // Config endpoint
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<1024> doc;
        doc["moistureDry"] = config.moistureDry;
        doc["moistureWet"] = config.moistureWet;
        doc["cycleOnTime"] = config.cycleOnTime;
        doc["cycleSoakTime"] = config.cycleSoakTime;
        doc["cycleRepeat"] = config.cycleRepeat;
        doc["mode"] = config.mode;
        
        JsonArray scheds = doc.createNestedArray("schedules");
        for (int i = 0; i < 4; i++) {
            JsonObject s = scheds.createNestedObject();
            s["enabled"] = config.schedules[i].enabled;
            s["hour"] = config.schedules[i].hour;
            s["minute"] = config.schedules[i].minute;
        }
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });
    
    // Save config
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, data);
            
            if (!error) {
                if (doc.containsKey("moistureDry")) config.moistureDry = doc["moistureDry"];
                if (doc.containsKey("moistureWet")) config.moistureWet = doc["moistureWet"];
                if (doc.containsKey("cycleOnTime")) config.cycleOnTime = doc["cycleOnTime"];
                if (doc.containsKey("cycleSoakTime")) config.cycleSoakTime = doc["cycleSoakTime"];
                if (doc.containsKey("cycleRepeat")) config.cycleRepeat = doc["cycleRepeat"];
                
                if (doc.containsKey("schedules")) {
                    JsonArray scheds = doc["schedules"];
                    for (int i = 0; i < 4 && i < scheds.size(); i++) {
                        config.schedules[i].enabled = scheds[i]["enabled"];
                        config.schedules[i].hour = scheds[i]["hour"];
                        config.schedules[i].minute = scheds[i]["minute"];
                    }
                }
                
                saveConfig();
                request->send(200, "application/json", "{\"status\":\"saved\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"parse_error\"}");
            }
        }
    );
    
    // Mode control
    server.on("/mode", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<128> doc;
            if (!deserializeJson(doc, data) && doc.containsKey("mode")) {
                config.mode = doc["mode"];
                if (config.mode == 0) stopPump();
                saveConfig();
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(400);
            }
        }
    );
    
    // Manual pump control
    server.on("/pump", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<128> doc;
            if (!deserializeJson(doc, data) && doc.containsKey("state")) {
                if (config.mode == 1) { // Only in manual mode
                    if (doc["state"] == true) {
                        startPump();
                    } else {
                        stopPump();
                    }
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                } else {
                    request->send(403, "application/json", "{\"error\":\"not_manual_mode\"}");
                }
            } else {
                request->send(400);
            }
        }
    );
    
    // Fertilizer reset
    server.on("/fert/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        config.fertLastReset = millis();
        saveConfig();
        request->send(200, "application/json", "{\"status\":\"reset\"}");
    });
    
    // WiFi config
    server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<256> doc;
            if (!deserializeJson(doc, data)) {
                if (doc.containsKey("ssid")) {
                    strlcpy(config.staSSID, doc["ssid"], sizeof(config.staSSID));
                    strlcpy(config.staPassword, doc["password"] | "", sizeof(config.staPassword));
                    config.useSTAMode = doc["enable"] | false;
                    saveConfig();
                    request->send(200, "application/json", "{\"status\":\"saved\"}");
                    
                    if (config.useSTAMode) {
                        delay(1000);
                        ESP.restart();
                    }
                } else {
                    request->send(400);
                }
            } else {
                request->send(400);
            }
        }
    );
    
    server.begin();
    Serial.println("Web server started");
}

// ==================== BUTTON HANDLING ====================
void handleButtons() {
    for (int i = 0; i < 6; i++) {
        bool reading = digitalRead(buttons[i].pin);
        if (reading != buttons[i].lastState) {
            buttons[i].lastDebounce = millis();
        }
        
        if ((millis() - buttons[i].lastDebounce) > DEBOUNCE_DELAY) {
            if (reading == LOW && buttons[i].lastState == HIGH) {
                handleButtonPress(i);
            }
        }
        buttons[i].lastState = reading;
    }
}

void handleButtonPress(int btn) {
    if (editMode) {
        handleEditMode(btn);
        return;
    }
    
    if (inSubMenu) {
        handleSubMenu(btn);
        return;
    }
    
    // Main menu navigation
    switch (btn) {
        case 0: // UP
            menuIndex = (menuIndex - 1 + 8) % 8;
            break;
        case 1: // DOWN
            menuIndex = (menuIndex + 1) % 8;
            break;
        case 4: // SELECT
            enterMenu();
            break;
    }
}

void enterMenu() {
    switch (menuIndex) {
        case 0: // Mode
            currentMenu = MENU_MODE;
            inSubMenu = true;
            break;
        case 1: // Moisture Thresholds
            currentMenu = MENU_MOISTURE_THLDS;
            inSubMenu = true;
            subMenuIndex = 0;
            break;
        case 2: // Cycle Settings
            currentMenu = MENU_CYCLE_SETTINGS;
            inSubMenu = true;
            subMenuIndex = 0;
            break;
        case 3: // Schedules
            currentMenu = MENU_SCHEDULES;
            inSubMenu = true;
            subMenuIndex = 0;
            break;
        case 4: // Fertilizer
            currentMenu = MENU_FERTILIZER;
            inSubMenu = true;
            break;
        case 5: // WiFi
            currentMenu = MENU_WIFI_CONFIG;
            inSubMenu = true;
            break;
        case 6: // Statistics
            currentMenu = MENU_STATISTICS;
            inSubMenu = true;
            break;
        case 7: // Manual Control
            currentMenu = MENU_MANUAL_CONTROL;
            inSubMenu = true;
            break;
    }
}

void handleSubMenu(int btn) {
    if (btn == 5) { // BACK
        inSubMenu = false;
        currentMenu = MENU_MAIN;
        subMenuIndex = 0;
        return;
    }
    
    switch (currentMenu) {
        case MENU_MODE:
            if (btn == 0) config.mode = (config.mode - 1 + 3) % 3;
            if (btn == 1) config.mode = (config.mode + 1) % 3;
            if (btn == 4) { saveConfig(); inSubMenu = false; currentMenu = MENU_MAIN; }
            break;
            
        case MENU_MOISTURE_THLDS:
            if (btn == 0) subMenuIndex = (subMenuIndex - 1 + 2) % 2;
            if (btn == 1) subMenuIndex = (subMenuIndex + 1) % 2;
            if (btn == 4) {
                editMode = true;
                editValue = (subMenuIndex == 0) ? config.moistureDry : config.moistureWet;
                editDigit = 0;
            }
            break;
            
        case MENU_CYCLE_SETTINGS:
            if (btn == 0) subMenuIndex = (subMenuIndex - 1 + 3) % 3;
            if (btn == 1) subMenuIndex = (subMenuIndex + 1) % 3;
            if (btn == 4) {
                editMode = true;
                if (subMenuIndex == 0) editValue = config.cycleOnTime;
                else if (subMenuIndex == 1) editValue = config.cycleSoakTime;
                else editValue = config.cycleRepeat;
                editDigit = 0;
            }
            break;
            
        case MENU_SCHEDULES:
            if (btn == 0) subMenuIndex = (subMenuIndex - 1 + 4) % 4;
            if (btn == 1) subMenuIndex = (subMenuIndex + 1) % 4;
            if (btn == 2) config.schedules[subMenuIndex].enabled = !config.schedules[subMenuIndex].enabled;
            if (btn == 4) {
                editMode = true;
                editValue = config.schedules[subMenuIndex].hour * 100 + config.schedules[subMenuIndex].minute;
                editDigit = 0;
            }
            break;
            
        case MENU_FERTILIZER:
            if (btn == 4) {
                config.fertLastReset = millis();
                saveConfig();
                inSubMenu = false;
                currentMenu = MENU_MAIN;
            }
            break;
            
        case MENU_MANUAL_CONTROL:
            if (config.mode == 1) {
                if (btn == 4) {
                    if (pumpRunning) stopPump();
                    else startPump();
                }
            }
            break;
    }
}

void handleEditMode(int btn) {
    if (btn == 5) { // BACK - cancel
        editMode = false;
        return;
    }
    
    if (btn == 4) { // SELECT - save
        switch (currentMenu) {
            case MENU_MOISTURE_THLDS:
                if (subMenuIndex == 0) config.moistureDry = editValue;
                else config.moistureWet = editValue;
                break;
            case MENU_CYCLE_SETTINGS:
                if (subMenuIndex == 0) config.cycleOnTime = editValue;
                else if (subMenuIndex == 1) config.cycleSoakTime = editValue;
                else config.cycleRepeat = editValue;
                break;
            case MENU_SCHEDULES:
                config.schedules[subMenuIndex].hour = editValue / 100;
                config.schedules[subMenuIndex].minute = editValue % 100;
                break;
        }
        saveConfig();
        editMode = false;
        return;
    }
    
    if (btn == 0) editValue++; // UP
    if (btn == 1) editValue--; // DOWN
    
    // Constrain values
    if (currentMenu == MENU_MOISTURE_THLDS) {
        editValue = constrain(editValue, 0, 100);
    } else if (currentMenu == MENU_CYCLE_SETTINGS) {
        if (subMenuIndex < 2) editValue = constrain(editValue, 1, 999);
        else editValue = constrain(editValue, 1, 10);
    } else if (currentMenu == MENU_SCHEDULES) {
        int h = editValue / 100;
        int m = editValue % 100;
        h = constrain(h, 0, 23);
        m = constrain(m, 0, 59);
        editValue = h * 100 + m;
    }
}

// ==================== LCD DISPLAY ====================
void updateLCD() {
    lcd.clear();
    
    if (currentMenu == MENU_MAIN) {
        displayMainMenu();
    } else {
        displaySubMenu();
    }
}

void displayMainMenu() {
    const char* menuItems[] = {
        "Mode", "Moisture", "Cycle&Soak", "Schedules",
        "Fertilizer", "WiFi", "Statistics", "Manual Ctrl"
    };
    
    lcd.setCursor(0, 0);
    lcd.print(">");
    lcd.print(menuItems[menuIndex]);
    
    lcd.setCursor(0, 1);
    if (!inSubMenu) {
        lcd.printf("M:%d%%T:%.0fC H:%.0f%%", moisturePercent, temperature, humidity);
    }
}

void displaySubMenu() {
    lcd.setCursor(0, 0);
    
    switch (currentMenu) {
        case MENU_MODE:
            lcd.print("Mode:");
            lcd.setCursor(0, 1);
            lcd.print(config.mode == 0 ? "OFF" : (config.mode == 1 ? "MANUAL" : "AUTO"));
            break;
            
        case MENU_MOISTURE_THLDS:
            lcd.print(subMenuIndex == 0 ? ">Dry:" : " Dry:");
            lcd.print(config.moistureDry);
            lcd.print("%");
            lcd.setCursor(0, 1);
            lcd.print(subMenuIndex == 1 ? ">Wet:" : " Wet:");
            lcd.print(config.moistureWet);
            lcd.print("%");
            if (editMode) {
                lcd.setCursor(6, subMenuIndex);
                lcd.print(editValue);
                lcd.print("  ");
            }
            break;
            
        case MENU_CYCLE_SETTINGS:
            if (subMenuIndex == 0) {
                lcd.print(">On Time:");
                lcd.setCursor(0, 1);
                lcd.print(config.cycleOnTime);
                lcd.print("s");
            } else if (subMenuIndex == 1) {
                lcd.print(">Soak Time:");
                lcd.setCursor(0, 1);
                lcd.print(config.cycleSoakTime);
                lcd.print("s");
            } else {
                lcd.print(">Cycles:");
                lcd.setCursor(0, 1);
                lcd.print(config.cycleRepeat);
            }
            if (editMode) {
                lcd.setCursor(0, 1);
                lcd.print(editValue);
                lcd.print("  ");
            }
            break;
            
        case MENU_SCHEDULES:
            lcd.print("Schedule ");
            lcd.print(subMenuIndex + 1);
            lcd.print(":");
            lcd.setCursor(0, 1);
            lcd.printf("%s %02d:%02d", 
                config.schedules[subMenuIndex].enabled ? "ON " : "OFF",
                config.schedules[subMenuIndex].hour,
                config.schedules[subMenuIndex].minute);
            if (editMode) {
                lcd.setCursor(4, 1);
                lcd.printf("%02d:%02d", editValue / 100, editValue % 100);
            }
            break;
            
        case MENU_FERTILIZER:
            lcd.print("Fertilizer");
            lcd.setCursor(0, 1);
            lcd.print("Days left: ");
            lcd.print(getFertilizerDaysLeft());
            break;
            
        case MENU_WIFI_CONFIG:
            lcd.print("WiFi Config");
            lcd.setCursor(0, 1);
            if (WiFi.status() == WL_CONNECTED) {
                lcd.print("Connected!");
            } else {
                lcd.print("Use Web UI");
            }
            break;
            
        case MENU_STATISTICS:
            lcd.print("Stats");
            lcd.setCursor(0, 1);
            lcd.printf("Cnt:%d T:%lum", stats.wateringCount, stats.totalWateringTime / 60000);
            break;
            
        case MENU_MANUAL_CONTROL:
            lcd.print("Manual Control");
            lcd.setCursor(0, 1);
            if (config.mode != 1) {
                lcd.print("Set MANUAL mode");
            } else {
                lcd.print(pumpRunning ? "PUMP ON " : "PUMP OFF");
            }
            break;
    }
}

// ==================== WEB PAGE ====================
String getWebPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Irrigation System</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        .status-bar {
            display: flex;
            justify-content: space-around;
            background: #f8f9fa;
            padding: 20px;
            flex-wrap: wrap;
        }
        .status-item {
            text-align: center;
            padding: 15px;
            background: white;
            border-radius: 10px;
            margin: 5px;
            min-width: 150px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .status-value {
            font-size: 2em;
            font-weight: bold;
            color: #667eea;
            margin: 10px 0;
        }
        .status-label {
            color: #666;
            font-size: 0.9em;
        }
        .content {
            padding: 30px;
        }
        .section {
            margin-bottom: 30px;
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
        }
        .section h2 {
            color: #1e3c72;
            margin-bottom: 20px;
            border-bottom: 3px solid #667eea;
            padding-bottom: 10px;
        }
        .control-group {
            margin: 15px 0;
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 10px;
            background: white;
            border-radius: 5px;
        }
        label {
            font-weight: 500;
            color: #333;
        }
        input[type="number"], input[type="text"], input[type="password"], select {
            padding: 10px;
            border: 2px solid #ddd;
            border-radius: 5px;
            font-size: 1em;
            width: 150px;
        }
        input[type="checkbox"] {
            width: 20px;
            height: 20px;
            cursor: pointer;
        }
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 25px;
            cursor: pointer;
            font-size: 1em;
            font-weight: bold;
            transition: transform 0.2s;
        }
        button:hover {
            transform: scale(1.05);
        }
        button:active {
            transform: scale(0.95);
        }
        .btn-danger {
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
        }
        .btn-success {
            background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
        }
        .pump-status {
            text-align: center;
            padding: 30px;
            font-size: 1.5em;
        }
        .pump-on {
            color: #00f2fe;
            animation: pulse 1s infinite;
        }
        .pump-off {
            color: #999;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .schedule-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 15px;
        }
        .schedule-card {
            background: white;
            padding: 15px;
            border-radius: 10px;
            border: 2px solid #ddd;
        }
        .schedule-card.active {
            border-color: #667eea;
            background: #f0f4ff;
        }
        .fertilizer-alert {
            background: #fff3cd;
            border: 2px solid #ffc107;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            font-size: 1.2em;
        }
        .fertilizer-alert.warning {
            background: #f8d7da;
            border-color: #dc3545;
        }
        @media (max-width: 768px) {
            .status-bar { flex-direction: column; }
            .control-group { flex-direction: column; align-items: flex-start; }
            input[type="number"], input[type="text"], select { width: 100%; margin-top: 10px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌱 Smart Irrigation System</h1>
            <p>Automated Garden Watering Control</p>
        </div>
        
        <div class="status-bar">
            <div class="status-item">
                <div class="status-label">Soil Moisture</div>
                <div class="status-value" id="moisture">--%</div>
            </div>
            <div class="status-item">
                <div class="status-label">Temperature</div>
                <div class="status-value" id="temp">--°C</div>
            </div>
            <div class="status-item">
                <div class="status-label">Humidity</div>
                <div class="status-value" id="humidity">--%</div>
            </div>
            <div class="status-item">
                <div class="status-label">Mode</div>
                <div class="status-value" id="mode">--</div>
            </div>
        </div>
        
        <div class="content">
            <!-- Pump Status -->
            <div class="section">
                <h2>💧 Pump Status</h2>
                <div class="pump-status">
                    <span id="pumpStatus" class="pump-off">PUMP OFF</span>
                </div>
                <div style="text-align: center;">
                    <p>Cycle: <span id="cycle">0</span> / <span id="totalCycles">0</span></p>
                    <p>Phase: <span id="phase">Idle</span></p>
                </div>
            </div>
            
            <!-- Fertilizer Reminder -->
            <div class="section">
                <h2>🌿 Fertilizer Reminder</h2>
                <div id="fertAlert" class="fertilizer-alert">
                    Days until next fertilizer: <strong id="fertDays">7</strong>
                </div>
                <div style="text-align: center; margin-top: 15px;">
                    <button onclick="resetFertilizer()">✓ Fertilizer Added</button>
                </div>
            </div>
            
            <!-- Operation Mode -->
            <div class="section">
                <h2>⚙️ Operation Mode</h2>
                <div class="control-group">
                    <label>Select Mode:</label>
                    <select id="modeSelect" onchange="changeMode()">
                        <option value="0">OFF</option>
                        <option value="1">MANUAL</option>
                        <option value="2">AUTO</option>
                    </select>
                </div>
                <div id="manualControls" style="display: none; text-align: center; margin-top: 15px;">
                    <button id="pumpBtn" onclick="togglePump()">START PUMP</button>
                </div>
            </div>
            
            <!-- Moisture Thresholds -->
            <div class="section">
                <h2>💧 Moisture Thresholds</h2>
                <div class="control-group">
                    <label>Dry Threshold (%):</label>
                    <input type="number" id="moistDry" min="0" max="100" value="30">
                </div>
                <div class="control-group">
                    <label>Wet Threshold (%):</label>
                    <input type="number" id="moistWet" min="0" max="100" value="70">
                </div>
            </div>
            
            <!-- Cycle & Soak Settings -->
            <div class="section">
                <h2>🔄 Cycle & Soak Settings</h2>
                <div class="control-group">
                    <label>Water Time (seconds):</label>
                    <input type="number" id="cycleOn" min="1" max="300" value="30">
                </div>
                <div class="control-group">
                    <label>Soak Time (seconds):</label>
                    <input type="number" id="cycleSoak" min="1" max="300" value="60">
                </div>
                <div class="control-group">
                    <label>Number of Cycles:</label>
                    <input type="number" id="cycleRepeat" min="1" max="10" value="3">
                </div>
            </div>
            
            <!-- Schedules -->
            <div class="section">
                <h2>📅 Watering Schedules</h2>
                <div class="schedule-grid">
                    <div class="schedule-card" id="sched0">
                        <h3>Schedule 1</h3>
                        <div style="margin: 10px 0;">
                            <label><input type="checkbox" id="sch0en"> Enabled</label>
                        </div>
                        <div>
                            <label>Time:</label>
                            <input type="number" id="sch0h" min="0" max="23" style="width: 60px;"> :
                            <input type="number" id="sch0m" min="0" max="59" style="width: 60px;">
                        </div>
                    </div>
                    <div class="schedule-card" id="sched1">
                        <h3>Schedule 2</h3>
                        <div style="margin: 10px 0;">
                            <label><input type="checkbox" id="sch1en"> Enabled</label>
                        </div>
                        <div>
                            <label>Time:</label>
                            <input type="number" id="sch1h" min="0" max="23" style="width: 60px;"> :
                            <input type="number" id="sch1m" min="0" max="59" style="width: 60px;">
                        </div>
                    </div>
                    <div class="schedule-card" id="sched2">
                        <h3>Schedule 3</h3>
                        <div style="margin: 10px 0;">
                            <label><input type="checkbox" id="sch2en"> Enabled</label>
                        </div>
                        <div>
                            <label>Time:</label>
                            <input type="number" id="sch2h" min="0" max="23" style="width: 60px;"> :
                            <input type="number" id="sch2m" min="0" max="59" style="width: 60px;">
                        </div>
                    </div>
                    <div class="schedule-card" id="sched3">
                        <h3>Schedule 4</h3>
                        <div style="margin: 10px 0;">
                            <label><input type="checkbox" id="sch3en"> Enabled</label>
                        </div>
                        <div>
                            <label>Time:</label>
                            <input type="number" id="sch3h" min="0" max="23" style="width: 60px;"> :
                            <input type="number" id="sch3m" min="0" max="59" style="width: 60px;">
                        </div>
                    </div>
                </div>
            </div>
            
            <!-- Statistics -->
            <div class="section">
                <h2>📊 Statistics</h2>
                <div class="control-group">
                    <label>Total Watering Events:</label>
                    <strong id="statCount">0</strong>
                </div>
                <div class="control-group">
                    <label>Total Water Time:</label>
                    <strong id="statTime">0 minutes</strong>
                </div>
                <div class="control-group">
                    <label>Average Moisture:</label>
                    <strong id="statAvg">0%</strong>
                </div>
            </div>
            
            <!-- WiFi Configuration -->
            <div class="section">
                <h2>📡 WiFi Configuration</h2>
                <div class="control-group">
                    <label>Network SSID:</label>
                    <input type="text" id="wifiSSID" placeholder="Your WiFi Name">
                </div>
                <div class="control-group">
                    <label>Password:</label>
                    <input type="password" id="wifiPass" placeholder="Password">
                </div>
                <div class="control-group">
                    <label><input type="checkbox" id="wifiEnable"> Enable Station Mode</label>
                </div>
                <div style="text-align: center; margin-top: 15px;">
                    <button onclick="saveWiFi()">Save WiFi (Device Will Restart)</button>
                </div>
                <div style="margin-top: 15px; padding: 10px; background: white; border-radius: 5px;">
                    <p><strong>AP IP:</strong> <span id="apIP">-</span></p>
                    <p><strong>STA IP:</strong> <span id="staIP">-</span></p>
                    <p><strong>Status:</strong> <span id="wifiStatus">Checking...</span></p>
                </div>
            </div>
            
            <!-- Save Button -->
            <div style="text-align: center; margin-top: 30px;">
                <button onclick="saveConfig()" class="btn-success" style="font-size: 1.2em; padding: 15px 50px;">
                    💾 SAVE ALL SETTINGS
                </button>
            </div>
        </div>
    </div>

    <script>
        let currentConfig = {};
        
        // Load configuration on page load
        window.onload = function() {
            loadConfig();
            updateStatus();
            setInterval(updateStatus, 2000);
        };
        
        function loadConfig() {
            fetch('/config')
                .then(response => response.json())
                .then(data => {
                    currentConfig = data;
                    document.getElementById('moistDry').value = data.moistureDry;
                    document.getElementById('moistWet').value = data.moistureWet;
                    document.getElementById('cycleOn').value = data.cycleOnTime;
                    document.getElementById('cycleSoak').value = data.cycleSoakTime;
                    document.getElementById('cycleRepeat').value = data.cycleRepeat;
                    document.getElementById('modeSelect').value = data.mode;
                    
                    for (let i = 0; i < 4; i++) {
                        document.getElementById('sch' + i + 'en').checked = data.schedules[i].enabled;
                        document.getElementById('sch' + i + 'h').value = data.schedules[i].hour;
                        document.getElementById('sch' + i + 'm').value = data.schedules[i].minute;
                        
                        if (data.schedules[i].enabled) {
                            document.getElementById('sched' + i).classList.add('active');
                        }
                    }
                    
                    updateModeUI();
                });
        }
        
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('moisture').textContent = data.moisture + '%';
                    document.getElementById('temp').textContent = data.temperature.toFixed(1) + '°C';
                    document.getElementById('humidity').textContent = data.humidity.toFixed(1) + '%';
                    
                    const modeText = ['OFF', 'MANUAL', 'AUTO'][data.mode];
                    document.getElementById('mode').textContent = modeText;
                    
                    const pumpStatus = document.getElementById('pumpStatus');
                    if (data.pumpRunning) {
                        pumpStatus.textContent = '💧 PUMP RUNNING';
                        pumpStatus.className = 'pump-on';
                    } else {
                        pumpStatus.textContent = 'PUMP OFF';
                        pumpStatus.className = 'pump-off';
                    }
                    
                    document.getElementById('cycle').textContent = data.currentCycle;
                    document.getElementById('totalCycles').textContent = data.totalCycles;
                    document.getElementById('phase').textContent = data.inSoak ? 'Soaking' : (data.pumpRunning ? 'Watering' : 'Idle');
                    
                    // Fertilizer
                    const fertDays = data.fertDaysLeft;
                    document.getElementById('fertDays').textContent = fertDays;
                    const fertAlert = document.getElementById('fertAlert');
                    if (fertDays <= 1) {
                        fertAlert.className = 'fertilizer-alert warning';
                        fertAlert.innerHTML = '⚠️ Time to add fertilizer! Days left: <strong>' + fertDays + '</strong>';
                    } else {
                        fertAlert.className = 'fertilizer-alert';
                        fertAlert.innerHTML = 'Days until next fertilizer: <strong>' + fertDays + '</strong>';
                    }
                    
                    // Statistics
                    document.getElementById('statCount').textContent = data.wateringCount;
                    document.getElementById('statTime').textContent = Math.round(data.totalTime / 60) + ' minutes';
                    document.getElementById('statAvg').textContent = data.avgMoisture.toFixed(1) + '%';
                    
                    // WiFi
                    document.getElementById('apIP').textContent = data.apIP;
                    document.getElementById('staIP').textContent = data.staIP;
                    document.getElementById('wifiStatus').textContent = data.wifiConnected ? '✓ Connected' : '✗ Not Connected';
                    
                    // Update pump button
                    if (data.mode === 1) {
                        const pumpBtn = document.getElementById('pumpBtn');
                        if (data.pumpRunning) {
                            pumpBtn.textContent = 'STOP PUMP';
                            pumpBtn.className = 'btn-danger';
                        } else {
                            pumpBtn.textContent = 'START PUMP';
                            pumpBtn.className = '';
                        }
                    }
                });
        }
        
        function changeMode() {
            const mode = parseInt(document.getElementById('modeSelect').value);
            fetch('/mode', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ mode: mode })
            }).then(() => {
                updateModeUI();
                alert('Mode changed successfully!');
            });
        }
        
        function updateModeUI() {
            const mode = parseInt(document.getElementById('modeSelect').value);
            const manualControls = document.getElementById('manualControls');
            if (mode === 1) {
                manualControls.style.display = 'block';
            } else {
                manualControls.style.display = 'none';
            }
        }
        
        function togglePump() {
            const currentState = document.getElementById('pumpBtn').textContent.includes('STOP');
            fetch('/pump', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ state: !currentState })
            }).then(() => {
                updateStatus();
            });
        }
        
        function saveConfig() {
            const config = {
                moistureDry: parseInt(document.getElementById('moistDry').value),
                moistureWet: parseInt(document.getElementById('moistWet').value),
                cycleOnTime: parseInt(document.getElementById('cycleOn').value),
                cycleSoakTime: parseInt(document.getElementById('cycleSoak').value),
                cycleRepeat: parseInt(document.getElementById('cycleRepeat').value),
                schedules: []
            };
            
            for (let i = 0; i < 4; i++) {
                config.schedules.push({
                    enabled: document.getElementById('sch' + i + 'en').checked,
                    hour: parseInt(document.getElementById('sch' + i + 'h').value),
                    minute: parseInt(document.getElementById('sch' + i + 'm').value)
                });
            }
            
            fetch('/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            }).then(response => response.json())
              .then(data => {
                  alert('✓ Configuration saved successfully!');
                  loadConfig();
              });
        }
        
        function resetFertilizer() {
            if (confirm('Mark fertilizer as added?')) {
                fetch('/fert/reset', { method: 'POST' })
                    .then(() => {
                        alert('✓ Fertilizer timer reset!');
                        updateStatus();
                    });
            }
        }
        
        function saveWiFi() {
            const config = {
                ssid: document.getElementById('wifiSSID').value,
                password: document.getElementById('wifiPass').value,
                enable: document.getElementById('wifiEnable').checked
            };
            
            if (!config.ssid && config.enable) {
                alert('Please enter WiFi SSID');
                return;
            }
            
            if (confirm('Save WiFi settings? Device will restart.')) {
                fetch('/wifi', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                }).then(() => {
                    alert('WiFi settings saved. Device restarting...');
                });
            }
        }
    </script>
</body>
</html>
)rawliteral";
}