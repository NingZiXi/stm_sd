#include "stm_sd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL:%d:%s\n",__LINE__,#x); return 1; } } while (0)
static unsigned char disk[32*512]; static unsigned writes;
static stm_err_t check(void *ctx){return ctx?STM_OK:STM_ERR_INVALID_CONTEXT;}
static stm_err_t init(void *ctx,uint32_t max,sd_geometry_t *g){(void)ctx;g->sector_count=32;g->sector_size=512;g->clock_hz=max;g->erase_sectors=8;g->write_protected=0;return STM_OK;}
static stm_err_t status(void *ctx){return ctx?STM_OK:SD_ERR_NO_MEDIA;}
static stm_err_t readb(void *ctx,void *b,uint64_t s,uint32_t n,uint32_t t){(void)ctx;(void)t; if(s+n>32)return STM_ERR_OUT_OF_RANGE;memcpy(b,disk+s*512,n*512);return STM_OK;}
static stm_err_t writeb(void *ctx,const void *b,uint64_t s,uint32_t n,uint32_t t){(void)ctx;(void)t; if(s+n>32)return STM_ERR_OUT_OF_RANGE;memcpy(disk+s*512,b,n*512);writes++;return STM_OK;}
static stm_err_t syncb(void *ctx,uint32_t t){(void)ctx;(void)t;return STM_OK;}
static const sd_host_ops_t ops={check,init,status,readb,writeb,syncb};
int main(void){sd_handle_t d=NULL;sd_config_t c={&sd_xczsdnand4gas,{&ops,(void*)1},100};CHECK(sd_create(&c,&d)==STM_OK);unsigned char tx[1024],rx[1024];for(unsigned i=0;i<sizeof tx;i++)tx[i]=(unsigned char)i;CHECK(sd_write_blocks(d,tx,3,2)==STM_OK);CHECK(sd_read_blocks(d,rx,3,2)==STM_OK&&memcmp(tx,rx,sizeof tx)==0);CHECK(sd_read_blocks(d,rx,31,2)==STM_ERR_OUT_OF_RANGE);CHECK(writes==1);CHECK(sd_delete(&d)==STM_OK&&!d);return 0;}
