/* SPDX-License-Identifier: MIT */
#pragma once
#include <Arduino.h>
#include <atomic>
#include "driver/twai.h"
#include "CanStatusProtocol.h"

class CanStatusClient {
public:
    struct Options {
        int txPin=7, rxPin=6, ledPin=40;
        uint32_t bitrate=250000;
        // Optional check against the derived MAC suffix; -1 accepts any suffix.
        int32_t expectedId=-1;
        uint32_t idleTimeoutMs=180000;
    };
    CanStatusClient() = default;
    CanStatusClient(const CanStatusClient&)=delete;
    CanStatusClient& operator=(const CanStatusClient&)=delete;
    bool begin();
    bool begin(const Options &options);
    // Copies a text snapshot. Returns false for >80 bytes, CR/LF, or control bytes.
    bool setData(const char *text);
    bool setData(const String &text) { return setData(text.c_str()); }
    uint16_t nodeId() const { return id_; }
    bool running() const { return task_!=nullptr; }
    uint32_t acknowledged() const { return acknowledgments_.load(std::memory_order_relaxed); }
    static void disableRadios();
private:
    static void entry(void *arg);
    void run();
    bool send(unsigned type,unsigned index,const uint8_t data[8]);
    void makeSnapshot(uint32_t token);
    void respond();
    void led(uint8_t r,uint8_t g,uint8_t b);
    Options options_;
    TaskHandle_t task_=nullptr;
    portMUX_TYPE mux_=portMUX_INITIALIZER_UNLOCKED;
    char text_[CS_MAX_TEXT+1]={0};
    uint8_t length_=0;
    bool hasData_=false, cached_=false;
    int64_t sampleUs_=0;
    uint16_t id_=0,crc_=0;
    uint8_t mac_[6]={0},body_[CS_MAX_BODY]={0},cachedLength_=0,cachedStatus_=CS_NO_DATA;
    uint32_t token_=0;
    std::atomic<uint32_t> acknowledgments_{0};
};
