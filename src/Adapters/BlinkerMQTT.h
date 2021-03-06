#ifndef BlinkerMQTT_H
#define BlinkerMQTT_H

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
#endif
#include "Blinker/BlinkerProtocol.h"
#include "modules/WebSockets/WebSocketsServer.h"
#include "modules/mqtt/Adafruit_MQTT.h"
#include "modules/mqtt/Adafruit_MQTT_Client.h"
#include "modules/ArduinoJson/ArduinoJson.h"

static char MQTT_HOST[BLINKER_MQTT_HOST_SIZE];
static uint16_t MQTT_PORT;
// static char MQTT_DEVICEID[BLINKER_MQTT_DEVICEID_SIZE];
static char MQTT_ID[BLINKER_MQTT_ID_SIZE];
static char MQTT_NAME[BLINKER_MQTT_NAME_SIZE];
static char MQTT_KEY[BLINKER_MQTT_KEY_SIZE];
static char MQTT_PRODUCTINFO[BLINKER_MQTT_PINFO_SIZE];
static char UUID[BLINKER_MQTT_UUID_SIZE];
// static char DEVICE_NAME[BLINKER_MQTT_DEVICENAME_SIZE];
static char DEVICE_NAME[BLINKER_MQTT_DEVICEID_SIZE];
static char *BLINKER_PUB_TOPIC;
static char *BLINKER_SUB_TOPIC;

WiFiClientSecure        client_s;
WiFiClient              client;
Adafruit_MQTT_Client    *mqtt;
// Adafruit_MQTT_Publish   *iotPub;
Adafruit_MQTT_Subscribe *iotSub;

#define WS_SERVERPORT                       81
WebSocketsServer webSocket = WebSocketsServer(WS_SERVERPORT);

// static char msgBuf[BLINKER_MAX_READ_SIZE];
static char* msgBuf;//[BLINKER_MAX_READ_SIZE];
static bool isFresh = false;
static bool isConnect = false;
static bool isAvail = false;
static uint8_t ws_num = 0;
static uint8_t dataFrom = BLINKER_MSG_FROM_MQTT;

static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{

    switch(type)
    {
        case WStype_DISCONNECTED:
            BLINKER_LOG_ALL("Disconnected! ", num);

            isConnect = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                
                BLINKER_LOG_ALL("num: ", num, ", Connected from: ", ip, ", url: ", (char *)payload);
                
                // send message to client
                webSocket.sendTXT(num, "{\"state\":\"connected\"}\n");

                ws_num = num;

                isConnect = true;
            }
            break;
        case WStype_TEXT:
            BLINKER_LOG_ALL("num: ", num, ", get Text: ", (char *)payload, ", length: ", length);
            
            if (length < BLINKER_MAX_READ_SIZE) {
                if (!isFresh) msgBuf = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
                // msgBuf = (char*)malloc((length+1)*sizeof(char));
                // memcpy (msgBuf, (char*)payload, length);
                // buflen = length;
                strcpy(msgBuf, (char*)payload);
                isAvail = true;
                isFresh = true;
            }

            dataFrom = BLINKER_MSG_FROM_WS;

            ws_num = num;

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            // BLINKER_LOG("num: ", num, " get binary length: ", length);
            // hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }
}

class BlinkerMQTT {
    public :
        BlinkerMQTT() {}
        // :
        //     _authKey(NULL) {}
        
        bool connect();

        bool connected() { 
            if (!isMQTTinit) {
                return *isHandle;
            }

            return mqtt->connected()||*isHandle; 
        }

        bool mConnected() {
            if (!isMQTTinit) return false;
            else return mqtt->connected();
        }

        void disconnect() {
            mqtt->disconnect();

            if (*isHandle)
                webSocket.disconnect();
        }
        void ping();
        
        bool available() {
            webSocket.loop();

            checkKA();

            if (!mqtt->connected() || (millis() - this->latestTime) > BLINKER_MQTT_PING_TIMEOUT) {
                ping();
            }
            else {
                subscribe();
            }

            if (isAvail) {
                isAvail = false;

                return true;
            }
            else {
                return false;
            }
        }

        bool aligenieAvail() {
            if (isAliAvail) {
                isAliAvail = false;

                return true;
            }
            else {
                return false;
            }
        }

        bool extraAvailable() {
            if (isBavail) {
                isBavail = false;
                
                return true;
            }
            else {
                return false;
            }
        }

        void subscribe();
        char * lastRead() { return isFresh ? msgBuf : NULL; }
        void flush() {
            if (isFresh) {
                free(msgBuf); isFresh = false; isAvail = false;
                isAliAvail = false; isBavail = false;
            }
        }
        bool print(String data);
        bool bPrint(String name, String data);

#if defined(BLINKER_ALIGENIE)
        bool aliPrint(String data);
#endif

