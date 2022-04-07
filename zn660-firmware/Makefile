DIRS=zn660-receiver-basic/custom/armgcc zn660-keyboard-basic/custom/armgcc

all::
	make -C zn660-receiver-basic/custom/armgcc; \
	make -C zn660-keyboard-basic/custom/armgcc keyboard_side=left; \
	make -C zn660-keyboard-basic/custom/armgcc keyboard_side=right;

clean::
	make -C zn660-receiver-basic/custom/armgcc clean; \
	make -C zn660-keyboard-basic/custom/armgcc clean; \
	make -C zn660-keyboard-basic/custom/armgcc clean;