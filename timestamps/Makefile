obj-m += timestamps.o	#Genera el build
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) 	#Utilizará los módulos que tiene el nombre del kernel y hace el build, M->en este directorio (shell pwd)
modulesclean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean	# 