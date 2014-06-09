#ifndef GRIDTABLE_H_
#define GRIDTABLE_H_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

class StdoutRenderer {
 public:
    static bool allow_null() {
        return true;
    }
    void begin_line() {
    }
    void end_line() {
        printf("\n");
    }

    void begin_table() {
        printf("===================================\n");
    }
    void column(ssize_t len, char * buf) {
        printf(buf);
    }
    void end_table() {
        printf("\n");
    }

    void special(ssize_t width, ssize_t type) {
        char    code[]={'-','='};
        ssize_t x;

        for (x = 0; x <= width; ++x) {
            putchar(code[type]);
        }
    }
};

class HtmlRenderer {
 public:
    static bool allow_null() {
        return true;
    }
    void begin_line() {
        printf("    <tr>\n");
    }
    void end_line() {
        printf("    </tr>\n");
    }

    void begin_table() {
        printf("<table border='1' cellpadding='0' cellspacing='0'>\n");
    }
    void column(ssize_t len, char * buf) {
        printf("        <td>\n");
        printf(buf);
        printf("        </td>\n");
    }
    void end_table() {
        printf("</table>\n");
    }

    void special(ssize_t width, ssize_t type) {
        printf("<hr/>");
    }
};


template <class Renderer>
class GridTable {
    static const ssize_t  WIDTH_LIMIT = 80;
    static const ssize_t  GRID_X    = 128;
    static const ssize_t  GRID_Y    = 1024;
    enum {
        SPECIAL_NORMAL = 0,
        SPECIAL_SINGLE_BAR = 1,
        SPECIAL_DOUBLE_BAR = 2
    };

 public:
    GridTable():buf_len(65536), buf_ptr(NULL),
        buf_used(0), widths_last_used(0),
        widths_max(0), max_x(-1), max_y(-1) {
        assert(GRID_Y % 8 == 0); // "GRID_Y must be a multiple of 4."

        buf_ptr = static_cast<char*>(malloc(buf_len));
        memset(map_bmp, 0, sizeof(map_bmp));
    }

    bool  set(ssize_t x,ssize_t y, int32_t val) {
        char buf[WIDTH_LIMIT];

        snprintf(buf, WIDTH_LIMIT, "%d", val);

        return set(x, y, buf);
    }
    bool  set(ssize_t x,ssize_t y, const char *str) {
        return set(x, y, const_cast<char*>(str));
    }
    bool  set(ssize_t x,ssize_t y, char *str) {
        if (!is_valid_range(x,y)) {
            return false;
        }

        ssize_t len = strlen(str);
        if (buf_used + 1 + len > buf_len) {
            return false;
        }

        set_map_off(x,y, buf_used);
        memcpy(buf_ptr + buf_used, str, len);
        buf_used += len;
        buf_ptr[buf_used] = '\0';
        buf_used++;
        special[y] = SPECIAL_NORMAL;
        if (max_y < y) {
            max_y = y;
        }

        return true;
    }

    void set_bar(ssize_t y, uint8_t type = SPECIAL_SINGLE_BAR) {
        special[y] = type;
        widths_last_used = 0;
        if (max_y < y) {
            max_y = y;
        }
    }

    char *get(ssize_t x, ssize_t y) {
        if (!is_exist(x,y)) {
            return const_cast<char*>(empty_string());
        }
        assert(map_off[x][y] <= buf_used);
        return buf_ptr + map_off[x][y];
    }
    void print() {
        assert(Renderer::allow_null() == true);

        print(NULL);
    }
    void print(Renderer *r) {
        ssize_t  x;
        ssize_t  y;

        calc_widths();

        r->begin_table();
        for (y = 0; y <= max_y; ++y) {
            r->begin_line();
            switch (special[y]) {
                case SPECIAL_NORMAL:
                    for (x = 0; x <= max_x; ++x) {
                        print_column_with_padding(r, get(x,y), widths[x]);
                    }
                    break;
                default:
                    r->special(widths_max, special[y]);
                    break;
            }
            r->end_line();
        }
        r->end_table();
    }

 private:
    void set_map_off(ssize_t x, ssize_t y, ssize_t off) {
        assert(is_valid_range(x,y));
        map_off[x][y] = off;

        uint32_t shift = y % 8;
        uint8_t  mask = 1<<shift;
        map_bmp[x][y/8] |= mask;
    }

    bool is_exist(ssize_t x, ssize_t y) {
        if (is_valid_range(x,y)) {
            return (map_bmp[x][y/8] >> (y % 8)) & 1;
        }
        return false;
    }

    bool is_valid_range(ssize_t x, ssize_t y) {
        return (0 <= x && x < GRID_X) && (0 <= y && y < GRID_Y);
    }

    void calc_widths() {
        ssize_t y;
        ssize_t x;
        ssize_t len;
        
        if (buf_used != widths_last_used) {
            widths_max = 0;
            max_x = 0;

            for (x = 0; x < GRID_X; ++x) {
                widths[x] = 0;
                for (y = 0; y <= max_y; ++y) {
                    len = strlen(get(x,y));
                    if (widths[x] < len) {
                        widths[x] = len;
                    }
                }
                if (widths_max + widths[x] > WIDTH_LIMIT) {
                    widths[x] = WIDTH_LIMIT - widths_max;
                    if (widths[x] >= 3) {
                        max_x = x;
                    }
                    break;
                }
                if (widths[x]) {
                    widths[x]++;
                    max_x = x;
                    widths_max += widths[x];
                }
            }
            widths_last_used = buf_used;
        }
    }

    void  print_column_with_padding(Renderer *r, char *str, int width) {
        ssize_t  len = strlen(str);
        char     buf[WIDTH_LIMIT+1];
        ssize_t  ren_len = 0;
        int32_t  i;

        for (i = 0; i < width; ++i) {
            if (len <= i) {
                buf[ren_len++] = ' ';
            } else {
                if ((len > width) && (width - i < 3)) {
                    buf[ren_len++] = '.';
                } else {
                    buf[ren_len++] = str[i];
                }
            }
        }
        buf[ren_len] = '\0';
        r->column(ren_len, buf);
    }

    const char     *empty_string(){return "";}

    ssize_t    buf_len;
    ssize_t    buf_used;
    char      *buf_ptr;

    ssize_t    widths_last_used;
    ssize_t    widths[GRID_X];
    ssize_t    widths_max;

    uint8_t    special[GRID_Y];


    ssize_t    max_x;
    ssize_t    max_y;
    ssize_t    map_off[GRID_X][GRID_Y];
    char       map_bmp[GRID_X][GRID_Y/8];
};

typedef GridTable<StdoutRenderer> GridTable_std;

#endif  // GRIDTABLE_H_
