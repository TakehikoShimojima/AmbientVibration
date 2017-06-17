/*
 * ADXL345で最初に10m秒間隔で10回ｘ、ｙ、ｚ軸の値の平均を取り、キャリブレーション値とする。
 * ADXL345で10ミリ秒毎に1000回、10秒、x, y, z軸の加速度を測定し、Ambientに送信
 */
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <SPI.h>
#include "ADXL345_SPI.h"
#include "Ambient.h"

extern "C" {
#include "user_interface.h"
}

#define _DEBUG 1

#if _DEBUG
#define DBG(...) { Serial.print(__VA_ARGS__); }
#define DBGLED(...) { digitalWrite(__VA_ARGS__); }
#else
#define DBG(...)
#define DBGLED(...)
#endif /* _DBG */

#define ADXL_CS 5

ADXL345 adxl345;

const char* ssid = "...ssid...";
const char* password = "...password...";
WiFiClient client;

unsigned int channelId = 100;
const char* writeKey = "...writeKey...";
Ambient ambient;

#define SAMPLING 10     // サンプリング間隔(ミリ秒)
#define NSAMPLES 1000     // 10ms x 1000 = 10秒
#define BUFSIZE 16000

Ticker t2;
struct xyz {
    int16_t x;
    int16_t y;
    int16_t z;
} xyzbuf[NSAMPLES];
struct xyz calib;

int sampleIndex;
int nsamples;
volatile int done;

char buffer[BUFSIZE];

void sampling() {
    int16_t x, y, z;

    adxl345.readAccel(&x, &y, &z);

    if (!done) {
        xyzbuf[sampleIndex].x = x;
        xyzbuf[sampleIndex].y = y;
        xyzbuf[sampleIndex].z = z;

        if (++sampleIndex >= nsamples) {
            done = true;
        }
    }
}

void calibration() {
    done = false;
    sampleIndex = 0;
    nsamples = 100;

    t2.attach_ms(10, sampling);  // 10m秒(100Hz)でサンプリングする
    while (!done) {
        delay(0);
    }
    t2.detach();

    calib.x = calib.y = calib.z = 0;
    for (int i = 0; i < nsamples; i++) {
        calib.x += xyzbuf[i].x;
        calib.y += xyzbuf[i].y;
        calib.z += xyzbuf[i].z;
    }
    calib.x /= nsamples;
    calib.y /= nsamples;
    calib.z /= nsamples;
}

#define min(a,b) (((a) < (b)) ? (a) : (b))

void setup()
{
#ifdef _DEBUG
    Serial.begin(115200);
    delay(20);
#endif
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        DBG(".");
        delay(100);
    }
    DBG("WiFi connected\r\nIP address: ");
    DBG(WiFi.localIP());
    DBG("\r\n");

    ambient.begin(channelId, writeKey, &client);
    DBG("channelId: ");DBG(channelId);DBG("\r\n");

    DBG("\r\nStart\r\n");

    SPI.begin();
    adxl345.begin(ADXL_CS);

    char devid;
    devid = adxl345.devid();
    Serial.println(devid, HEX);

    calibration();
    Serial.print("calibration x: ");
    Serial.print(calib.x); Serial.print(",\ty: ");
    Serial.print(calib.y); Serial.print(",\tz: ");
    Serial.println(calib.z); Serial.println();

    done = false;
    sampleIndex = 0;
    nsamples = NSAMPLES;

    t2.attach_ms(SAMPLING, sampling);
    while (!done) {
        delay(0);
    }
    t2.detach();

    int sent = 0;
    int datapersend = (BUFSIZE - 44) / 60;

    char strx[10], stry[10], strz[10];
    while (sent < NSAMPLES) {
        sprintf(buffer, "{\"writeKey\":\"%s\",\"data\":[", writeKey);
        datapersend = min(datapersend, NSAMPLES - sent);
        DBG("data/send: ");DBG(datapersend);DBG(", sent: ");DBG(sent);DBG("\r\n");

        for (int i = 0; i < datapersend; i++) {
            double xg = (xyzbuf[(sent + i)].x - calib.x) * 3.9;
            double yg = (xyzbuf[(sent + i)].y - calib.y) * 3.9;
            double zg = (xyzbuf[(sent + i)].z - calib.z) * 3.9;


            dtostrf(xg, 7, 1, strx);
            dtostrf(yg, 7, 1, stry);
            dtostrf(zg, 7, 1, strz);
            sprintf(&buffer[strlen(buffer)], "{\"created\":%d,\"d1\":%s,\"d2\":%s,\"d3\":%s},", SAMPLING * (sent + i), strx, stry, strz);
        }
        buffer[strlen(buffer)-1] = '\0';
        sprintf(&buffer[strlen(buffer)], "]}\r\n");

        if (strlen(buffer) > BUFSIZE) {
            Serial.println("Message size exceeds buffer size. You should adjust BUFSIZE.");
            return;
        }

        DBG("buf: ");DBG(strlen(buffer));DBG(" bytes\r\n");
//        DBG(buffer);DBG("\r\n");

        int n = ambient.bulk_send(buffer);
        DBG("sent: ");DBG(n);DBG("\r\n");

        sent += datapersend;

        delay(5000); // bluk_send()であっても5秒間隔未満では送れない。
    }
}

void loop()
{
    while (true) {
        delay(0);
    }
}
