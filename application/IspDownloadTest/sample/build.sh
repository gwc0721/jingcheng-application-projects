rm -f sm3257En_prog
clear
gcc -x c ForceModeBuffer.cpp sm3257En.cpp SMIFunc.cpp -Wall -lusb -I/usr/lib/ -o sm3257En_prog
