/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywflexow.h>

/********************************************************************
 *                      Multi Column
 * - Algorithm
 *   1) Add Column
 *   if (exist Unused Column?) {
 *      idx = recycle unused_column;
 *      ++version;
 *      cre_ver[idx] = version;
 *      mdoule[idx]  = new_mdoule;
 *   } else {
 *      idx = column_count++;
 *      cre_ver[idx] = version;
 *      mdoule[idx]  = new_mdoule;
 *   }
 *
 * - Image Format
 *   - Version(2B)
 *   - NormalColumnCount(1B)
 *   - ExtraColumnCount(1B)
 *   - OffsetMap(4B * (Normal + Extra - Fixed))
 *   - FixColumns(N)
 *   - VarColumns(M)
 ********************************************************************/

typedef struct {
    MCVersion  ver;
    MCColID    normal_count;
    MCColID    extra_count;
    type_info *col[COLUMN_MAX];
    ywtMCInfo *mc_info;
} ywtMC;

/*
void dump_ywtmc(ywtMC *val, T *ar, ywtMCMeta *meta) {
    MCColID    i;
    MCColID    var_col_cnt;
    ywtMCInfo *old;
    ywtMCInfo *cur;
    Byte      *body_pos;
    int32_t    offset = 0;
    int32_t    offsets[COLUMN_MAX] = 0;

    ar->initialize();
    AR_DUMP(dump_int, val->ver, "version");
    old = meta->get_info(val->ver);
    cur = meta->get_latest_info();
    val->mc_info = cur;

    AR_DUMP(dump_int, val->ver, "version");
    AR_DUMP(dump_int, val->normal_count, "normal column count");
    AR_DUMP(dump_int, val->extra_count,  "extra column count");
    var_col_cnt = val->normal_count  + val->extra_count - old->fix_count;
    body_pos = ar->get_cur_ptr() + (var_col_cnt * sizeof(offset));
    for (i = 0; i < var_col_cnt; ++i) {
        if (ar->is_writing()) {
        }
        AR_DUMP(dump_int, offsets[i],  "offset ");
        if (ar->is_reading()) {
        }
    }
    for (i = 0; i < old->fix_count; ++i) {
        AR_DUMP(dump, val->col, "column");
    }
}

const TypeModule MC_module = {
    TYPE_DECL(-1, ywtMC, read_ywtmc, write_ywtmc,
              print_ywtmc, comp_ywtmc,
              get_size_ywtmc)
};

*/
