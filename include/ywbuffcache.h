/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWBUFFCACHE_H_
#define INCLUDE_YWBUFFCACHE_H_

#include <ywcommon.h>
#include <ywutil.h>
#include <malloc.h>

typedef uint32_t ywBCID; /*Buffer Cache ID*/
typedef uint32_t ywHTID; /*Buffer Cache ID*/

/* con => Contents */
template <typename ConID, typename ConInfo, typename CacheBody>
class ywBuffCache {
    static const int32_t COLLISION_RETRY_COUNT = 5;
    static const int32_t HASHTABLE_RETRY_COUNT = 32;
    static const ssize_t BODY_SIZE = sizeof(CacheBody);

    static const ywBCID  NULL_BCID = -1;
    static const ywHTID  NULL_HTID = -1;

    typedef enum {
        STATUS_UNUSED = 0,
        STATUS_INIT   = 1
    } Status;

    class CacheInfo {
     public:
        ConID   con_id;
        Status  status;
        ConInfo sub_info;
    };

 public:
    ywBuffCache(intptr_t align_val, ywBCID count, ywHTID ht_size):
        _count(count), _ht_size(ht_size) {
        _init(align_val);
    }
    ywBuffCache(intptr_t align_val, ywBCID count):_count(count) {
        _ht_size = count;
        _init(align_val);
    }
    ~ywBuffCache() {
        _dest();
    }

    bool is_null(ywBCID bcid) {
        return bcid == NULL_BCID;
    }

    ywBCID alloc_cache() {
        ywBCID bcid;
        if (_hwm < _count) {
            do {
                bcid = _hwm;
            } while (!__sync_bool_compare_and_swap(&_hwm, bcid, bcid+1));
        } else {
            bcid = 0;
            do {
                ++bcid;
                if (bcid >= _count) {
                    bcid = 0;
                }
            } while (_info[bcid].status != STATUS_UNUSED);
        }

        new (&(_info[bcid].sub_info)) ConInfo();
        _info[bcid].status = STATUS_INIT;

        return bcid;
    }

    bool dealloc_cache(ywBCID bcid) {
        if (_info[bcid].status == STATUS_UNUSED) {
            return false;
        }
        _info[bcid].status = STATUS_UNUSED;
        return true;
    }

    CacheBody *get_body(ywBCID bcid) {
        return &_body[bcid];
    }

    ConInfo   *get_info(ywBCID bcid) {
        return &_info[bcid].sub_info;
    }

    /* HashTable*/
    bool ht_regist(ywBCID  bcid, ConID con_id,
                   int32_t try_count = HASHTABLE_RETRY_COUNT) {
        CacheInfo *info = &_info[bcid];
        ywHTID     idx  = _get_ht(con_id);
        int        i;

        info->con_id = con_id;

        assert(info->status != STATUS_UNUSED);

        do {
            if (_ht[idx] != NULL_BCID) { /* Collision */
                for (i = 0; i < COLLISION_RETRY_COUNT; ++i) {
                    ++idx;
                    if (idx >= _ht_size) {
                        idx = 0;
                    }
                    if (_ht[idx] == NULL_BCID) {
                        break;
                    }
                }
            }

            if (__sync_bool_compare_and_swap(&_ht[idx], NULL_BCID, bcid)) {
                return true;
            }
        } while ((--try_count) > 0);

        return false;
    }

    ywHTID ht_find(ConID con_id) {
        ywHTID  idx = _get_ht(con_id);
        ywBCID  bcid;
        int     i = COLLISION_RETRY_COUNT;

        do {
            bcid = _ht[idx];
            if (bcid == NULL_BCID) {
                return NULL_HTID;
            }
            if (_info[bcid].con_id == con_id) {
                if (_info[bcid].status != STATUS_UNUSED) {
                    return idx;
                }
            }
            ++idx;
        } while (--i);

        return NULL_HTID;
    }

    bool set_victim(ConID con_id) {
        ywHTID  htid = ht_find(con_id);

        if (htid == NULL_HTID) {
            return false;
        }

        ywBCID  bcid = _ht[htid];
        if (bcid == NULL_BCID) {
            return false;
        }

        CacheInfo *info = &_info[bcid];
        if ((info->con_id != con_id) || (info->status == STATUS_UNUSED)) {
            return false;
        }

        dealloc_cache(bcid);

        return __sync_bool_compare_and_swap(&_ht[htid], bcid, NULL_BCID);
    }

 private:
    ywHTID  _get_ht(ConID id) {
        return simple_hash(sizeof(id), reinterpret_cast<char*>(&id))
        %  _ht_size;
    }
    void _init(intptr_t align_val) {
        Byte     *aligned;
        intptr_t  end_ptr;
        size_t   alloc_size =
            _count*(BODY_SIZE + sizeof(CacheInfo))
            + _ht_size*sizeof(CacheInfo*)
            + align_val;

        if (!_count) {
            return;
        }

        _buffer = reinterpret_cast<Byte*>(malloc(alloc_size));
        aligned = reinterpret_cast<Byte*>(
            align(reinterpret_cast<intptr_t>(_buffer), align_val));
        end_ptr = reinterpret_cast<intptr_t>(_buffer) + alloc_size;

        _body = reinterpret_cast<CacheBody*>(aligned);
        aligned += _count * sizeof(*_body);

        _info = reinterpret_cast<CacheInfo*>(aligned);
        aligned += _count * sizeof(*_info);

        _ht = reinterpret_cast<ywBCID*>(aligned);
        aligned += _ht_size*sizeof(*_ht);

        assert(reinterpret_cast<intptr_t>(aligned) >= end_ptr - align_val);
        assert(reinterpret_cast<intptr_t>(aligned) <= end_ptr);

        /* initialize */
        _hwm = 0;
        memset(_ht, NULL_HTID, _ht_size*sizeof(*_ht));
        assert(_ht[0] == NULL_HTID);
    }
    void _dest() {
        if (_count) {
            free(_buffer);
        }
    }

    ywBCID       _count;
    ywBCID       _hwm;
    Byte        *_buffer;
    CacheBody   *_body;
    CacheInfo   *_info;

    ywHTID       _ht_size;
    ywBCID      *_ht; /*hash table */
};

#endif  // INCLUDE_YWBUFFCACHE_H_
