#include <jni.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <linux/usbdevice_fs.h>

#include <cstdio>
#include <iostream>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>


std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}

std::string getUsb() {
    std::istringstream f(exec("lsusb"));
    std::string line;
    std::string path;

    while (std::getline(f, line)) {
        if (line.find("12d1:") != std::string::npos) {
            break;
        }
    }

    if (line.length()) {
        std::size_t nBus = line.find("Bus ");
        std::size_t nDev = line.find("Device ");
        if (nBus == std::string::npos || nDev == std::string::npos) {
            return "";
        }

        path = "/dev/bus/usb/";
        path += line.substr(nBus + 4, 3);
        path += "/";
        path += line.substr(nDev + 7, 3);
    }

    return path;
}

int resetUsb(const std::string& device) {
    int fd;
    int rc;

    fd = open(device.c_str(), O_WRONLY);
    if (fd < 0) {
        return errno;
    }

    rc = ioctl(fd, USBDEVFS_RESET, 0);
    close(fd);

    return rc;
}

extern "C" JNIEXPORT jstring JNICALL
Java_my_syahmi_internetauto_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    return env->NewStringUTF(getUsb().c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_my_syahmi_internetauto_MainActivity_resetUSB(
        JNIEnv* env,
        jobject /* this */) {

    setuid(0);
    setgid(0);
    seteuid(0);
    return resetUsb(getUsb());
}

