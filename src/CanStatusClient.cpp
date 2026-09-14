/* SPDX-License-Identifier: MIT */
#include "CanStatusClient.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp32-hal-rgb-led.h"
#include <limits.h>

void CanStatusClient::disableRadios() {
    // Not-initialized/disabled errors are expected in radio-free sketches.
    esp_wifi_stop();
    esp_wifi_deinit();
#if CONFIG_BT_ENABLED
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
#endif
}
void CanStatusClient::led(uint8_t r,uint8_t g,uint8_t b) {
    if(options_.ledPin>=0) rgbLedWrite(options_.ledPin,r,g,b);
}
bool CanStatusClient::begin() { return begin(Options{}); }
bool CanStatusClient::begin(const Options &o) {
    if(task_) return false;
    options_=o;led(0,0,12);disableRadios();
    if(esp_efuse_mac_get_default(mac_)!=ESP_OK) {led(16,0,0);return false;}
    // Wire identity uses the true MAC suffix; overrides would break full-MAC reconstruction.
    id_=((uint16_t)mac_[4]<<8)|mac_[5];
    if(o.expectedId!=-1 && o.expectedId!=id_) {led(16,0,0);return false;}
    twai_general_config_t g=TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)o.txPin,(gpio_num_t)o.rxPin,TWAI_MODE_NORMAL);
    g.tx_queue_len=32;g.rx_queue_len=32;
    g.alerts_enabled=TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED | TWAI_ALERT_RX_QUEUE_FULL;
    twai_timing_config_t t;
    if(o.bitrate==250000) t=TWAI_TIMING_CONFIG_250KBITS();
    else if(o.bitrate==500000) t=TWAI_TIMING_CONFIG_500KBITS();
    else if(o.bitrate==125000) t=TWAI_TIMING_CONFIG_125KBITS();
    else {led(16,0,0);return false;}
    twai_filter_config_t f=TWAI_FILTER_CONFIG_ACCEPT_ALL();
    if(twai_driver_install(&g,&t,&f)!=ESP_OK) {led(16,0,0);return false;}
    if(twai_start()!=ESP_OK) {twai_driver_uninstall();led(16,0,0);return false;}
    if(xTaskCreate(entry,"can_status",6144,this,3,&task_)!=pdPASS) {
        task_=nullptr;twai_stop();twai_driver_uninstall();led(16,0,0);return false;
    }
    led(10,6,0);
    Serial.printf("CAN client id=%04x MAC=%02x:%02x:%02x:%02x:%02x:%02x bitrate=%lu\n",id_,mac_[0],mac_[1],mac_[2],mac_[3],mac_[4],mac_[5],(unsigned long)o.bitrate);
    return true;
}
bool CanStatusClient::setData(const char *text) {
    if(!text) return false;
    size_t n=strnlen(text,CS_MAX_TEXT+1);
    if(n>CS_MAX_TEXT) return false;
    for(size_t i=0;i<n;i++) if((uint8_t)text[i]<32 || (uint8_t)text[i]==127) return false;
    portENTER_CRITICAL(&mux_);
    memcpy(text_,text,n);text_[n]=0;length_=n;sampleUs_=esp_timer_get_time();hasData_=true;
    portEXIT_CRITICAL(&mux_);
    return true;
}
void CanStatusClient::entry(void *arg) { static_cast<CanStatusClient*>(arg)->run(); }
bool CanStatusClient::send(unsigned type,unsigned index,const uint8_t data[8]) {
    twai_message_t m={};m.identifier=cs_id(type,id_,index);m.extd=1;m.ss=1;m.data_length_code=8;memcpy(m.data,data,8);
    return twai_transmit(&m,pdMS_TO_TICKS(30))==ESP_OK;
}
void CanStatusClient::makeSnapshot(uint32_t token) {
    token_=token;memset(body_,0,sizeof(body_));
    int64_t now,sample;
    portENTER_CRITICAL(&mux_);
    cachedLength_=length_;cachedStatus_=hasData_?CS_OK:CS_NO_DATA;sample=sampleUs_;
    memcpy(body_+CS_META_SIZE,text_,length_);
    portEXIT_CRITICAL(&mux_);
    now=esp_timer_get_time();
    cs_put64(body_,now/1000);
    uint64_t age=cachedStatus_==CS_OK?(uint64_t)((now-sample)/1000):UINT32_MAX;
    cs_put32(body_+8,age>UINT32_MAX?UINT32_MAX:(uint32_t)age);
    crc_=cs_crc(mac_,cachedLength_,cachedStatus_,body_);cached_=true;
}
void CanStatusClient::respond() {
    uint8_t d[8]={0};cs_put32(d,token_);memcpy(d+4,mac_,4);
    twai_clear_transmit_queue();
    if(!send(CS_IDENTITY,0,d)) return;
    d[4]=cachedLength_;d[5]=cachedStatus_;d[6]=crc_;d[7]=crc_>>8;
    if(!send(CS_HEADER,0,d)) return;
    unsigned count=(CS_META_SIZE+cachedLength_+3)/4;
    for(unsigned i=0;i<count;i++) {
        memcpy(d+4,body_+4*i,4);
        if(!send(CS_DATA,i,d)) return;
    }
}
void CanStatusClient::run() {
    int64_t lastRequest=0;
    while(true) {
        twai_status_info_t state;
        if(twai_get_status_info(&state)==ESP_OK) {
            if(state.state==TWAI_STATE_BUS_OFF) {led(16,0,0);twai_initiate_recovery();}
            else if(state.state==TWAI_STATE_STOPPED) {if(twai_start()==ESP_OK) led(10,6,0);}
        }
        twai_message_t m={};
        if(twai_receive(&m,pdMS_TO_TICKS(100))==ESP_OK && m.extd && !m.rtr && m.data_length_code==8 && cs_is_id(m.identifier) && cs_node(m.identifier)==id_ && cs_index(m.identifier)==0) {
            if(cs_type(m.identifier)==CS_POLL && m.data[0]==CS_VERSION && m.data[1]==0 && m.data[6]==0 && m.data[7]==0) {
                uint32_t token=cs_get32(m.data+2);
                if(!cached_ || token!=token_) makeSnapshot(token);
                lastRequest=esp_timer_get_time();led(0,8,10);respond();
            } else if(cs_type(m.identifier)==CS_ACK && cached_ && cs_get32(m.data)==token_ && m.data[4]==0) {
                acknowledgments_++;led(0,12,0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        if(lastRequest && esp_timer_get_time()-lastRequest>(int64_t)options_.idleTimeoutMs*1000) {led(10,6,0);lastRequest=0;}
    }
}
