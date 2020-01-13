package my.syahmi.internetauto;

import android.util.Log;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;

public class ExecuteAsRootBase {
    private static final String TAG = "Root";
    private ArrayList<String> mCommands = new ArrayList<>();

    public void addCommand(String cmd) {
        mCommands.add(cmd);
    }

    public void clear() {
        mCommands.clear();
    }

    public static boolean canRunRootCommands()
    {
        boolean retval = false;
        Process suProcess;

        try
        {
            suProcess = Runtime.getRuntime().exec("su");

            DataOutputStream os = new DataOutputStream(suProcess.getOutputStream());
            DataInputStream osRes = new DataInputStream(suProcess.getInputStream());

            // Getting the id of the current user to check if this is root
            os.writeBytes("id\n");
            os.flush();

            String currUid = osRes.readLine();
            boolean exitSu = false;
            if (null == currUid)
            {
                retval = false;
                exitSu = false;
                Log.d(TAG, "Can't get root access or denied by user");
            }
            else if (true == currUid.contains("uid=0"))
            {
                retval = true;
                exitSu = true;
                Log.d(TAG, "Root access granted");
            }
            else
            {
                retval = false;
                exitSu = true;
                Log.d(TAG, "Root access rejected: " + currUid);
            }

            if (exitSu)
            {
                os.writeBytes("exit\n");
                os.flush();
            }
        }
        catch (Exception e)
        {
            // Can't get root !
            // Probably broken pipe exception on trying to write to output stream (os) after su failed, meaning that the device is not rooted

            retval = false;
            Log.d("ROOT", "Root access rejected [" + e.getClass().getName() + "] : " + e.getMessage());
        }

        return retval;
    }

    public final boolean execute()
    {
        boolean retval = false;

        try
        {
            if (mCommands.size() > 0)
            {
                Process suProcess = Runtime.getRuntime().exec("su");

                DataOutputStream os = new DataOutputStream(suProcess.getOutputStream());
                //BufferedReader in = new BufferedReader(new InputStreamReader(suProcess.getInputStream()));

                String result = "";
                // result = in.readLine();

                // Execute commands that require root access
                for (String currCommand : mCommands)
                {
                    Log.d(TAG, "Running " + currCommand);
                    os.writeBytes(currCommand + "\n");
                    os.flush();

                    //result = in.readLine();
                    //Log.d(TAG, result);
                }

                os.writeBytes("exit\n");
                os.flush();

                try
                {
                    int suProcessRetval = suProcess.waitFor();
                    if (255 != suProcessRetval)
                    {
                        // Root access granted
                        retval = true;
                    }
                    else
                    {
                        // Root access denied
                        retval = false;
                    }
                }
                catch (Exception ex)
                {
                    Log.e(TAG, "Error executing root action", ex);
                }
            }
        }
        catch (IOException ex)
        {
            Log.w(TAG, "Can't get root access", ex);
        }
        catch (SecurityException ex)
        {
            Log.w(TAG, "Can't get root access", ex);
        }
        catch (Exception ex)
        {
            Log.w(TAG, "Error executing internal operation", ex);
        }

        return retval;
    }
}