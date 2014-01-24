OUTPUT=$1

rm -f $OUTPUT
touch $OUTPUT

PORTABLE_FILES=`find src/*.c -print | sort | sed "s,^$PREFIX/,," | tr "\n" " "`

echo "SOURCES=$PORTABLE_FILES $PORT_FILE" >> $OUTPUT
