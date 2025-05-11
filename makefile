.PHONY: help
help:
	cat makefile

src:
	git clone git@github.com:Dimka4369/3D-Engine.git src

.PHONY: pull
pull: | src
	cd src && git pull

.PHONY: clean
clean:
	rm -f \
		Screen?.bmp \
		example-3-c \
		first-test \
		main-ppm \
		transform-test \
		voxel-c \
		voxel-c.ppm \
		voxel-cpp \
		voxel-cpp.ppm

first-test: src/FirstTest.cpp src/3Dengine.h src/3Dengine.cpp src/bmpProcessing.h src/bmpProcessing.cpp
	g++ -O2 -o first-test -Isrc src/FirstTest.cpp src/3Dengine.cpp src/bmpProcessing.cpp

.PHONY: run-first-test
run-first-test: first-test
	./first-test

transform-test: src/TransformTest.cpp src/3Dengine.h src/3Dengine.cpp
	g++ -O2 -o transform-test -Isrc src/TransformTest.cpp src/3Dengine.cpp

.PHONY: run-transform-test
run-transform-test: transform-test
	./transform-test

main-ppm: src/Main_ppm.cpp src/Ray_Branch2_v1.h
	g++ -O2 -o main-ppm -Isrc src/Main_ppm.cpp

.PHONY: run-main-ppm
run-main-ppm: main-ppm
	./main-ppm

voxel-c : voxel.c
	gcc \
		-g \
		-std=c11 \
		-pedantic \
		-Wall \
		-Wextra \
		-Werror \
		-Wwrite-strings \
		-Wconversion \
		-Wno-unused-parameter \
		`pkg-config --cflags gtk4` \
		voxel.c \
		-ovoxel-c \
		`pkg-config --libs gtk4` \
	    -lm

voxel-cpp : voxel.cpp
	g++ \
		-std=c++23 \
		-pedantic \
		-Wall \
		-Wextra \
		-Werror \
		-Wwrite-strings \
		-Wconversion \
		`pkg-config --cflags gtk4` \
		voxel.cpp \
		-ovoxel-cpp \
		`pkg-config --libs gtk4` \
	    -lm

voxel-c.ppm: voxel-c
	./voxel-c test-ppm

voxel-cpp.ppm: voxel-cpp
	./voxel-cpp test-ppm

.PHONY: voxel-c-test-gtk
voxel-c-test-gtk: voxel-c
	./voxel-c test-gtk

.PHONY: valgrind-voxel-c-test-gtk
valgrind-voxel-c-test-gtk: voxel-c
	valgrind \
		--suppressions=/usr/share/gtk-4.0/valgrind/gtk.supp \
		./voxel-c \
		test-gtk

example-3-c : example-3.c
	gcc \
		-g \
		-std=c11 \
		-pedantic \
		-Wall \
		-Wextra \
		-Werror \
		-Wwrite-strings \
		-Wconversion \
		-Wno-unused-parameter \
		`pkg-config --cflags gtk4` \
		example-3.c \
		-oexample-3-c \
		`pkg-config --libs gtk4`

.PHONY: valgrind-example-3-c
valgrind-example-3-c: example-3-c
	valgrind \
		--suppressions=/usr/share/gtk-4.0/valgrind/gtk.supp \
		./example-3-c
