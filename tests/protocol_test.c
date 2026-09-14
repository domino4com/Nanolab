#include <assert.h>
#include <stdio.h>
#include "../src/CanStatusProtocol.h"
static uint8_t payload[25][8];
static uint32_t ids[25];
static unsigned make(unsigned length) {
    uint8_t mac[6]={0xcc,0x8d,0xa2,0x20,0x8d,0xd4};uint8_t body[CS_MAX_BODY]={0};
    cs_put64(body,0x1020304050607080ULL);cs_put32(body+8,1500);
    for(unsigned k=0;k<length;k++)body[12+k]='A'+k%26;
    uint16_t crc=cs_crc(mac,length,CS_OK,body);
    memset(payload,0,sizeof(payload));
    ids[0]=cs_id(CS_IDENTITY,0x8dd4,0);cs_put32(payload[0],123);memcpy(payload[0]+4,mac,4);
    ids[1]=cs_id(CS_HEADER,0x8dd4,0);cs_put32(payload[1],123);payload[1][4]=length;payload[1][6]=crc;payload[1][7]=crc>>8;
    unsigned frames=(12+length+3)/4;
    for(unsigned k=0;k<frames;k++){ids[k+2]=cs_id(CS_DATA,0x8dd4,k);cs_put32(payload[k+2],123);memcpy(payload[k+2]+4,body+k*4,4);}
    return frames+2;
}
static int feed(cs_assembly *a,unsigned k){return cs_receive(a,ids[k],payload[k],8,true,false);}
int main(void) {
    assert(cs_crc_update(0xffff,(const uint8_t*)"123456789",9)==0x29b1);
    for(unsigned len=0;len<=80;len++) {
        unsigned n=make(len);cs_assembly a;cs_reset(&a,0x8dd4,123);
        for(unsigned k=0;k<n;k++)assert(feed(&a,k)==(k==n-1?1:0));
        assert(a.length==len&&cs_get64(a.body)==0x1020304050607080ULL);
        cs_reset(&a,0x8dd4,123);
        for(unsigned k=n;k-->0;)assert(feed(&a,k)==(k==0?1:0));
        cs_reset(&a,0x8dd4,123);assert(feed(&a,0)==0);assert(feed(&a,0)==0);
        payload[0][4]^=1;assert(feed(&a,0)==-1);payload[0][4]^=1;
    }
    unsigned n=make(80);cs_assembly a;cs_reset(&a,0x8dd4,124);
    for(unsigned k=0;k<n;k++)assert(feed(&a,k)==0); // stale token
    cs_reset(&a,0x8dd4,123);payload[n-1][7]^=1;
    for(unsigned k=0;k<n;k++)assert(feed(&a,k)==(k==n-1?-1:0));
    make(80);cs_reset(&a,0x8dd4,123);payload[1][4]=81;assert(feed(&a,1)==-1);
    make(80);cs_reset(&a,0x8dd4,123);assert(cs_receive(&a,ids[0],payload[0],7,true,false)==-1);
    assert(cs_receive(&a,ids[0],payload[0],8,false,false)==0);
    assert(cs_receive(&a,ids[0],payload[0],8,true,true)==0);
    assert(cs_receive(&a,cs_id(CS_DATA,0x8dd4,23),payload[2],8,true,false)==-1);
    make(80);cs_reset(&a,0x8dd4,123);for(unsigned k=0;k<n-1;k++)assert(feed(&a,k)==0);
    puts("PASS: lengths 0..80, reverse ordering, duplicates/conflicts, CRC vector/corruption, stale tokens, bad length/DLC/index, RTR/standard frames, incomplete response.");
}
