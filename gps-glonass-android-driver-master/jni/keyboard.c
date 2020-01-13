#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <linux/uinput.h>
#include "keyboard.h"
#include "gps.h"

int keyInited = 0;
int keyFd;

void emitKey(int type, int code, int val)
{
   struct input_event ie;

   if (keyInited == 0) return;
   
   ie.type = type;
   ie.code = code;
   ie.value = val;
   /* timestamp values below are ignored */
   ie.time.tv_sec = 0;
   ie.time.tv_usec = 0;

   int res = write(keyFd, &ie, sizeof(ie));
   // printf("emit write bytes=%d fd=%d code=%d val=%d\n",res, fd, code, val);
}

void initKey()
{
   struct uinput_user_dev uud;
   int version, rc;

   keyFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
   D("Open uinput fd = %d", keyFd);

   rc = ioctl(keyFd, UI_GET_VERSION, &version);
   D("Get uinput version = %d", rc);

   if (rc == 0 && version >= 5) 
   {
       D("Error, version not supported: %d", version);
       // return;
   }

   /*
    * The ioctls below will enable the device that is about to be
    * created, to pass key events, in this case the space key.
    */
   int i1 = ioctl(keyFd, UI_SET_EVBIT, EV_KEY);
   int i2 = ioctl(keyFd, UI_SET_EVBIT, EV_SYN);
   int i3 = ioctl(keyFd, UI_SET_KEYBIT, KEY_VOLUMEUP);
   int i4 = ioctl(keyFd, UI_SET_KEYBIT, KEY_VOLUMEDOWN);
   int i5 = ioctl(keyFd, UI_SET_KEYBIT, KEYCODE_MEDIA_PREVIOUS);
   int i6 = ioctl(keyFd, UI_SET_KEYBIT, KEYCODE_MEDIA_NEXT);
   int i7 = ioctl(keyFd, UI_SET_KEYBIT, KEY_ESC);

   D("ioctl = %d, %d, %d ,%d , %d, %d, %d", i1,i2,i3,i4,i5,i6,i7);

   memset(&uud, 0, sizeof(uud));
   snprintf(uud.name, UINPUT_MAX_NAME_SIZE, "uinput-keyboard");
   uud.id.bustype = BUS_HOST;
   uud.id.vendor  = 0x1;
   uud.id.product = 0x2;
   uud.id.version = 1;

   write(keyFd, &uud, sizeof(uud));
   sleep(2);

   int i = ioctl(keyFd, UI_DEV_CREATE);
   D("Keyboard created %d", i);
   
   keyInited = 1;
}