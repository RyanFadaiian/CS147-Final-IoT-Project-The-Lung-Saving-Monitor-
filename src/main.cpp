#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "DHT.h"
#include <ArduinoJson.h>
#include <Adafruit_PM25AQI.h>
#include "secrets.h"

// ----------------------------
// DHT11 sensor configuration
// ----------------------------
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ----------------------------
// PMS5003 Sensor
// ----------------------------
Adafruit_PM25AQI aqi;
HardwareSerial pmsSerial(1); // UART1

// ----------------------------
// RED LED for air quality alert
// ----------------------------
#define RED_LED_PIN 2   // D2 on ESP32 = GPIO2

// ----------------------------
// Azure IoT Hub Root CA Cert
// ----------------------------
const char* root_ca = 
"-----BEGIN CERTIFICATE-----\n"
"MIIEtjCCA56gAwIBAgIQCv1eRG9c89YADp5Gwibf9jANBgkqhkiG9w0BAQsFADBh\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
"MjAeFw0yMjA0MjgwMDAwMDBaFw0zMjA0MjcyMzU5NTlaMEcxCzAJBgNVBAYTAlVT\n"
"MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xGDAWBgNVBAMTD01TRlQg\n"
"UlMyNTYgQ0EtMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMiJV34o\n"
"eVNHI0mZGh1Rj9mdde3zSY7IhQNqAmRaTzOeRye8QsfhYFXSiMW25JddlcqaqGJ9\n"
"GEMcJPWBIBIEdNVYl1bB5KQOl+3m68p59Pu7npC74lJRY8F+p8PLKZAJjSkDD9Ex\n"
"mjHBlPcRrasgflPom3D0XB++nB1y+WLn+cB7DWLoj6qZSUDyWwnEDkkjfKee6ybx\n"
"SAXq7oORPe9o2BKfgi7dTKlOd7eKhotw96yIgMx7yigE3Q3ARS8m+BOFZ/mx150g\n"
"dKFfMcDNvSkCpxjVWnk//icrrmmEsn2xJbEuDCvtoSNvGIuCXxqhTM352HGfO2JK\n"
"AF/Kjf5OrPn2QpECAwEAAaOCAYIwggF+MBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYD\n"
"VR0OBBYEFAyBfpQ5X8d3on8XFnk46DWWjn+UMB8GA1UdIwQYMBaAFE4iVCAYlebj\n"
"buYP+vq5Eu0GF485MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcD\n"
"AQYIKwYBBQUHAwIwdgYIKwYBBQUHAQEEajBoMCQGCCsGAQUFBzABhhhodHRwOi8v\n"
"b2NzcC5kaWdpY2VydC5jb20wQAYIKwYBBQUHMAKGNGh0dHA6Ly9jYWNlcnRzLmRp\n"
"Z2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RHMi5jcnQwQgYDVR0fBDswOTA3\n"
"oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9v\n"
"dEcyLmNybDA9BgNVHSAENjA0MAsGCWCGSAGG/WwCATAHBgVngQwBATAIBgZngQwB\n"
"AgEwCAYGZ4EMAQICMAgGBmeBDAECAzANBgkqhkiG9w0BAQsFAAOCAQEAdYWmf+AB\n"
"klEQShTbhGPQmH1c9BfnEgUFMJsNpzo9dvRj1Uek+L9WfI3kBQn97oUtf25BQsfc\n"
"kIIvTlE3WhA2Cg2yWLTVjH0Ny03dGsqoFYIypnuAwhOWUPHAu++vaUMcPUTUpQCb\n"
"eC1h4YW4CCSTYN37D2Q555wxnni0elPj9O0pymWS8gZnsfoKjvoYi/qDPZw1/TSR\n"
"penOgI6XjmlmPLBrk4LIw7P7PPg4uXUpCzzeybvARG/NIIkFv1eRYIbDF+bIkZbJ\n"
"QFdB9BjjlA4ukAg2YkOyCiB8eXTBi2APaceh3+uBLIgLk8ysy52g2U3gP7Q26Jlg\n"
"q/xKzj3O9hFh/g==\n"
"-----END CERTIFICATE-----\n";


// ----------------------------
// Azure hub + device names
// ----------------------------
String iothubName = "cs147IotHub47";
String deviceName = "147esp32";

String url =
    "https://" + iothubName +
    ".azure-devices.net/devices/" +
    deviceName +
    "/messages/events?api-version=2021-04-12";

// Telemetry interval
#define TELEMETRY_INTERVAL 3000
uint32_t lastTelemetryTime = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    // Init DHT sensor
    dht.begin();

    // Init PMS5003 UART
    // PMS5003 TX → ESP32 RX = GPIO16
    // PMS5003 RX → ESP32 TX = GPIO17
    pmsSerial.begin(9600, SERIAL_8N1, 16, 17);

    if (!aqi.begin_UART(&pmsSerial)) {
        Serial.println("Could not find PMS5003 sensor!");
    } else {
        Serial.println("PMS5003 sensor initialized!");
    }

    // LED setup
    pinMode(RED_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, LOW);

    // WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
}

void loop() {
    if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL) {

        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();

        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read DHT11");
            return;
        }

        // Read PMS5003
        PM25_AQI_Data data;
        bool pmOK = aqi.read(&data);

        if (!pmOK) {
            Serial.println("Failed to read PMS5003");
        }

        if (pmOK) {
            Serial.print("PM1.0: ");
            Serial.print(data.pm10_standard);
            Serial.print("  PM2.5: ");
            Serial.print(data.pm25_standard);
            Serial.print("  PM10: ");
            Serial.println(data.pm100_standard);
        }

            // LED air-quality logic
            float pm1  = data.pm10_standard;
            float pm25 = data.pm25_standard;
            float pm10 = data.pm100_standard;
            float hum  = dht.readHumidity();

            // SENSOR SCALE (determined from your real readings)
            const float SCALE = 80.0;

            // Normalize to real-world units (approx ug/m^3)
            float n1  = pm1  / SCALE;
            float n25 = pm25 / SCALE;
            float n10 = pm10 / SCALE;

            // Weighted indoor air-quality score
            float AQ =
                (0.6 * n25) +   // PM2.5 most important
                (0.2 * n1)  +   // PM1 = fine dust
                (0.15 * n10) +  // PM10 = coarse dust/pollen
                (0.05 * (hum / 50.0)); // humidity effect added gently

            Serial.print("AQ Score: ");
            Serial.println(AQ);

            // Air quality thresholds for LED
            // Indoor-unhealthy approx AQ >= 25-30
            if (AQ > 30) {
                digitalWrite(RED_LED_PIN, HIGH);
                Serial.println("BAD AIR -> LED ON");
            } else {
                digitalWrite(RED_LED_PIN, LOW);
                Serial.println("Air OK > LED OFF");
            }


        // Build JSON
        JsonDocument doc;
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;

        if (pmOK) {
            doc["pm1_0"] = data.pm10_standard;
            doc["pm2_5"] = data.pm25_standard;
            doc["pm10"]  = data.pm100_standard;
        }

        char buffer[256];
        serializeJson(doc, buffer);

        // Azure HTTPS send
        WiFiClientSecure client;
        client.setCACert(root_ca);

        HTTPClient http;
        http.begin(client, url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", SAS_TOKEN);

        int httpCode = http.POST(buffer);

        if (httpCode == 204) {
            Serial.print("Telemetry sent: ");
            Serial.println(buffer);
        } else {
            Serial.print("ERROR sending! HTTP code: ");
            Serial.println(httpCode);
        }

        http.end();
        lastTelemetryTime = millis();
    }
}
