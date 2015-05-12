CEF_INCLUDE_DIR=../cef/cef-git
CEF_LIB_DIR="$(dirname ../servo/libcef.so)"

gcc -o cef_test test.c -I${CEF_INCLUDE_DIR}  -g -O0 -Wall -lcef -L${CEF_LIB_DIR}
