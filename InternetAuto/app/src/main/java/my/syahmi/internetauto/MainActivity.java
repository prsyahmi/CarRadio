package my.syahmi.internetauto;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;

// Reference: https://groups.google.com/forum/#!topic/android-porting/ga_qoiyRTd4
// First we switch the USB to CDC (ECM)
//    55534243123456780000000000000011062000000101010100000000000000 - CDC ECM
//    55534243123456780000000000000011063000000100010000000000000000 - MODEM?
// Running the command twice will unregister CDC ethernet!
// After that we need to setup the ethernet connection once the CDC interface shows up
//    netcfg eth1 up
//    netcfg eth1 dhcp
// By this time, we can ping any IP, however domain name resolution isn't setup yet
// Get the gateway and dns from dhcp and use it on the following command
//    ndc network create 100
//    ndc network interface add 100 eth1
//    ndc network route add 100 eth1 <GATEWAY>/24
//    ndc network route add 100 eth1 0.0.0.0/0 <DNS1>
//    ndc resolver setnetdns 100 localdomain <DNS1>
//    ndc network default set 100
// Now we can ping google.com (domain name) instead of IP
// Make sure system clock is OK
// Android system can be notified of network availability

public class MainActivity extends Activity {
    private static final String ACTION_USB_PERMISSION = "com.android.example.USB_PERMISSION";
    private static final String TAG = "InternetAuto";

    private HuaweiModem mModem;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        mModem = new HuaweiModem(usbManager);

        mModem.init();
        mModem.requestPermission(this);

        this.finish();
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native int resetUSB();
}
