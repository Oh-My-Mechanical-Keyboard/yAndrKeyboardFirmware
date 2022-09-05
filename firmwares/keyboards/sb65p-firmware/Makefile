DIRS=sb65p-receiver-basic/custom/armgcc sb65p-keyboard-basic/custom/armgcc

all::
	make -C sb65p-receiver-basic/custom/armgcc; \
	make -C sb65p-keyboard-basic/custom/armgcc keyboard_side=left; \
	make -C sb65p-keyboard-basic/custom/armgcc keyboard_side=right; 

clean::
	make -C sb65p-receiver-basic/custom/armgcc clean; \
	make -C sb65p-keyboard-basic/custom/armgcc clean; \
	make -C sb65p-keyboard-basic/custom/armgcc clean; 