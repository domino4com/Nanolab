/* SPDX-License-Identifier: MIT */
#ifndef CAN_STATUS_PROTOCOL_H
#define CAN_STATUS_PROTOCOL_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#define CS_VERSION 1
#define CS_MAX_TEXT 80
#define CS_META_SIZE 12
#define CS_MAX_BODY (CS_META_SIZE + CS_MAX_TEXT)
#define CS_PREFIX 0x1A000000u
#define CS_PREFIX_MASK 0x1E000000u
#define CS_POLL 0
#define CS_IDENTITY 1
#define CS_HEADER 2
#define CS_DATA 3
#define CS_ACK 4
#define CS_OK 0
#define CS_NO_DATA 1
static inline uint32_t cs_id(unsigned type, uint16_t node, unsigned fragment) {
    return CS_PREFIX | ((uint32_t)type << 22) | ((uint32_t)node << 6) | fragment;
}
static inline bool cs_is_id(uint32_t id) { return (id & CS_PREFIX_MASK)==CS_PREFIX && id <= 0x1fffffffu; }
static inline unsigned cs_type(uint32_t id) { return (id >> 22) & 7; }
static inline uint16_t cs_node(uint32_t id) { return (uint16_t)(id >> 6); }
static inline unsigned cs_index(uint32_t id) { return id & 63; }
static inline uint32_t cs_get32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static inline void cs_put32(uint8_t *p,uint32_t n) { for(unsigned i=0;i<4;i++) p[i]=(uint8_t)(n>>(8*i)); }
static inline uint64_t cs_get64(const uint8_t *p) { return cs_get32(p) | (uint64_t)cs_get32(p+4)<<32; }
static inline void cs_put64(uint8_t *p,uint64_t n) { cs_put32(p,(uint32_t)n);cs_put32(p+4,(uint32_t)(n>>32)); }
static inline uint16_t cs_crc_update(uint16_t crc,const uint8_t *p,unsigned n) {
    while(n--) { crc ^= (uint16_t)*p++ << 8;for(unsigned b=0;b<8;b++) crc=(crc&0x8000)?(uint16_t)((crc<<1)^0x1021):(uint16_t)(crc<<1); }
    return crc;
}
static inline uint16_t cs_crc(const uint8_t mac[6],uint8_t length,uint8_t status,const uint8_t *body) {
    uint8_t h[2]={length,status};
    uint16_t c=cs_crc_update(0xffff,mac,6);c=cs_crc_update(c,h,2);
    return cs_crc_update(c,body,CS_META_SIZE+length);
}
typedef struct {
    uint16_t node;uint32_t token;uint8_t mac[6],length,status,body[CS_MAX_BODY];
    uint32_t received;uint16_t crc;bool identity,header;
} cs_assembly;
static inline void cs_reset(cs_assembly *a,uint16_t node,uint32_t token) {
    memset(a,0,sizeof(*a));a->node=node;a->token=token;
}
/* 0 = incomplete/irrelevant; 1 = complete and CRC checked; -1 = malformed/conflicting. */
static inline int cs_receive(cs_assembly *a,uint32_t id,const uint8_t *data,unsigned dlc,bool extended,bool rtr) {
    if(!extended || rtr || !cs_is_id(id) || cs_node(id)!=a->node) return 0;
    unsigned type=cs_type(id),idx=cs_index(id);
    if(type<CS_IDENTITY || type>CS_DATA) return 0;
    if(dlc!=8) return -1;
    if(cs_get32(data)!=a->token) return 0;
    if(type==CS_IDENTITY) {
        if(idx) return -1;
        if(a->identity && memcmp(a->mac,data+4,4)) return -1;
        memcpy(a->mac,data+4,4);a->mac[4]=(uint8_t)(a->node>>8);a->mac[5]=(uint8_t)a->node;a->identity=true;
    } else if(type==CS_HEADER) {
        if(idx || data[4]>CS_MAX_TEXT || data[5]>CS_NO_DATA || (data[5]==CS_NO_DATA && data[4])) return -1;
        uint16_t crc=(uint16_t)data[6] | (uint16_t)data[7]<<8;
        if(a->header && (a->length!=data[4] || a->status!=data[5] || a->crc!=crc)) return -1;
        a->length=data[4];a->status=data[5];a->crc=crc;a->header=true;
    } else {
        if(idx >= (CS_MAX_BODY+3)/4) return -1;
        if((a->received&(1u<<idx)) && memcmp(a->body+idx*4,data+4,4)) return -1;
        memcpy(a->body+idx*4,data+4,4);a->received |= 1u<<idx;
    }
    if(!a->header || !a->identity) return 0;
    unsigned n=(CS_META_SIZE+a->length+3)/4;uint32_t mask=(1u<<n)-1;
    if(a->received & ~mask) return -1;
    if(a->received!=mask) return 0;
    return cs_crc(a->mac,a->length,a->status,a->body)==a->crc ? 1 : -1;
}
#endif
