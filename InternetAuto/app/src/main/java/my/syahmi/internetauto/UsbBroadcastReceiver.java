package my.syahmi.internetauto;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;

public class UsbBroadcastReceiver extends BroadcastReceiver {
    private static final String ACTION_USB_PERMISSION = "my.syahmi.internetauto.USB_PERMISSION";
    private static final String TAG = "InternetAuto";

    private HuaweiModem mModem;

    public UsbBroadcastReceiver(HuaweiModem modem) {
        this.mModem = modem;
    }

    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (ACTION_USB_PERMISSION.equals(action)) {
            synchronized (this) {
                UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                    if(device != null){
                        mModem.switchToEther();
                        context.unregisterReceiver(this);
                    }
                }
                else {
                    Log.d(TAG, "permission denied for device " + device);
                }
            }
        }
    }
}