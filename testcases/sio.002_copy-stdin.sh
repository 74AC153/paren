diff <( echo "hello world" ) <( echo "hello world" | LD_LIBRARY_PATH=../ PAREN_LEAK_CHECK=1 ../paren sio.002_copy-stdin )