        void begin(const char* auth) {
            // _authKey = auth;
            strcpy(_authKey, auth);
            
            BLINKER_LOG_ALL("_authKey: ", auth);

            // if (connectServer()) {
            //     mDNSInit();
            //     isMQTTinit = true;
            // }
            // else {
                uint32_t re_time = millis();
                bool isConnect = true;
                while(1) {
                    re_time = millis();
                    // ::delay(10000);
                    // BLINKER_ERR_LOG("Maybe you have put in the wrong AuthKey!");
                    // BLINKER_ERR_LOG("Or maybe your request is too frequently!");
                    // BLINKER_ERR_LOG("Or maybe your network is disconnected!");
                    if (connectServer()) {
                        mDNSInit();
                        isMQTTinit = true;
                        return;
                    }
                    // delay(10000);
                    while ((millis() - re_time) < 10000) {
                        if (WiFi.status() != WL_CONNECTED && isConnect) {
                            isConnect = false;
                            WiFi.begin();
                            WiFi.reconnect();
                        }
                        else if (WiFi.status() == WL_CONNECTED && !isConnect) {
                            isConnect = true;
                        }
                        ::delay(10);
                        // WiFi.status();
                    }
                }
            // }
        }

        bool autoPrint(uint32_t id) {
            String payload = "{\"data\":{\"set\":{" + \
                STRING_format("\"trigged\":true,\"autoData\":{") + \
                "\"autoId\":" + STRING_format(id)  + "}}}" + \
                ",\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"" + \
                ",\"toDevice\":\"" + "autoManager" + "\"}";
                // "\",\"deviceType\":\"" + "type" + "\"}";

            BLINKER_LOG_ALL("autoPrint...");

            if (mqtt->connected()) {
                if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || linkTime == 0) {
                    // linkTime = millis();

                    // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

                    // if (! iotPub.publish(payload.c_str())) {

                    if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str())) {
                        BLINKER_LOG_ALL(payload);
                        BLINKER_LOG_ALL("...Failed");
                        
                        return false;
                    }
                    else {
                        BLINKER_LOG_ALL(payload);
                        BLINKER_LOG_ALL("...OK!");
                        
                        linkTime = millis();
                        return true;
                    }
                }
                else {
                    BLINKER_ERR_LOG_ALL("MQTT NOT ALIVE OR MSG LIMIT ", linkTime);
                    
                    return false;
                }
            }
            else {
                BLINKER_ERR_LOG("MQTT Disconnected");
                return false;
            }
        }

        bool autoPrint(char *name, char *type, char *data) {
            String payload = "{\"data\":{" + STRING_format(data) + "}," + \ 
                + "\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"," + \
                + "\"toDevice\":\"" + name + "\"," + \
                + "\"deviceType\":\"" + type + "\"}";
                
            BLINKER_LOG_ALL("autoPrint...");
            
            if (mqtt->connected()) {
                if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || linkTime == 0) {
                    linkTime = millis();
                    
                    BLINKER_LOG_ALL(payload, ("...OK!"));
                    
                    return true;
                }
                else {
                    BLINKER_ERR_LOG_ALL("MQTT NOT ALIVE OR MSG LIMIT ", linkTime);
                    
                    return false;
                }
            }
            else {
                BLINKER_ERR_LOG("MQTT Disconnected");
                return false;
            }
        }

        bool autoPrint(char *name1, char *type1, char *data1
            , char *name2, char *type2, char *data2)
        {
            String payload = "{\"data\":{" + STRING_format(data1) + "}," + \ 
                + "\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"," + \
                + "\"toDevice\":\"" + name1 + "\"," + \
                + "\"deviceType\":\"" + type1 + "\"}";
                
            BLINKER_LOG_ALL("autoPrint...");
            
            if (mqtt->connected()) {
                if ((millis() - linkTime) > BLINKER_LINK_MSG_LIMIT || linkTime == 0) {
                    linkTime = millis();

                    BLINKER_LOG_ALL(payload, ("...OK!"));

                    payload = "{\"data\":{" + STRING_format(data2) + "}," + \ 
                        + "\"fromDevice\":\"" + STRING_format(MQTT_ID) + "\"," + \
                        + "\"toDevice\":\"" + name2 + "\"," + \
                        + "\"deviceType\":\"" + type2 + "\"}";
                        
                    BLINKER_LOG_ALL(payload, ("...OK!"));
                    
                    return true;
                }
                else {
                    BLINKER_ERR_LOG_ALL("MQTT NOT ALIVE OR MSG LIMIT ", linkTime);
                    
                    return false;
                }
            }
            else {
                BLINKER_ERR_LOG("MQTT Disconnected");
                return false;
            }
        }

        String deviceName() { return DEVICE_NAME;/*MQTT_ID;*/ }

        bool init() { return isMQTTinit; }

        bool reRegister() { return connectServer(); }

    private :    

        bool isMQTTinit = false;

        bool connectServer();

        void mDNSInit()
        {
#if defined(ESP8266)
            if (!MDNS.begin(DEVICE_NAME, WiFi.localIP())) {
#elif defined(ESP32)
            if (!MDNS.begin(DEVICE_NAME)) {
#endif
                while(1) {
                    ::delay(100);
                }
            }

            BLINKER_LOG(("mDNS responder started"));
            
            MDNS.addService(BLINKER_MDNS_SERVICE_BLINKER, "tcp", WS_SERVERPORT);
            MDNS.addServiceTxt(BLINKER_MDNS_SERVICE_BLINKER, "tcp", "deviceName", String(DEVICE_NAME));

            webSocket.begin();
            webSocket.onEvent(webSocketEvent);
            BLINKER_LOG(("webSocket server started"));
            BLINKER_LOG("ws://", DEVICE_NAME, ".local:", WS_SERVERPORT);
        }

        void checkKA() {
            if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
                isAlive = false;
        }

        bool checkAliKA() {
            if (millis() - aliKaTime >= 10000)
                return false;
            else
                return true;
        }

        bool checkCanPrint() {
            if ((millis() - printTime >= BLINKER_MQTT_MSG_LIMIT && isAlive) || printTime == 0) {
                return true;
            }
            else {
                BLINKER_ERR_LOG_ALL("MQTT NOT ALIVE OR MSG LIMIT");
                
                checkKA();

                return false;
            }
        }

        bool checkCanBprint() {
            if ((millis() - bPrintTime >= BLINKER_BRIDGE_MSG_LIMIT) || bPrintTime == 0) {
                return true;
            }
            else {
                BLINKER_ERR_LOG_ALL("MQTT NOT ALIVE OR MSG LIMIT");
                
                return false;
            }
        }

        bool checkPrintSpan() {
            if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
                if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
                    BLINKER_ERR_LOG_ALL("WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT");
                    
                    return false;
                }
                else {
                    respTimes++;
                    return true;
                }
            }
            else {
                respTimes = 0;
                return true;
            }
        }

        bool checkAliPrintSpan() {
            if (millis() - respAliTime < BLINKER_PRINT_MSG_LIMIT/2) {
                if (respAliTimes > BLINKER_PRINT_MSG_LIMIT/2) {
                    BLINKER_ERR_LOG_ALL("ALIGENIE NOT ALIVE OR MSG LIMIT");
                    
                    return false;
                }
                else {
                    respAliTimes++;
                    return true;
                }
            }
            else {
                respAliTimes = 0;
                return true;
            }
        }

    protected :
        // const char* _authKey;
        char        _authKey[BLINKER_AUTHKEY_SIZE];
        bool*       isHandle = &isConnect;
        bool        isAlive = false;
        bool        isBavail = false;
        uint32_t    latestTime;
        uint32_t    printTime = 0;
        uint32_t    bPrintTime = 0;
        uint32_t    kaTime = 0;
        uint32_t    linkTime = 0;
        uint8_t     respTimes = 0;
        uint32_t    respTime = 0;
        uint8_t     respAliTimes = 0;
        uint32_t    respAliTime = 0;

        uint32_t    aliKaTime = 0;
        bool        isAliAlive = false;
        bool        isAliAvail = false;
        String      mqtt_broker;
};

