package my.syahmi.internetauto;

import android.app.IntentService;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.Context;
import android.content.IntentFilter;
import android.hardware.usb.UsbManager;

/**
 * An {@link IntentService} subclass for handling asynchronous task requests in
 * a service on a separate handler thread.
 * <p>
 * TODO: Customize class - update intent actions, extra parameters and static
 * helper methods.
 */
public class MyIntentService extends IntentService {
    private static final String ACTION_USB_PERMISSION = "com.android.example.USB_PERMISSION";

    private HuaweiModem mModem;

    public MyIntentService() {
        super("MyIntentService");
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        if (intent != null) {
            UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
            mModem = new HuaweiModem(usbManager);
            mModem.init();
            mModem.requestPermission(this);
        }
    }

}
