//============================================================================
// Name        : scandump.cpp
// Author      : Ruslan Trofymenko
// Version     :
// Copyright   : 
// Description :
//============================================================================

#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <fstream>
#include <string.h>
#include <sys/ioctl.h>
#include <vector>

#include <App.h>
#include <ustring.h>

using namespace std;

int main() {
	int fd;
	struct termios tty;
	int ret;
	int bytes;
	const int buff_size = 128;
	unsigned char buffer[buff_size];
	string DevicePath;// = "/dev/ttyUSB0";


	string Result = sys::CApp::Exec("find /sys/bus/usb-serial/drivers/cp210x/ -name ttyUSB*");
    vector<string> ResultStrings = Split(Result, '\n');

    if (ResultStrings.size() == 0) {
    	cerr << "usb-serial device not detected" << endl;
    	exit(EXIT_FAILURE);
    }

    if (ResultStrings.size() != 1) {
    	cerr << "clarify usb-serial device:" << endl;
    	for (auto& str: ResultStrings)
    		cerr << str << endl;
    	exit(EXIT_FAILURE);
    }

    string ttyDev = Result.substr(Result.find("ttyUSB"));
    RemoveNewLineChar(ttyDev);
    DevicePath = "/dev/" + ttyDev;




	fd = open(DevicePath.c_str(), O_RDWR|O_NOCTTY|O_SYNC/*|O_CLOEXEC*/);
	if (fd < 0) {
		cout << DevicePath << "not opened" << endl;
		exit(EXIT_FAILURE);
	}

//	ret = ioctl(fd, TIOCEXCL);
//    if (ret != 0) {
//        cout << DevicePath << "error TIOCEXCL ioctl" << endl;
//        exit(EXIT_FAILURE);
//    }

	tty.c_cflag = CBAUD | CS8 | CLOCAL | CREAD;
	tty.c_iflag = IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;//ICANON;
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;
	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);

	if (tcflush(fd,TCIFLUSH) == -1)
		exit(EXIT_FAILURE);
	if (tcflush(fd,TCOFLUSH) == -1)
		exit(EXIT_FAILURE);
	if (tcsetattr(fd, TCSANOW, &tty) == -1)
		exit(EXIT_FAILURE);

	unsigned char len;
	int id;
	while (true) {
		bytes = read(fd, buffer, buff_size);
		if (bytes <= 0) {
			cout << "error bytes" << endl;
			continue;
		}

		if (bytes != 21) {
			cout << "error bytes21" << endl;
			continue;
		}

		if (buffer[0] != 0xAA && buffer[1] != 0xAA) {
			cout << "error preamble" << endl;
			continue;
		}

		if (buffer[bytes - 1] != 0x55 && buffer[bytes - 2] != 0x55) {
			cout << "error post sign" << endl;
			continue;
		}

		memcpy(&id, &buffer[3], 4);
		len = buffer[14];

		fprintf(stdout, "%02X%02X%02X%02X\t%d\t", buffer[5], buffer[4], buffer[3], buffer[2], buffer[14]);
		for (int ii = 0; ii < len; ii++)
			fprintf(stdout, "%02X ", buffer[6 + ii]);

		fprintf(stdout, "\r\n");
	}

	return 0;
}