bool BlinkerMQTT::connectServer() {
    const int httpsPort = 443;
#if defined(ESP8266)
    const char* host = "iotdev.clz.me";
    const char* fingerprint = "84 5f a4 8a 70 5e 79 7e f5 b3 b4 20 45 c8 35 55 72 f6 85 5a";

    // WiFiClientSecure client_s;
    
    BLINKER_LOG_ALL(("connecting to "), host);
    
    uint8_t connet_times = 0;
    client_s.stop();
    ::delay(100);

    while (1) {
        bool cl_connected = false;
        if (!client_s.connect(host, httpsPort)) {
    // #ifdef BLINKER_DEBUG_ALL
            BLINKER_ERR_LOG(("server connection failed"));
    // #endif
            // return BLINKER_CMD_FALSE;

            connet_times++;
        }
        else {
            BLINKER_LOG_ALL(("connection succeed"));
            // return true;
            cl_connected = true;

            break;
        }

        if (connet_times >= 4 && !cl_connected)  return BLINKER_CMD_FALSE;
    }

#ifndef BLINKER_LAN_DEBUG
    if (client_s.verify(fingerprint, host)) {
        // _status = DH_VERIFIED;
        BLINKER_LOG_ALL(("Fingerprint verified"));
        // return true;
    }
    else {
        // _status = DH_VERIFY_FAILED;
        // _status = DH_VERIFIED;
        BLINKER_LOG_ALL(("Fingerprint verification failed!"));
        // return false;
    }
#endif

    // String url;
    String client_msg;

    String url_iot = "/api/v1/user/device/diy/auth?authKey=" + String(_authKey);

#if defined(BLINKER_ALIGENIE_LIGHT)
    url_iot += "&aliType=light";
#elif defined(BLINKER_ALIGENIE_OUTLET)
    url_iot += "&aliType=outlet";
#elif defined(BLINKER_ALIGENIE_SWITCH)
#elif defined(BLINKER_ALIGENIE_SENSOR)
    url_iot += "&aliType=sensor";
#endif

    BLINKER_LOG_ALL("HTTPS begin: ", host, url_iot);
    
    client_msg = STRING_format("GET " + url_iot + " HTTP/1.1\r\n" +
        "Host: " + host + ":" + STRING_format(httpsPort) + "\r\n" +
        "Connection: close\r\n\r\n");

    client_s.print(client_msg);
    
    BLINKER_LOG_ALL(("client_msg: "), client_msg);

    unsigned long timeout = millis();
    while (client_s.available() == 0) {
        if (millis() - timeout > 5000) {
            BLINKER_LOG_ALL((">>> Client Timeout !"));
            client_s.stop();
            return BLINKER_CMD_FALSE;
        }
    }

    String _dataGet;
    String lastGet;
    String lengthOfJson;
    while (client_s.available()) {
        // String line = client_s.readStringUntil('\r');
        _dataGet = client_s.readStringUntil('\n');

        if (_dataGet.startsWith("Content-Length: ")){
            int addr_start = _dataGet.indexOf(' ');
            int addr_end = _dataGet.indexOf('\0', addr_start + 1);
            lengthOfJson = _dataGet.substring(addr_start + 1, addr_end);
        }

        if (_dataGet == "\r") {
            BLINKER_LOG_ALL(("headers received"));
            
            break;
        }
    }

    for(int i=0;i<lengthOfJson.toInt();i++){
        lastGet += (char)client_s.read();
    }

    _dataGet = lastGet;
    
    BLINKER_LOG_ALL(("_dataGet: "), _dataGet);

    String payload = _dataGet;

#elif defined(ESP32)
    const char* host = "https://iotdev.clz.me";
    // const char* ca = \ 
    //     "-----BEGIN CERTIFICATE-----\n" \
    //     "MIIEgDCCA2igAwIBAgIQDKTfhr9lmWbWUT0hjX36oDANBgkqhkiG9w0BAQsFADBy\n" \
    //     "MQswCQYDVQQGEwJDTjElMCMGA1UEChMcVHJ1c3RBc2lhIFRlY2hub2xvZ2llcywg\n" \
    //     "SW5jLjEdMBsGA1UECxMURG9tYWluIFZhbGlkYXRlZCBTU0wxHTAbBgNVBAMTFFRy\n" \
    //     "dXN0QXNpYSBUTFMgUlNBIENBMB4XDTE4MDEwNDAwMDAwMFoXDTE5MDEwNDEyMDAw\n" \
    //     "MFowGDEWMBQGA1UEAxMNaW90ZGV2LmNsei5tZTCCASIwDQYJKoZIhvcNAQEBBQAD\n" \
    //     "ggEPADCCAQoCggEBALbOFn7cJ2I/FKMJqIaEr38n4kCuJCCeNf1bWdWvOizmU2A8\n" \
    //     "QeTAr5e6Q3GKeJRdPnc8xXhqkTm4LOhgdZB8KzuVZARtu23D4vj4sVzxgC/zwJlZ\n" \
    //     "MRMxN+cqI37kXE8gGKW46l2H9vcukylJX+cx/tjWDfS2YuyXdFuS/RjhCxLgXzbS\n" \
    //     "cve1W0oBZnBPRSMV0kgxTWj7hEGZNWKIzK95BSCiMN59b+XEu3NWGRb/VzSAiJEy\n" \
    //     "Hy9DcDPBC9TEg+p5itHtdMhy2gq1OwsPgl9HUT0xmDATSNEV2RB3vwviNfu9/Eif\n" \
    //     "ObhsV078zf30TqdiESqISEB68gJ0Otru67ePoTkCAwEAAaOCAWowggFmMB8GA1Ud\n" \
    //     "IwQYMBaAFH/TmfOgRw4xAFZWIo63zJ7dygGKMB0GA1UdDgQWBBR/KLqnke61779P\n" \
    //     "xc9htonQwLOxPDAYBgNVHREEETAPgg1pb3RkZXYuY2x6Lm1lMA4GA1UdDwEB/wQE\n" \
    //     "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTAYDVR0gBEUwQzA3\n" \
    //     "BglghkgBhv1sAQIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQu\n" \
    //     "Y29tL0NQUzAIBgZngQwBAgEwgYEGCCsGAQUFBwEBBHUwczAlBggrBgEFBQcwAYYZ\n" \
    //     "aHR0cDovL29jc3AyLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\n" \
    //     "Y2VydHMuZGlnaXRhbGNlcnR2YWxpZGF0aW9uLmNvbS9UcnVzdEFzaWFUTFNSU0FD\n" \
    //     "QS5jcnQwCQYDVR0TBAIwADANBgkqhkiG9w0BAQsFAAOCAQEAhtM4eyrWB14ajJpQ\n" \
    //     "ibZ5FbzVuvv2Le0FOSoss7UFCDJUYiz2LiV8yOhL4KTY+oVVkqHaYtcFS1CYZNzj\n" \
    //     "6xWcqYZJ+pgsto3WBEgNEEe0uLSiTW6M10hm0LFW9Det3k8fqwSlljqMha3gkpZ6\n" \
    //     "8WB0f2clXOuC+f1SxAOymnGUsSqbU0eFSgevcOIBKR7Hr3YXBXH3jjED76Q52OMS\n" \
    //     "ucfOM9/HB3jN8o/ioQbkI7xyd/DUQtzK6hSArEoYRl3p5H2P4fr9XqmpoZV3i3gQ\n" \
    //     "oOdVycVtpLunyUoVAB2DcOElfDxxXCvDH3XsgoIU216VY03MCaUZf7kZ2GiNL+UX\n" \
    //     "9UBd0Q==\n" \
    //     "-----END CERTIFICATE-----\n";
// #endif

    HTTPClient http;

    String url_iot = String(host) + "/api/v1/user/device/diy/auth?authKey=" + String(_authKey);

#if defined(BLINKER_ALIGENIE_LIGHT)
    url_iot += "&aliType=light";
#elif defined(BLINKER_ALIGENIE_OUTLET)
    url_iot += "&aliType=outlet";
#elif defined(BLINKER_ALIGENIE_SWITCH)
#elif defined(BLINKER_ALIGENIE_SENSOR)
    url_iot += "&aliType=sensor";
#endif

    BLINKER_LOG_ALL("HTTPS begin: ", url_iot);

// #if defined(ESP8266)
//     http.begin(url_iot, fingerprint); //HTTP
// #elif defined(ESP32)
    // http.begin(url_iot, ca); TODO
    http.begin(url_iot);
// #endif
    int httpCode = http.GET();

    String payload;

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled

        BLINKER_LOG_ALL("[HTTP] GET... code: ", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            // BLINKER_LOG(payload);
        }
    }
    else {
        BLINKER_LOG_ALL("[HTTP] GET... failed, error: ", http.errorToString(httpCode).c_str());
        payload = http.getString();
        BLINKER_LOG_ALL(payload);
    }

    http.end();
