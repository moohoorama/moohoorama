#include <ywchunkmgr.h>

uint32_t ywChunkMgr::alloc_chunk(int32_t type) {
    ywSeqLockGuard guard(map_lock);
    uint32_t       ret;

    while (!map_lock.lock()) { }

    ret = map_size;
    chunk_map[ret] = alloc_free_chunk(type);
    ++map_size;

    return ret;
}


