CON_ROOT_PATH=/tmp/mycontainer_root
TARGET=capsule

all: build

build:
	gcc -Isrc -Isrc/libcapsule src/main.c src/libcapsule/libcapsule.c -Wall -o "${TARGET}"

setup:
	debootstrap --variant=minbase bookworm "${CON_ROOT_PATH}" http://deb.debian.org/debian

clean:
	rm -f "${TARGET}"
	rm -fr "${CON_ROOT_PATH}"
