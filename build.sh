CEF_INCLUDE_DIR=
CEF_LIB_DIR=

gcc -o ecef -DHAVE_SERVO $(ls *.c|grep -v test.c) $(pkg-config --cflags --libs elementary ecore-x evas) -I ../cef/cef-git  -g -O0 -Wall -lcef -L../servo
#gcc -o ecef *.c $(pkg-config --cflags --libs elementary ecore-x evas) -I ../cef-bin/cef_binary_3.2321.2067_linux64/  -g -O0 -Wall -lcef -L../cef-bin/cef_binary_3.2321.2067_linux64/Release -lGL
#gcc -o ecef *.c $(pkg-config --cflags --libs elementary ecore-x evas) -I ../cef-bin/cef_binary_3.2217.1922_linux64/  -g -O0 -Wall -lcef -L../cef-bin/cef_binary_3.2217.1922_linux64/out/Release -lGL
#gcc -o ecef *.c $(pkg-config --cflags --libs elementary ecore-x evas) -I ../cef-bin/cef_binary_3.2171.1901_linux64/  -g -O0 -Wall -lcef -L../cef-bin/cef_binary_3.2171.1901_linux64/out/Release -lGL

edje_cc -id icons-dd ecef.edc
