LIBUSB_PATH=libusb-1.0-win
SRC=src/main.cpp
OUT=franklin_u600_mode_switch.exe

all:
	i686-w64-mingw32-g++ "-I$(LIBUSB_PATH)/include" $(SRC) "$(LIBUSB_PATH)/MinGW32/static/libusb-1.0.a" -static  -o "$(OUT)"
clean:
	rm "$(OUT)"
