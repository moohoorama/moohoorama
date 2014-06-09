#include <stdio.h>
#include "gridtable.h"

int main(int argc, char **argv) {
    GridTable<HtmlRenderer> gt;

    gt.print();
    gt.set(0,0,"abc");
    gt.print();

    gt.set(0,0,"name");
    gt.set(1,0,"tel");
    gt.set(2,0,"addr");
    gt.print();

    gt.set(0,0,"no");
    gt.set(1,0,"name");
    gt.set(2,0,"tel");
    gt.set(3,0,"addr");
    gt.print();

    gt.set(0, 1, 1);
    gt.set(1, 1, "Younwoo");
    gt.set(2, 1, "010-2565-3903");
    gt.set(3, 1, "Ahnyang");
    gt.print();

    gt.set(0, 2, 1);
    gt.set(1, 2, "Younwoo");
    gt.set(2, 2, "010-2565-3903");
    gt.set(3, 2, "Ahnyang");
    gt.set_bar(1);
    gt.set(1, 3, "[               ]");
    gt.print();

    return 0;
}