#endif

    BLINKER_LOG_ALL("reply was:");
    BLINKER_LOG_ALL("==============================");
    BLINKER_LOG_ALL(payload);
    BLINKER_LOG_ALL("==============================");

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(payload);

    if (STRING_contains_string(payload, BLINKER_CMD_NOTFOUND) || !root.success() ||
        !STRING_contains_string(payload, BLINKER_CMD_IOTID)) {
        // while(1) {
            BLINKER_ERR_LOG("Maybe you have put in the wrong AuthKey!");
            BLINKER_ERR_LOG("Or maybe your request is too frequently!");
            BLINKER_ERR_LOG("Or maybe your network is disconnected!");
            // ::delay(60000);

            return false;
        // }
    }

    // String _userID = STRING_find_string(payload, "deviceName", "\"", 4);
    // String _userName = STRING_find_string(payload, "iotId", "\"", 4);
    // String _key = STRING_find_string(payload, "iotToken", "\"", 4);
    // String _productInfo = STRING_find_string(payload, "productKey", "\"", 4);
    // String _broker = STRING_find_string(payload, "broker", "\"", 4);
    // String _uuid = STRING_find_string(payload, "uuid", "\"", 4);
    String _userID = root[BLINKER_CMD_DETAIL][BLINKER_CMD_DEVICENAME];
    String _userName = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTID];
    String _key = root[BLINKER_CMD_DETAIL][BLINKER_CMD_IOTTOKEN];
    String _productInfo = root[BLINKER_CMD_DETAIL][BLINKER_CMD_PRODUCTKEY];
    String _broker = root[BLINKER_CMD_DETAIL][BLINKER_CMD_BROKER];
    String _uuid = root[BLINKER_CMD_DETAIL][BLINKER_CMD_UUID];

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(DEVICE_NAME, _userID.c_str());
        strcpy(MQTT_ID, _userID.c_str());
        strcpy(MQTT_NAME, _userName.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_ALIYUN_HOST);
        MQTT_PORT = BLINKER_MQTT_ALIYUN_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        // String id2name = _userID.subString(10, _userID.length());
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(DEVICE_NAME, _userID.c_str());
        String IDtest = _productInfo + _userID;
        strcpy(MQTT_ID, IDtest.c_str());
        String NAMEtest = IDtest + ";" + _userName;
        strcpy(MQTT_NAME, NAMEtest.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_QCLOUD_HOST);
        MQTT_PORT = BLINKER_MQTT_QCLOUD_PORT;
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        // memcpy(DEVICE_NAME, _userID.c_str(), 12);
        strcpy(DEVICE_NAME, _userID.c_str());
        strcpy(MQTT_ID, _userName.c_str());
        strcpy(MQTT_NAME, _productInfo.c_str());
        strcpy(MQTT_KEY, _key.c_str());
        strcpy(MQTT_PRODUCTINFO, _productInfo.c_str());
        strcpy(MQTT_HOST, BLINKER_MQTT_ONENET_HOST);
        MQTT_PORT = BLINKER_MQTT_ONENET_PORT;
    }
    strcpy(UUID, _uuid.c_str());
    
    BLINKER_LOG_ALL("====================");
    BLINKER_LOG_ALL("DEVICE_NAME: ", DEVICE_NAME);
    BLINKER_LOG_ALL("MQTT_PRODUCTINFO: ", MQTT_PRODUCTINFO);
    BLINKER_LOG_ALL("MQTT_ID: ", MQTT_ID);
    BLINKER_LOG_ALL("MQTT_NAME: ", MQTT_NAME);
    BLINKER_LOG_ALL("MQTT_KEY: ", MQTT_KEY);
    BLINKER_LOG_ALL("MQTT_BROKER: ", _broker);
    BLINKER_LOG_ALL("HOST: ", MQTT_HOST);
    BLINKER_LOG_ALL("PORT: ", MQTT_PORT);
    BLINKER_LOG_ALL("UUID: ", UUID);
    BLINKER_LOG_ALL("====================");

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        uint8_t str_len;
        String PUB_TOPIC_STR = "/" + String(MQTT_PRODUCTINFO) + "/" + String(MQTT_ID) + "/s";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
        
        BLINKER_LOG_ALL("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
        
        String SUB_TOPIC_STR = "/" + String(MQTT_PRODUCTINFO) + "/" + String(MQTT_ID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
        
        BLINKER_LOG_ALL("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        uint8_t str_len;
        String PUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/s";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
        
        BLINKER_LOG_ALL("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
        
        String SUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
        
        BLINKER_LOG_ALL("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        uint8_t str_len;
        String PUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/onenet_rule/r";
        str_len = PUB_TOPIC_STR.length() + 1;
        BLINKER_PUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_PUB_TOPIC, PUB_TOPIC_STR.c_str(), str_len);
        
        BLINKER_LOG_ALL("BLINKER_PUB_TOPIC: ", BLINKER_PUB_TOPIC);
        
        String SUB_TOPIC_STR = String(MQTT_PRODUCTINFO) + "/" + String(_userID) + "/r";
        str_len = SUB_TOPIC_STR.length() + 1;
        BLINKER_SUB_TOPIC = (char*)malloc(str_len*sizeof(char));
        memcpy(BLINKER_SUB_TOPIC, SUB_TOPIC_STR.c_str(), str_len);
        
        BLINKER_LOG_ALL("BLINKER_SUB_TOPIC: ", BLINKER_SUB_TOPIC);
    }

    if (_broker == BLINKER_MQTT_BORKER_ALIYUN) {
        mqtt = new Adafruit_MQTT_Client(&client_s, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
    }
    else if (_broker == BLINKER_MQTT_BORKER_QCLOUD) {
        mqtt = new Adafruit_MQTT_Client(&client_s, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
    }
    else if (_broker == BLINKER_MQTT_BORKER_ONENET) {
        mqtt = new Adafruit_MQTT_Client(&client, MQTT_HOST, MQTT_PORT, MQTT_ID, MQTT_NAME, MQTT_KEY);
    }

    // iotPub = new Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);
    iotSub = new Adafruit_MQTT_Subscribe(mqtt, BLINKER_SUB_TOPIC);

    mqtt_broker = _broker;

    // mDNSInit(MQTT_ID);
    this->latestTime = millis();
    mqtt->subscribe(iotSub);
    connect();

    return true;
}

bool BlinkerMQTT::connect() {
    int8_t ret;

    webSocket.loop();

    if (mqtt->connected()) {
        return true;
    }

    disconnect();

    if ((millis() - latestTime) < 5000) {
        return false;
    }

// #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG("Connecting to MQTT... ");
// #endif

    if ((ret = mqtt->connect()) != 0) {
        BLINKER_LOG(mqtt->connectErrorString(ret));
        BLINKER_LOG("Retrying MQTT connection in 5 seconds...");

        this->latestTime = millis();
        return false;
    }
// #ifdef BLINKER_DEBUG_ALL
    BLINKER_LOG("MQTT Connected!");
// #endif

    this->latestTime = millis();

    return true;
}

void BlinkerMQTT::ping() {
    BLINKER_LOG_ALL("MQTT Ping!");

    if (!mqtt->ping()) {
        disconnect();
        delay(100);

        connect();
    }
    else {
        this->latestTime = millis();
    }
}

void BlinkerMQTT::subscribe() {
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt->readSubscription(10))) {
        if (subscription == iotSub) {
            BLINKER_LOG_ALL(("Got: "), (char *)iotSub->lastread);

            // String dataGet = String((char *)iotSub->lastread);

            // DynamicJsonDocument doc;
            // deserializeJson(doc, String((char *)iotSub->lastread));
            // JsonObject& root = doc.as<JsonObject>();
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(String((char *)iotSub->lastread));
	        // JsonObject& root = jsonBuffer.parseObject((char *)iotSub->lastread);

            // if (!root.success()) {
            //     BLINKER_LOG("json test error");
            //     return;
            // }

            String _uuid = root["fromDevice"];
            String dataGet = root["data"];

            // String _uuid = STRING_find_string(dataGet, "fromDevice", "\"", 3);

            // dataGet = STRING_find_string(dataGet, BLINKER_CMD_DATA, ",\"fromDevice", 2);

            // if (dataGet.indexOf("\"") != -1 && dataGet.indexOf("\"") == 0) {
            //     dataGet = STRING_find_string(dataGet, "\"", "\"", 0);
            // }

            // BLINKER_LOG("data: ", dataGet);
            
            BLINKER_LOG_ALL("data: ", dataGet);
            BLINKER_LOG_ALL("fromDevice: ", _uuid);
            
            if (strcmp(_uuid.c_str(), UUID) == 0) {
                BLINKER_LOG_ALL("Authority uuid");
                
                kaTime = millis();
                isAvail = true;
                isAlive = true;
            }
            else if (_uuid == BLINKER_CMD_ALIGENIE) {
                BLINKER_LOG_ALL("form AliGenie");
                
                aliKaTime = millis();
                isAliAlive = true;
                isAliAvail = true;
            }
            else {
                dataGet = String((char *)iotSub->lastread);
                
                BLINKER_ERR_LOG_ALL("No authority uuid, check is from bridge/share device, data: ", dataGet);
                
                // return;

                isBavail = true;
            }

            // memset(msgBuf, 0, BLINKER_MAX_READ_SIZE);
            // memcpy(msgBuf, dataGet.c_str(), dataGet.length());

            if (!isFresh) msgBuf = (char*)malloc(BLINKER_MAX_READ_SIZE*sizeof(char));
            strcpy(msgBuf, dataGet.c_str());
            isFresh = true;
            
            this->latestTime = millis();

            dataFrom = BLINKER_MSG_FROM_MQTT;
        }
    }
}

bool BlinkerMQTT::print(String data) {
    if (*isHandle && dataFrom == BLINKER_MSG_FROM_WS) {
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE) ||
                    (STRING_contains_string(data, BLINKER_CMD_TIMING) && 
                     STRING_contains_string(data, BLINKER_CMD_ENABLE)) ||
                    (STRING_contains_string(data, BLINKER_CMD_LOOP) && 
                     STRING_contains_string(data, BLINKER_CMD_TIMES)) ||
                    (STRING_contains_string(data, BLINKER_CMD_COUNTDOWN) &&
                     STRING_contains_string(data, BLINKER_CMD_TOTALTIME));

        if (!state) {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) 
                && STRING_contains_string(data, BLINKER_CMD_ONLINE))
                || (STRING_contains_string(data, BLINKER_CMD_BUILTIN_SWITCH)));

            if (!checkPrintSpan()) {
                respTime = millis();
                return false;
            }
            respTime = millis();
        }

        if (!state) {
            if (!checkPrintSpan()) {
                respTime = millis();
                return false;
            }
        }

        respTime = millis();
        
        BLINKER_LOG_ALL("WS response: ");
        BLINKER_LOG_ALL(data);
        BLINKER_LOG_ALL("Succese...");
        
        webSocket.sendTXT(ws_num, data + BLINKER_CMD_NEWLINE);

        return true;
