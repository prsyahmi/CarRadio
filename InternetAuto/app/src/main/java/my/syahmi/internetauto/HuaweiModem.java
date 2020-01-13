package my.syahmi.internetauto;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;

public class HuaweiModem {
    private static final String ACTION_USB_PERMISSION = "my.syahmi.internetauto.USB_PERMISSION";
    private static final String TAG = "InternetAuto";
    private UsbManager mManager;
    private UsbDevice mDevice = null;
    private UsbBroadcastReceiver mUsbReceiver = new UsbBroadcastReceiver(this);
    CommThread mCommThread;

    HuaweiModem(UsbManager manager) {
        this.mManager = manager;
    }

    public boolean init() {
        HashMap<String, UsbDevice> deviceList = mManager.getDeviceList();
        Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
        while(deviceIterator.hasNext()){
            UsbDevice device = deviceIterator.next();
            if (device.getVendorId() == 0x12d1) {
                Log.v(TAG, "Found huawei modem. Product ID = " + device.getProductId());
                if (device.getProductId() == 0x1f01) {
                    Log.v(TAG, "Found huawei target mass storage");
                    mDevice = device;
                    return true;
                }
            }
        }

        return false;
    }

    public void requestPermission(Context context) {
        if (mDevice != null) {
            PendingIntent permissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);
            IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
            context.registerReceiver(mUsbReceiver, filter);
            mManager.requestPermission(mDevice, permissionIntent);
        } else {
            switchToEther();
        }
    }

    public void switchToEther() {
        boolean forceClaim = true;
        UsbEndpoint epIN = null;
        UsbEndpoint epOUT = null;
        UsbDeviceConnection conn = null;

        if (mDevice != null) {
            UsbInterface usbIf = mDevice.getInterface(0);
            conn = mManager.openDevice(mDevice);
            conn.claimInterface(usbIf, forceClaim);

            Log.d(TAG, "Endpoint count: " + usbIf.getEndpointCount());
            for (int i = 0; i < usbIf.getEndpointCount(); i++) {
                Log.d(TAG, "EP: " + String.format("0x%02X", usbIf.getEndpoint(i).getAddress()));
                if (usbIf.getEndpoint(i).getType() == UsbConstants.USB_ENDPOINT_XFER_BULK) {
                    Log.d(TAG, "Found Bulk Endpoint at " + i);
                    if (usbIf.getEndpoint(i).getDirection() == UsbConstants.USB_DIR_IN)
                        epIN = usbIf.getEndpoint(i);
                    else
                        epOUT = usbIf.getEndpoint(i);
                } else {
                    Log.d(TAG, "Skipping endpoint " + i + ": Not Bulk " + usbIf.getEndpoint(i).getType());
                }
            }
        }

        if (mCommThread == null || !mCommThread.isAlive()) {
            mCommThread = new CommThread(epIN, epOUT, conn);
            mCommThread.start();
        } else {
            Log.e(TAG, "Communication thread is still running");
        }
    }

    public boolean hasModemAppeared() {
        HashMap<String, UsbDevice> deviceList = mManager.getDeviceList();
        Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
        while(deviceIterator.hasNext()){
            UsbDevice device = deviceIterator.next();
            if (device.getVendorId() == 0x12d1) {
                Log.v(TAG, "Found huawei modem. Product ID = " + device.getProductId());
                if (device.getProductId() == 0x14db) {
                    Log.v(TAG, "Found huawei target modem");
                    return true;
                }
            }
        }

        return false;
    }

    class CommThread extends Thread {
        UsbEndpoint epIN;
        UsbEndpoint epOUT;
        UsbDeviceConnection conn;

        CommThread(UsbEndpoint epIN, UsbEndpoint epOUT, UsbDeviceConnection conn) {
            if (epIN == null ) {
                Log.e(TAG, "epIN is null!");
            }
            if ( epOUT == null ) {
                Log.e(TAG, "epOUT is null!");
            }
            if ( conn == null ) {
                Log.e(TAG, "conn is null!");
            }

            this.epIN = epIN;
            this.epOUT = epOUT;
            this.conn = conn;
        }

        public void run() {
            int count;
            ExecuteAsRootBase root = new ExecuteAsRootBase();

            Log.d(TAG, "Starting modem command...");

            try {
                Thread.sleep(5000);
            } catch (InterruptedException e) {
                Log.e(TAG,"Thread sleep interrupted");
            }

            Log.v(TAG, "Getting root");
            count = 10;
            while (!ExecuteAsRootBase.canRunRootCommands()) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    Log.e(TAG,"Thread sleep interrupted");
                }

                if (--count <= 0) {
                    Log.e(TAG, "Can't get root, skipping anyway");
                    break;
                }
                Log.e(TAG, "Unable to get root, retrying");
            }

            SetupInterfaceRename(root);
            if (epIN != null || epOUT != null || conn != null) {
                // xfer("55534243123456780000000000000011063000000100010000000000000000"); // modem?
                xfer("55534243123456780000000000000011062000000101010100000000000000"); // huawei ECM
            }


            // Wait for modem to appear...
            Log.v(TAG, "Waiting for modem to appear");
            count = 5;
            while (!hasModemAppeared()) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    Log.e(TAG,"Thread sleep interrupted");
                }
                if (--count <= 0) {
                    break;
                }
            }

            Log.v(TAG, "Running series of command");

            Log.v(TAG, "Checking date first");
            Calendar calendar = Calendar.getInstance();
            int year = calendar.get(Calendar.YEAR);
            int month = calendar.get(Calendar.MONTH);
            if (year < 2018 && month < 10) {
                Log.v(TAG, "Date is behind: " + year + "-" + month);
                Log.v(TAG, "Correcting date first");
                root.addCommand("date -s 20181014.130000");
                root.execute();
                root.clear();
            }

            root.addCommand("netcfg eth0 up");
            root.execute();
            root.clear();
        }

        private void SetupInterfaceRename(ExecuteAsRootBase root) {
            // Try to rename existing eth0 to eth1, since android ethernet manager
            // only support eth0 and ignore the rest.
            // https://android.googlesource.com/platform/frameworks/opt/net/ethernet/+/e3cbf2e7349dd366f33905784595e2e3a1cec245/java/com/android/server/ethernet/EthernetNetworkFactory.java#61
            // http://kernelpanik.net/rename-a-linux-network-interface-without-udev/

            String eth0mac = readFile("/sys/class/net/eth0/address");
            Log.v(TAG, "eth0 mac address: " + eth0mac);
            if (eth0mac.contains("0c:5b:8f:27:9a:64")) {
                // This mac address should valid for CDC across some Huawei modem.
                return;
            }

            root.addCommand("netcfg eth0 down");
            root.addCommand("ip link set eth0 name eth2");
            root.addCommand("netcfg eth2 up");

            // If the android is restarted, the modem might already in modem mode and got eth1.
            // Try to rename to eth0 if it exists.
            root.addCommand("netcfg eth1 down");
            root.addCommand("ip link set eth1 name eth0");

            root.execute();
            root.clear();
        }

        private void SetupInterfaceManually(ExecuteAsRootBase root) {
            // Below is old one that we setup manually. Since the android ethernet manager
            // only support one interface, we need to manually setup.

            root.addCommand("netcfg eth1 down");
            root.execute();
            root.clear();


            // Assume DNS/Gateway is at 192.168.8.0/24
            root.addCommand("ndc interface clearaddrs eth1");
            root.addCommand("ndc resolver clearnetdns 100");
            root.addCommand("ndc network destroy 100");
            root.execute();
            root.clear();

            root.addCommand("netcfg eth1 up");
            root.addCommand("netcfg eth1 dhcp");
            root.execute();
            root.clear();


            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                Log.e(TAG,"Thread sleep interrupted");
            }

            root.addCommand("ndc network create 100");
            root.addCommand("ndc network interface add 100 eth1");
            root.addCommand("ndc network route add 100 eth1 192.168.8.0/24");
            root.addCommand("ndc network route add 100 eth1 0.0.0.0/0 192.168.8.1");
            root.addCommand("ndc resolver setnetdns 100 localdomain 192.168.8.1");
            // root.addCommand("ndc resolver setnetdns eth1  1.1.1.1 1.0.0.1");
            root.addCommand("ndc network default set 100");
            root.execute();
        }

        private String readFile(String path) {
            File file = new File(path);
            StringBuilder text = new StringBuilder();

            try {
                BufferedReader br = new BufferedReader(new FileReader(file));
                String line;

                while ((line = br.readLine()) != null) {
                    text.append(line);
                    text.append('\n');
                }
                br.close();
            }
            catch (IOException e) {
                //You'll need to add proper error handling here
            }

            return text.toString();
        }

        private void xfer(String s) {
            int len = s.length();
            byte[] data = new byte[len / 2];
            for (int i = 0; i < len; i += 2) {
                data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                        + Character.digit(s.charAt(i+1), 16));
            }

            final int TIMEOUT = 200;
            Log.d(TAG, " > " + s);
            conn.bulkTransfer(epOUT, data, data.length, TIMEOUT);

            byte[] buffer = new byte[4096];
            StringBuilder str = new StringBuilder();

            if (conn.bulkTransfer(epIN, buffer, buffer.length, TIMEOUT) >= 0) {
                for (int i = 2; i < 4096; i++) {
                    if (buffer[i] != 0) {
                        str.append((char) buffer[i]);
                    } else {
                        Log.d(TAG, " < " + str);
                        break;
                    }
                }
            }
        }
    }
}
