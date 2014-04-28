/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#include <ywchunkmgr.h>

uint32_t ywChunkMgr::alloc_chunk(int32_t type) {
    ywLockGuard<1/*size*/>  guard;
    uint32_t                ret;

    while (!guard.WLock(&map_lock)) { }

    ret = map_size;
    chunk_map[ret] = alloc_free_chunk(type);
    ++map_size;

    return ret;
}


