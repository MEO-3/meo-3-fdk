# MEO 3 Library for Arduino Framework (platform: espressif32)

This is the official MEO 3 library for Arduino framework, designed to facilitate seamless integration of DIY devices with the MEO platform. This library provides essential functionalities to connect, communicate, and manage IoT devices effectively.

## Features
- Easy connection to MEO platform
- MQTT communication support
- JSON data handling
- Device management utilities
- Compatible with ESP32-based boards

## Installation

Currently, the library can be installed manually by downloading the source code and adding it to your Arduino libraries folder.
1. Download the library source code from the repository.
2. Extract the contents to your Arduino libraries directory (usually located at `Documents/Arduino/libraries`).
3. Restart the Arduino IDE to recognize the new library. 


## Usage Guide

This library simplifies connecting ESP32 and other Arduino-framework devices to the MEO Open Service platform. It handles WiFi connection, device registration, and MQTT communication automatically.

### 1. Include the Library

```cpp
#include <Meo3_Device.h>
#include <WiFi.h> // Ensure WiFi is included for ESP32
```

### 2. Create the Device Object

Instantiate the `MeoDevice` class globally.

```cpp
MeoDevice meo;
```

### 3. Define Feature Callbacks (Optional)

If your device needs to **receive commands** (e.g., turn on a light), define a callback function. This function is called whenever the MEO platform sends a command to your device.

```cpp
void onTurnOn(const MeoFeatureCall& call) {
    Serial.println("Received 'turn_on' command!");

    // Access parameters sent from the platform
    for (const auto& kv : call.params) {
        Serial.print(kv.first); // Parameter Key
        Serial.print(" = ");
        Serial.println(kv.second); // Parameter Value
    }

    // Perform your hardware action here
    digitalWrite(LED_BUILTIN, HIGH);

    // Send a response back to the platform
    meo.sendFeatureResponse(call, true, "Device turned on successfully");
}
```

### 4. Setup

In your `setup()` function, configure the connection and register your device's features.

```cpp
void setup() {
    Serial.begin(115200);

    // 1. (Optional) Enable logging to Serial for debugging
    meo.setLogger([](const char* level, const char* msg) {
        Serial.printf("[%s] %s\n", level, msg);
    });

    // 2. Connect to WiFi
    meo.beginWifi("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");

    // 3. Set the Gateway Address
    // Use the local DNS name or IP address of your MEO Gateway
    meo.begin("meo-open-service.local", 1883); 

    // 4. Define Device Info (Metadata)
    // Label, Model, Manufacturer, Connection Type
    meo.setDeviceInfo("My Smart Light", "Model-X1", "Maker Lab", MeoConnectionType::LAN);

    // 5. Register Features
    // Events: Data you send TO the platform (e.g., sensor readings)
    meo.addFeatureEvent("sensor_update");
    
    // Methods: Commands you receive FROM the platform (e.g., control)
    meo.addFeatureMethod("turn_on", onTurnOn);

    // 6. Start the Service
    // This handles registration and MQTT connection automatically
    meo.start();
}
```

### 5. Loop

You must call `meo.loop()` in your main loop to keep the connection alive and process incoming messages.

```cpp
void loop() {
    // Keep the library running
    meo.loop();

    // Example: Periodically publish sensor data
    static unsigned long lastTime = 0;
    if (millis() - lastTime > 5000) {
        lastTime = millis();

        // Create a payload map
        MeoEventPayload payload;
        payload["temperature"] = "25.5";
        payload["humidity"] = "60";

        // Publish the event
        if (meo.isMqttConnected()) {
            meo.publishEvent("sensor_update", payload);
            Serial.println("Sensor data sent.");
        }
    }
}
```

---

## API Reference

### Configuration

* **`void beginWifi(const char* ssid, const char* pass)`**: Connects the ESP32 to the specified WiFi network.
* **`void begin(const char* host, uint16_t mqttPort)`**: Sets the MEO Gateway address and MQTT port (default 1883).
* **`void setDeviceInfo(label, model, manufacturer, type)`**: Sets the metadata that will be displayed in the MEO Dashboard.

### Features & Registration

* **`void addFeatureEvent(const char* name)`**: Registers an event (data stream) that this device will publish.
* **`void addFeatureMethod(const char* name, MeoFeatureCallback cb)`**: Registers a command that this device can receive. The `cb` function is triggered when the command arrives.

### Runtime

* **`bool start()`**: Initiates the registration process. If the device is new, it registers with the gateway. If it's already registered, it loads credentials from storage.
* **`void loop()`**: Handles background tasks (MQTT keep-alive, incoming messages). Must be called frequently.
* **`bool publishEvent(const char* eventName, MeoEventPayload payload)`**: Sends data to the platform.
* **`bool sendFeatureResponse(call, success, message)`**: Replies to a method call, indicating if the command was successful.