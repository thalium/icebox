/* $Id: TestVBoxNATEngine.java $ */

/* Small sample/testcase which demonstrates that the same source code can
 * be used to connect to the webservice and (XP)COM APIs. */

/*
 * Copyright (C) 2013-2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
import org.virtualbox_5_0.*;
import java.util.List;
import java.util.Arrays;
import java.util.Iterator;
import java.math.BigInteger;

public class TestVBoxNATEngine
{
    void testEnumeration(VirtualBoxManager mgr, IVirtualBox vbox, IMachine vm)
    {
        String name;
        boolean inaccessible = false;
        /* different chipsets might have different number of attachments */
        ChipsetType chipsetType = vm.getChipsetType();
        INetworkAdapter adapters[] = 
            new INetworkAdapter[
                    vbox.getSystemProperties().getMaxNetworkAdapters(chipsetType).intValue()];

        try
        {
            name = vm.getName();
            /*
             * Dump adapters and if it's got NAT attachment
             * dump it settings
             */
                
            for (int nidx = 0; nidx < adapters.length; ++nidx)
            {
                /* select available and NATs only. */
                adapters[nidx] = vm.getNetworkAdapter(new Long(nidx));
                INetworkAdapter n = adapters[nidx];
                    
                if (n == null)
                    continue;
                NetworkAttachmentType attachmentType = n.getAttachmentType();
                if (attachmentType.equals(NetworkAttachmentType.NAT))
                {
                    INATEngine nat = n.getNATEngine();
                    List<String> portForward = nat.getRedirects();
                    String pf = null;
                    Iterator<String> itPortForward = portForward.iterator();
                    for (;itPortForward.hasNext();)
                    {
                        pf = itPortForward.next();
                        System.out.println(name + ":NIC" + n.getSlot() /* name:NIC<slot number>*/
                                           + " pf: " + pf); /* port-forward rule */
                    }
                    if (pf != null)
                    {
                        String pfAttributes[] = pf.split(",");
                        /* name,proto,hostip,host,hostport,guestip,guestport */
                        nat.removeRedirect(pfAttributes[0]);
                        nat.addRedirect("", 
                                        NATProtocol.fromValue(new Integer(pfAttributes[1]).longValue()),
                                        pfAttributes[2],
                                        new Integer(
                                                new Integer(pfAttributes[3]).intValue() + 1),
                                        pfAttributes[4],
                                        new Integer(pfAttributes[5]));
                    }

                }
            }
                
        }
        catch (VBoxException e)
        {
            name = "<inaccessible>";
            inaccessible = true;
        }            

        // process system event queue
        mgr.waitForEvents(0);
    }

    static void testStart(VirtualBoxManager mgr, IVirtualBox vbox, IMachine vm)
    {
        System.out.println("\nAttempting to start VM '" + vm.getName() + "'");
        mgr.startVm(vm.getName(), null, 7000);
        // process system event queue
        mgr.waitForEvents(0);
    }

    public TestVBoxNATEngine(String[] args)
    {
        VirtualBoxManager mgr = VirtualBoxManager.createInstance(null);

        boolean ws = false;
        String  url = null;
        String  user = null;
        String  passwd = null;
        String  vmname = null;
        IMachine vm = null;

        for (int i = 0; i<args.length; i++)
            {
                if ("-w".equals(args[i]))
                    ws = true;
                else if ("-url".equals(args[i]))
                    url = args[++i];
                else if ("-user".equals(args[i]))
                    user = args[++i];
                else if ("-passwd".equals(args[i]))
                    passwd = args[++i];
                else if ("-vm".equals(args[i]))
                    vmname = args[++i];
            }

        if (ws)
            {
                try {
                    mgr.connect(url, user, passwd);
                } catch (VBoxException e) {
                    e.printStackTrace();
                    System.out.println("Cannot connect, start webserver first!");
                }
            }

        try
            {
                IVirtualBox vbox = mgr.getVBox();
                if (vbox != null)
                {
                    if (vmname != null)
                    {
                        for (IMachine m:vbox.getMachines())
                        {
                            if (m.getName().equals(vmname))
                            {
                                vm = m;
                                break;
                            }
                        }

                    }
                    else
                        vm = vbox.getMachines().get(0);
                    System.out.println("VirtualBox version: " + vbox.getVersion() + "\n");
                    if (vm != null)
                    {
                        testEnumeration(mgr, vbox, vm);
                        testStart(mgr, vbox, vm);
                    }
                    System.out.println("done, press Enter...");
                    int ch = System.in.read();
                }
            }
        catch (VBoxException e)
            {
                System.out.println("VBox error: "+e.getMessage()+" original="+e.getWrapped());
                e.printStackTrace();
            }
        catch (java.io.IOException e)
            {
                e.printStackTrace();
            }

        // process system event queue
        mgr.waitForEvents(0);

        if (ws)
            {
                try {
                    mgr.disconnect();
                } catch (VBoxException e) {
                    e.printStackTrace();
                }
            }
        /* cleanup do the disconnect */
        mgr.cleanup();

    }
    public static void main(String[] args)
    {
        new TestVBoxNATEngine(args);
    }

}
