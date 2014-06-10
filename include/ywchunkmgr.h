/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWCHUNKMGR_H_
#define INCLUDE_YWCHUNKMGR_H_

#include <ywcommon.h>
#include <ywspinlock.h>
#include <ywtypes.h>

typedef uint32_t ywCnkID;
typedef uint32_t ywCnkLoc;

class ywCnkInfo { /*ChunkInfo*/
 public:
    static const uint32_t NULL_CNK = 0;

    explicit ywCnkInfo() {
        type = NULL_CNK;
    }
    explicit ywCnkInfo(int32_t _type, int32_t _p_loc):
        type(_type), p_loc(_p_loc) {
    }

    bool is_null() {
        return type == NULL_CNK;
    }
    void set_null() {
        type = NULL_CNK;
    }

    uint32_t get_int() {
        return *(reinterpret_cast<uint32_t*>(this));
    }
    uint32_t type:8;
    ywCnkLoc p_loc:24;   /*physical location*/
};

class ywChunkMgr {
    static const uint32_t CHUNK_MAP_MAX = 65536;
    static const ywCnkID  NULL_CNKID    = -1;

 public:
    explicit ywChunkMgr():last_id(0), used_cnk_loc(0) {
    }

    bool is_null(ywCnkID id) {
        return id == NULL_CNKID;
    }

    ywCnkID alloc_chunk(int32_t type) {
        ywLockGuard<1/*size*/>  guard;
        ywCnkID                 new_id;
        ywCnkLoc                new_loc;

        if (!guard.WLock(&map_lock)) {
            return NULL_CNKID;
        }

        new_id  = _alloc_cnk_id();
        new_loc = _find_free_space();
        if (!_regist_cnk(new_id, type, new_loc)) {
            _free_last_cnk_id(new_id);
            _reclaim_free_space(new_loc);
            return NULL_CNKID;
        }

        return new_id;
    }

    ywCnkInfo  get_cnk_info(ywCnkID id) {
        return _get_cnk_info(id);
    }

    template<typename T>
    bool dump(T *ar) {
        int32_t i;
        char    str[64];

        ar->initialize();
        ar->dump(ywInt(last_id), "last_id");
        ar->dump(ywInt(used_cnk_loc), "last alloc chunk loc");
        for (i = 0; i < last_id; ++i) {
            snprintf(str, sizeof(str)/sizeof(str[0]), "%d", i);
            ar->dump(ywInt(chunk_map[i].get_int()), str);
        }
        ar->finalize();

        return true;
    }

 private:
    ywCnkID   _alloc_cnk_id() {
        assert(map_lock.hasWLock());
        assert(last_id != NULL_CNKID);
        return last_id++;
    }
    void _free_last_cnk_id(ywCnkID new_id) {
        assert(map_lock.hasWLock());
        assert(new_id == last_id -1);
        --last_id;
    }

    ywCnkLoc _find_free_space() {
        return used_cnk_loc++;
    }
    void _reclaim_free_space(ywCnkLoc free_loc) {
        assert(map_lock.hasWLock());
        assert(free_loc == used_cnk_loc -1);
        --used_cnk_loc;
    }

    bool _regist_cnk(ywCnkID id, uint32_t type, ywCnkLoc p_loc) {
        if (id >= CHUNK_MAP_MAX) {
            return false;
        }

        chunk_map[id] = ywCnkInfo(type, p_loc);
        return true;
    }

    ywCnkInfo  _get_cnk_info(ywCnkID id) {
        return chunk_map[id];
    }

    ywSpinLock              map_lock;
    ywCnkInfo               chunk_map[CHUNK_MAP_MAX];
    ywCnkID                 last_id;
    ywCnkLoc                used_cnk_loc;
};

#endif  // INCLUDE_YWCHUNKMGR_H_
