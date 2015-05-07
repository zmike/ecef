CEF_INCLUDE_DIR=
CEF_LIB_DIR=

gcc -o ecef *.c $(pkg-config --cflags --libs elementary ecore-x evas) -I ../cef-bin/cef_binary_3.2321.2067_linux64/  -g -O0 -Wall -lcef -L../cef-bin/cef_binary_3.2321.2067_linux64/Release -lGL
#gcc -o ecef *.c $(pkg-config --cflags --libs elementary ecore-x evas) -I ../cef-bin/cef_binary_3.2217.1922_linux64/  -g -O0 -Wall -lcef -L../cef-bin/cef_binary_3.2217.1922_linux64/out/Release -lGL
#gcc -o ecef *.c $(pkg-config --cflags --libs elementary ecore-x evas) -I ../cef-bin/cef_binary_3.2171.1901_linux64/  -g -O0 -Wall -lcef -L../cef-bin/cef_binary_3.2171.1901_linux64/out/Release -lGL

edje_cc ecef.edc
