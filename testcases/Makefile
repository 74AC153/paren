PAREN=../paren

test:
	./run-tests.sh all

%.expect: %
	LD_LIBRARY_PATH=../ PAREN_LEAK_CHECK=1 ${PAREN} $< > $@

remake: base.000_load.expect \
	sio.000_putchar.expect sio.001_writestr.expect \
	test.000.expect test.001.expect test.002.expect test.003.expect \
	test.004.expect test.005.expect test.006.expect test.007.expect \
	test.008.expect test.009.expect test.010.expect test.011.expect \
	test.012.expect test.013.expect test.014.expect test.015.expect \
	test.016.expect test.017.expect test.018.expect test.019.expect \
	test.020.expect test.021.expect test.022.expect test.023.expect \
	test.024.expect test.025.expect test.026.expect test.027.expect \
	test.028.expect test.029.expect test.030.expect test.031.expect \
	test.032.expect test.033.expect test.034.expect test.035.expect \
	test.036.expect test.037.expect test.038.expect test.039.expect \
	test.040.expect test.041.expect
