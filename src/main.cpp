#include <Arduino.h>
#include <Meo3_Device.h>

#define LED_BUILTIN 8

MeoDevice meo;

// Example feature callback
void onTurnOn(const MeoFeatureCall& call) {
    Serial.println("Feature 'turn_on' invoked");
    // Trigger turn on Module LED
    digitalWrite(LED_BUILTIN, HIGH);

    int first;
    int second;

    for (const auto& kv : call.params) {
        Serial.print("  ");
        Serial.print(kv.first); // print key
        Serial.print(" = ");
        Serial.println(kv.second); // print value

        if (kv.first == "first") {
            first = kv.second.toInt();
        } else if (kv.first == "second") {
            second = kv.second.toInt();
        }
    }

    int sum = first + second;
    Serial.print("Sum: ");
    Serial.println(sum);

    String msg = "Sum is " + String(sum);

    meo.sendFeatureResponse(call, true, msg.c_str());
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

    pinMode(LED_BUILTIN, OUTPUT);

    meo.setLogger(meoLogger);

    meo.beginWifi("Maker IoT", "langmaker");
    meo.begin("meo-open-service.local", 1883); // optional, dont need to modify
    // meo.begin("192.168.100.248", 1883);

    // optionally set gateway ports for mDNS fallback or direct IP
    // meo.setGateway("192.168.100.248", 8901, 1883);


    meo.setDeviceInfo("DIY Sensor", "Test MEO Module", "ThingAI Lab", MeoConnectionType::LAN);

    meo.addFeatureEvent("humid_temp_update");
    meo.addFeatureMethod("turn_on_led", onTurnOn);

    meo.start();
}


void generateRandomPayload(MeoEventPayload& payload) {
    int temperature = random(200, 300) / 10; // 20.0 to 30.0
    int humidity = random(400, 600) / 10;    // 40.0 to 60.0
    payload["temperature"] = String(temperature);
    payload["humidity"] = String(humidity);
}

void loop() {
    meo.loop();

    static unsigned long last = 0;
    if (millis() - last > 5000) {
        last = millis();
        MeoEventPayload p;
        generateRandomPayload(p);
        meo.publishEvent("humid_temp_update", p);
    }
}