call C:\Users\prsyahmi\AppData\Local\Android\Sdk\ndk-bundle\ndk-build.cmd
C:\tools\platform-tools\adb.exe shell mount -o remount,rw /system
C:\tools\platform-tools\adb.exe push "D:\Developments\Software\Car\CarRadio\keepaliveaudio\libs\armeabi-v7a\keepaliveaudio" /system/bin/
C:\tools\platform-tools\adb.exe shell chmod 777 /system/bin/keepaliveaudio
pause