// #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG("WS response: ", data, "Succese...");
// #endif
    }
    else {
        String payload;
        if (STRING_contains_string(data, BLINKER_CMD_NEWLINE)) {
            payload = "{\"data\":" + data.substring(0, data.length() - 1) + \
                    ",\"fromDevice\":\"" + MQTT_ID + \
                    "\",\"toDevice\":\"" + UUID + \
                    "\",\"deviceType\":\"OwnApp\"}";
        }
        else {
            payload = "{\"data\":" + data + \
                    ",\"fromDevice\":\"" + MQTT_ID + \
                    "\",\"toDevice\":\"" + UUID + \
                    "\",\"deviceType\":\"OwnApp\"}";
        }
        
        BLINKER_LOG_ALL("MQTT Publish...");
        
        bool _alive = isAlive;
        bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE) ||
                    (STRING_contains_string(data, BLINKER_CMD_TIMING) && 
                     STRING_contains_string(data, BLINKER_CMD_ENABLE)) ||
                    (STRING_contains_string(data, BLINKER_CMD_LOOP) && 
                     STRING_contains_string(data, BLINKER_CMD_TIMES)) ||
                    (STRING_contains_string(data, BLINKER_CMD_COUNTDOWN) &&
                     STRING_contains_string(data, BLINKER_CMD_TOTALTIME));

        if (!state) {
            state = ((STRING_contains_string(data, BLINKER_CMD_STATE) 
                && STRING_contains_string(data, BLINKER_CMD_ONLINE))
                || (STRING_contains_string(data, BLINKER_CMD_BUILTIN_SWITCH)));

            if (!checkPrintSpan()) {
                return false;
            }
            respTime = millis();
        }

