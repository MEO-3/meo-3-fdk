#include <Arduino.h>
#include <Meo3_Device.h>

#define LED_PIN 7

MeoDevice meo;

// Example feature callback
void onTurnOn(const MeoFeatureCall& call) {
    Serial.println("Feature 'turn_on_led' invoked");
    digitalWrite(LED_PIN, HIGH);

    int first = 0, second = 0;
    for (const auto& kv : call.params) {
        Serial.printf("  %s = %s\n", kv.first.c_str(), kv.second.c_str());
        if (kv.first == "first")  first  = atoi(kv.second.c_str());
        if (kv.first == "second") second = atoi(kv.second.c_str());
    }
    char msg[64];
    snprintf(msg, sizeof(msg), "LED on, sum=%d", first + second);
    meo.sendFeatureResponse(call, true, msg);
}

// Optional logger
void meoLogger(const char* level, const char* message) {
    Serial.print("[");
    Serial.print(level);
    Serial.print("] ");
    Serial.println(message);
}

void setup() {
    Serial.begin(115200);
    
    delay(1000);
    Serial.println("Starting MEO Device...");
    
    pinMode(LED_PIN, OUTPUT);

    meo.setLogger(meoLogger);
    meo.setDeviceInfo("MEO Device", "ThingAI");
    meo.setCloudCompatibleInfo("product-1234", "build-20240601");
    meo.setGateway("192.168.0.106", 1883);
    meo.setDebugTags("DEVICE,MQTT,PROV");

    meo.addFeatureMethod("turn_on_led", onTurnOn);
    meo.addFeatureEvent("humid_temp_update");

    meo.start();
}

void loop() {
    meo.loop();

    static uint32_t last = 0;
    if (millis() - last > 5000 && meo.isMqttConnected()) {
        last = millis();
        MeoEventPayload p;
        p["temperature"] = std::to_string(random(200, 300) / 10);
        p["humidity"]    = std::to_string(random(400, 600) / 10);
        bool success = meo.publishEvent("humid_temp_update", p);
        meoLogger("INFO", success ? "Published humid_temp_update event" : "Failed to publish event");
    }
}