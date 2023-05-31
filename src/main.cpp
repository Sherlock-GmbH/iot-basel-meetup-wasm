#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <AsyncElegantOTA.h>
#include <LITTLEFS.h>
#include "wasm3.h"

const char *ssid = "your-ssid";
const char *password = "your-password";

AsyncWebServer server(80);

void startWebServer()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
             { request->send(200, "text/plain", "OK!"); });
    // Start ElegantOTA
    AsyncElegantOTA.begin(&server);
    server.begin();
}

void startSerial()
{
    Serial.begin(115200);
    while (!Serial)
    {
    }
    // Blank line for nicer output
    Serial.println("");
}

void startWifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
       delay(500);
       Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void checkFilesize() 
{
    File file = LittleFS.open("/firmware.wasm", "r");
    size_t s = file.size();
    Serial.print("Filesize: ");
    Serial.println(s);
    file.close();
}

void setupWasmInterpreter()
{
    // Initialize the wasm interpreter
    Serial.println("Initializing wasm interpreter...");

    #define WASM_STACK_SIZE 1024
    #define WASM_MEMORY_LIMIT 4096

    M3Result result = m3Err_none;
    IM3Environment env = m3_NewEnvironment();
    IM3Runtime runtime = m3_NewRuntime(env, WASM_STACK_SIZE, NULL);
    // runtime->memoryLimit = WASM_MEMORY_LIMIT;

    File file = LittleFS.open("/firmware.wasm", "r");
    size_t s = file.size();

    uint8_t buf[s];
    file.read(buf, s);

    IM3Module module;
    result = m3_ParseModule(env, &module, buf, s);
    if (result)
    {
        Serial.println("Failed to parse module");
        Serial.println(result);
    }

    result = m3_LoadModule(runtime, module);
    if (result)
    {
        Serial.println("Failed to load module");
        Serial.println(result);
    }

    Serial.println("Initialized wasm interpreter");

    IM3Function startF;
    result = m3_FindFunction(&startF, runtime, "wasmTest");
    if (result)
    {
        Serial.println("Failed to find function");
        Serial.println(result);
    }

    Serial.println("Running WebAssembly...");
    result = m3_CallV(startF);

    // Getting here means we've encountered an error.
    // Let's print out some debugging information
    if (result)
    {
        M3ErrorInfo info;
        m3_GetErrorInfo(runtime, &info);
        Serial.print("Error: ");
        Serial.print(result);
        Serial.print(" (");
        Serial.print(info.message);
        Serial.println(")");
        if (info.file && strlen(info.file) && info.line)
        {
            Serial.print("At ");
            Serial.print(info.file);
            Serial.print(":");
            Serial.println(info.line);
        }
    }
}

void setup()
{
    startSerial();
    startWifi();
    startWebServer();
    checkFilesize();
    // setupWasmInterpreter();
}

void loop()
{
    // AsyncElegantOTA.loop();
}