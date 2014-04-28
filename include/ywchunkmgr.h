/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWCHUNKMGR_H_
#define INCLUDE_YWCHUNKMGR_H_

#include <ywcommon.h>
#include <ywspinlock.h>
#include <ywtypes.h>

class ywCnkID { /*ChunkID*/
 public:
    static const uint32_t NULL_CNK = 0;

    explicit ywCnkID() {
        type = NULL_CNK;
    }
    explicit ywCnkID(int32_t _type, int32_t _idx):type(_type), idx(_idx) {
    }

    bool is_null() {
        return type == NULL_CNK;
    }

    uint32_t get_int() {
        return *(reinterpret_cast<uint32_t*>(this));
    }
    uint32_t type:8;
    uint32_t idx:24;
};

class ywChunkMgr {
    static const uint32_t CHUNK_MAP_MAX = 1024;
    static const uint32_t NULL_CHUNK    = -1;

 public:
    explicit ywChunkMgr():map_size(0), chunk_hwm(0) {
    }

    uint32_t alloc_chunk(int32_t type);
    ywCnkID  get_cnk_id(int32_t idx) {
        return chunk_map[idx];
    }
    int32_t get_map_size() {
        return map_size;
    }

    template<typename T>
    bool dump(T *ar) {
        int32_t i;
        char    str[64];

        ar->dump(ywInt(chunk_hwm), "Chunk High-watermark");
        ar->dump(ywInt(map_size), "map_size");
        for (i = 0; i < map_size; ++i) {
            snprintf(str, sizeof(str)/sizeof(str[0]), "%d", i);
            ar->dump(ywInt(chunk_map[i].get_int()), str);
        }
        ar->finalize();

        return true;
    }

 private:
    ywCnkID alloc_free_chunk(int32_t type) {
        assert(map_lock.hasWLock());
        return ywCnkID(type, chunk_hwm++);
    }

    ywSpinLock              map_lock;
    ywCnkID                 chunk_map[CHUNK_MAP_MAX];
    int32_t                 map_size;
    int32_t                 chunk_hwm;
};

#endif  // INCLUDE_YWCHUNKMGR_H_
