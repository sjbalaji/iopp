all: iotop

iotop: 
	gcc -o iotop iotop.c red_black_tree.c misc.c 
clean:
	rm -rf iotop *~ 	
