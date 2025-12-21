#include <Meo3_Device.h>

#define LED_BUILTIN 2

MeoDevice meo;

// Example feature callback
void onTurnOn(const MeoFeatureCall& call) {
    Serial.println("Feature 'turn_on' invoked");
    Serial.println("Viet Ngu");
    // Trigger turn on Module LED
    digitalWrite(LED_BUILTIN, HIGH);
    for (const auto& kv : call.params) {
        Serial.print("  ");
        Serial.print(kv.first);
        Serial.print(" = ");
        Serial.println(kv.second);
    }
    meo.sendFeatureResponse(call, true, "Turned on");
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
    delay(2000);

    meo.setLogger(meoLogger);

    meo.beginWifi("Maker IoT", "langmaker");
    meo.begin("meo-open-service.local", 1883); // optional, dont need to modify
    // meo.begin("192.168.100.248", 1883);

    // optionally set gateway ports for mDNS fallback or direct IP
    // meo.setGateway("192.168.100.248", 8901, 1883);


    meo.setDeviceInfo("DIY Sensor", "Test MEO Module", "ThingAI Lab", MeoConnectionType::LAN);

    meo.addFeatureEvent("sensor_update");
    meo.addFeatureMethod("turn_on", onTurnOn);

    meo.start();
}

void loop() {
    meo.loop();

    static unsigned long last = 0;
    if (millis() - last > 5000) {
        last = millis();
        MeoEventPayload p;
        p["temperature"] = "23.5";
        p["humidity"] = "50";
        meo.publishEvent("sensor_update", p);
    }
}