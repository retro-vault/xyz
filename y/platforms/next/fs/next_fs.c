/* Embedded, read-only NEX filesystem. MIT; see ALTO LICENSE.
 * File payloads occupy reserved banks 32..47; process heaps use 16..31.
 * Public YOS POSIX wrappers retain descriptor and ownership semantics. */
#include <stdint.h>
typedef struct { char name[13]; uint8_t page; uint16_t offset,size; } entry_t;
extern const entry_t next_archive[];
extern const uint8_t next_archive_count;
extern void nex_copy(const void *source,void *dest,uint16_t count,uint8_t page);
static struct { const entry_t *file; uint16_t pos; uint8_t active,dir; } handles[16];
static uint8_t lower(uint8_t ch) { return ch >= 'A' && ch <= 'Z' ? ch+32 : ch; }
static const entry_t *find(const char *path) {
    const char *base=path,*p; uint16_t i; uint8_t j;
    for (p=path; *p; ++p) if (*p=='/' || *p==':') base=p+1;
    for (i=0;i<next_archive_count;++i) {
        for(j=0;j<13;++j) if(lower((uint8_t)base[j])!=lower((uint8_t)next_archive[i].name[j]) || !base[j]) break;
        if (j<13 && !base[j] && !next_archive[i].name[j]) return next_archive+i;
    }
    return 0;
}
static int16_t slot(void) { uint8_t i; for(i=0;i<16;++i) if(!handles[i].active) return i; return -1; }
int16_t nex_open(const char *path) {
    const entry_t *f=find(path); int16_t i;
    if(!f) return -1;
    i=slot(); if(i<0) return -1;
    handles[i].file=f; handles[i].pos=0; handles[i].active=1; handles[i].dir=0;
    return i+1;
}
int16_t nex_close(uint16_t handle) {
    if(!handle || handle>16 || !handles[handle-1].active) return -1;
    handles[handle-1].active=0; return 0;
}
int16_t nex_read(uint16_t handle,void *dest,uint16_t count) {
    uint16_t pos,remaining,done=0,n,at; uint8_t page; const entry_t *f;
    if(!handle || handle>16 || !handles[handle-1].active || handles[handle-1].dir) return -1;
    --handle; f=handles[handle].file; pos=handles[handle].pos;
    remaining=f->size-pos; if(count>remaining) count=remaining;
    while(done<count) {
        /* Add the sub-page offset separately to avoid 16-bit overflow. */
        at=f->offset+(pos&8191); page=f->page+(pos>>13)+(at>>13); at &= 8191;
        n=8192-at; if(n>count-done) n=count-done;
        nex_copy((void *)(0xA000+at),(uint8_t *)dest+done,n,page);
        done+=n; pos+=n;
    }
    handles[handle].pos=pos; return done;
}
int16_t nex_seek(uint16_t handle,uint16_t pos) {
    if(!handle || handle>16 || !handles[handle-1].active || handles[handle-1].dir || pos>handles[handle-1].file->size) return -1;
    handles[handle-1].pos=pos; return 0;
}
uint16_t nex_position(uint16_t handle) { return handles[handle-1].pos; }
static void status(const entry_t *f,uint8_t *out) {
    uint8_t i; for(i=0;i<11;++i) out[i]=0;
    out[2]=1; out[7]=(uint8_t)f->size; out[8]=f->size>>8;
}
int16_t nex_fstat(uint16_t handle,uint8_t *out) {
    if(!handle || handle>16 || !handles[handle-1].active || handles[handle-1].dir) return -1;
    status(handles[handle-1].file,out); return 0;
}
int16_t nex_stat(const char *path,uint8_t *out) {
    const entry_t *f=find(path); if(!f) return -1; status(f,out); return 0;
}
int16_t nex_opendir(const char *path) {
    int16_t i;
    if (path[0] && path[1]==':') path+=2;
    if (*path && !(path[0]=='/' && !path[1]) && !(path[0]=='.' && !path[1])) return -1;
    i=slot(); if(i<0) return -1;
    handles[i].active=1; handles[i].dir=1; handles[i].pos=0; return i+1;
}
int16_t nex_readdir(uint16_t handle,uint8_t *out) {
    const entry_t *f; uint8_t i,j;
    if(!handle || handle>16 || !handles[handle-1].active || !handles[handle-1].dir) return -1;
    --handle; if(handles[handle].pos>=next_archive_count) return 0;
    f=next_archive+handles[handle].pos++; out[0]=1;
    for(i=0;i<13 && f->name[i];++i) out[i+1]=f->name[i];
    out[++i]=0; ++i;
    for(j=0;j<4;++j) out[i++]=0;
    out[i++]=f->size; out[i++]=f->size>>8; out[i++]=0; out[i]=0;
    return 1;
}
int16_t nex_rewinddir(uint16_t handle) {
    if(!handle || handle>16 || !handles[handle-1].active || !handles[handle-1].dir) return -1;
    handles[handle-1].pos=0; return 0;
}
