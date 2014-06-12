OUTPUT=$1

rm -f $OUTPUT
touch $OUTPUT

PORTABLE_FILES=`find src/*.cc -print | sort | sed "s,^$PREFIX/,," | tr "\n" " "`
echo "SOURCES=$PORTABLE_FILES $PORT_FILE" >> $OUTPUT

PORTABLE_FILES=`find main/*.cc -print | sort | sed "s,^$PREFIX/,," | tr "\n" " "`
echo "MAIN_SRC=$PORTABLE_FILES $PORT_FILE" >> $OUTPUT
