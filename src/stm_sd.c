#include "stm_sd.h"
#include <stdlib.h>
struct sd_context { sd_config_t cfg; sd_device_t device; sd_info_t info; };
static stm_err_t result(sd_handle_t d,stm_err_t e) {
    d->info.last_error=e;
    if(e==STM_ERR_IO || e==STM_ERR_TIMEOUT || e==SD_ERR_NO_MEDIA || e==STM_ERR_VERIFY) d->info.ready=0;
    return e;
}
stm_err_t sd_create(const sd_config_t *cfg,sd_handle_t *out) {
    if(!cfg || !out || !cfg->device) return STM_ERR_INVALID_ARG;
    if(*out) return STM_ERR_INVALID_STATE;
    const sd_host_ops_t *o=cfg->host.ops;
    if(!o || !o->check_context || !o->initialize || !o->status || !o->read || !o->write || !o->sync ||
       !cfg->timeout_ms || cfg->timeout_ms>=0x80000000U || !cfg->device->max_clock_hz) return STM_ERR_INVALID_CONFIG;
    stm_err_t e=o->check_context(cfg->host.ctx); if(e) return e;
    sd_handle_t d=calloc(1,sizeof(*d)); if(!d) return STM_ERR_NO_MEM;
    d->cfg=*cfg; d->device=*cfg->device; d->cfg.device=&d->device;
    e=o->initialize(cfg->host.ctx,d->device.max_clock_hz,&d->info.geometry);
    sd_geometry_t *g=&d->info.geometry;
    if(!e && (!g->sector_count || g->sector_size!=512 || !g->clock_hz ||
              g->clock_hz>d->device.max_clock_hz || g->sector_count>UINT64_MAX/512U)) e=STM_ERR_INVALID_CONFIG;
    if(e) { free(d); return e; }
    d->info.ready=1; *out=d; return STM_OK;
}
stm_err_t sd_delete(sd_handle_t *p) {
    if(!p) return STM_ERR_INVALID_ARG;
    if(*p) { stm_err_t e=(*p)->cfg.host.ops->check_context((*p)->cfg.host.ctx); if(e) return e; }
    free(*p); *p=NULL; return STM_OK;
}
stm_err_t sd_get_info(sd_handle_t d,sd_info_t *i) {
    if(!d || !i) return STM_ERR_INVALID_ARG;
    *i=d->info; return STM_OK;
}
stm_err_t sd_status(sd_handle_t d) {
    if(!d) return STM_ERR_INVALID_ARG;
    stm_err_t e=d->cfg.host.ops->check_context(d->cfg.host.ctx); if(e) return e;
    if(!d->info.ready) return STM_ERR_INVALID_STATE;
    return result(d,d->cfg.host.ops->status(d->cfg.host.ctx));
}
stm_err_t sd_sync(sd_handle_t d) {
    stm_err_t e=sd_status(d); if(e) return e;
    return result(d,d->cfg.host.ops->sync(d->cfg.host.ctx,d->cfg.timeout_ms));
}
static stm_err_t transfer(sd_handle_t d,void *rx,const void *tx,uint64_t sector,uint32_t n,int writing) {
    if(!d || (n && !(writing?tx:rx))) return STM_ERR_INVALID_ARG;
    if(sector>d->info.geometry.sector_count || n>d->info.geometry.sector_count-sector ||
       (uint64_t)n*512U>SIZE_MAX) return STM_ERR_OUT_OF_RANGE;
    stm_err_t e=sd_status(d); if(e || !n) return e;
    if(writing && d->info.geometry.write_protected) return SD_ERR_PROTECTED;
    const sd_host_ops_t *o=d->cfg.host.ops; void *c=d->cfg.host.ctx;
    e=writing?o->write(c,tx,sector,n,d->cfg.timeout_ms):o->read(c,rx,sector,n,d->cfg.timeout_ms);
    if(!e) e=o->sync(c,d->cfg.timeout_ms);
    return result(d,e);
}
stm_err_t sd_read_blocks(sd_handle_t d,void *data,uint64_t sector,uint32_t n) { return transfer(d,data,NULL,sector,n,0); }
stm_err_t sd_write_blocks(sd_handle_t d,const void *data,uint64_t sector,uint32_t n) { return transfer(d,NULL,data,sector,n,1); }