// #ifdef BLINKER_DEBUG_ALL
//         BLINKER_LOG("state: ", state);

//         BLINKER_LOG("state: ", STRING_contains_string(data, BLINKER_CMD_TIMING));

//         BLINKER_LOG("state: ", data.indexOf(BLINKER_CMD_TIMING));

//         BLINKER_LOG("data: ", data);
// #endif

        if (mqtt->connected()) {
            if (!state) {
                if (!checkCanPrint()) {
                    if (!_alive) {
                        isAlive = false;
                    }
                    return false;
                }
            }

            // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

            // if (!iotPub.publish(payload.c_str())) {

            if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str())) {
                BLINKER_LOG_ALL(payload);
                BLINKER_LOG_ALL("...Failed");
                
                if (!_alive) {
                    isAlive = false;
                }
                return false;
            }
//             else if (mqtt_broker == BLINKER_MQTT_BORKER_ONENET) {
//                 char buf[BLINKER_MAX_SEND_BUFFER_SIZE];
//                 buf[0] = 0x01;
//                 buf[1] = 0x00;
//                 buf[2] = (uint8_t)payload.length();

//                 memcpy(buf+3, payload.c_str(), payload.length());

//                 if (!iotPub.publish((uint8_t *)buf, payload.length()+3)) {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG(payload);
//                     BLINKER_LOG("...Failed");
// #endif
//                     if (!_alive) {
//                         isAlive = false;
//                     }
//                     return false;
//                 } else {
// #ifdef BLINKER_DEBUG_ALL
//                     BLINKER_LOG(payload);
//                     BLINKER_LOG("...OK!");
// #endif
//                     if (!state) printTime = millis();

//                     if (!_alive) {
//                         isAlive = false;
//                     }
//                     return true;
//                 }
//             }
            else {
                BLINKER_LOG_ALL(payload);
                BLINKER_LOG_ALL("...OK!");
                
                if (!state) printTime = millis();

                if (!_alive) {
                    isAlive = false;
                }
                return true;
            }            
        }
        else {
            BLINKER_ERR_LOG("MQTT Disconnected");
            isAlive = false;
            return false;
        }
    }
}

bool BlinkerMQTT::bPrint(String name, String data) {
    String payload;
    if (STRING_contains_string(data, BLINKER_CMD_NEWLINE)) {
        payload = "{\"data\":" + data.substring(0, data.length() - 1) + \
                ",\"fromDevice\":\"" + MQTT_ID + \
                "\",\"toDevice\":\"" + name + \
                "\",\"deviceType\":\"DiyBridge\"}";
    }
    else {
        payload = "{\"data\":" + data + \
                ",\"fromDevice\":\"" + MQTT_ID + \
                "\",\"toDevice\":\"" + name + \
                "\",\"deviceType\":\"DiyBridge\"}";
    }

    BLINKER_LOG_ALL("MQTT Publish...");

    // bool _alive = isAlive;
    // bool state = STRING_contains_string(data, BLINKER_CMD_NOTICE);

    // if (!state) {
    //     state = (STRING_contains_string(data, BLINKER_CMD_STATE) 
    //         && STRING_contains_string(data, BLINKER_CMD_ONLINE));
    // }

    if (mqtt->connected()) {
        // if (!state) {
        if (!checkCanBprint()) {
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        // }

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

        // if (! iotPub.publish(payload.c_str())) {

        String bPubTopic = "";

        if (mqtt_broker == BLINKER_MQTT_BORKER_ONENET) {
            bPubTopic = STRING_format(MQTT_PRODUCTINFO) + "/" + name + "/r";
        }
        else {
            bPubTopic = STRING_format(BLINKER_PUB_TOPIC);
        }

        if (! mqtt->publish(bPubTopic.c_str(), payload.c_str())) {
            BLINKER_LOG_ALL(payload);
            BLINKER_LOG_ALL("...Failed");
            
            // if (!_alive) {
            //     isAlive = false;
            // }
            return false;
        }
        else {
            BLINKER_LOG_ALL(payload);
            BLINKER_LOG_ALL("...OK!");
            
            bPrintTime = millis();

            // if (!_alive) {
            //     isAlive = false;
            // }
            return true;
        }            
    }
    else {
        BLINKER_ERR_LOG("MQTT Disconnected");
        // isAlive = false;
        return false;
    }
    // }
}

#if defined(BLINKER_ALIGENIE)
bool BlinkerMQTT::aliPrint(String data)
{
    String payload;

    payload = "{\"data\":" + data + \
            ",\"fromDevice\":\"" + MQTT_ID + \
            "\",\"toDevice\":\"AliGenie_r\",\"deviceType\":\"vAssistant\"}";
            
    BLINKER_LOG_ALL("MQTT AliGenie Publish...");

    if (mqtt->connected()) {
        if (!checkAliKA()) {
            return false;
        }

        if (!checkAliPrintSpan()) {
            respAliTime = millis();
            return false;
        }
        respAliTime = millis();

        // Adafruit_MQTT_Publish iotPub = Adafruit_MQTT_Publish(mqtt, BLINKER_PUB_TOPIC);

        // if (! iotPub.publish(payload.c_str())) {

        if (! mqtt->publish(BLINKER_PUB_TOPIC, payload.c_str())) {
            BLINKER_LOG_ALL(payload);
            BLINKER_LOG_ALL("...Failed");
            
            isAliAlive = false;
            return false;
        }
        else {
            BLINKER_LOG_ALL(payload);
            BLINKER_LOG_ALL("...OK!");
            
            isAliAlive = false;
            return true;
        }      
    }
    else {
        BLINKER_ERR_LOG("MQTT Disconnected");
        return false;
    }
}
#endif

#endif