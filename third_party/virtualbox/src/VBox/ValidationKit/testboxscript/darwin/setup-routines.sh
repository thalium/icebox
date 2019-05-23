# $Id: setup-routines.sh $
## @file
# VirtualBox Validation Kit - TestBoxScript Service Setup on Mac OS X (darwin).
#

#
# Copyright (C) 2006-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#
# The contents of this file may alternatively be used under the terms
# of the Common Development and Distribution License Version 1.0
# (CDDL) only, as it comes in the "COPYING.CDDL" file of the
# VirtualBox OSE distribution, in which case the provisions of the
# CDDL are applicable instead of those of the GPL.
#
# You may elect to license modified versions of this file under the
# terms and conditions of either the GPL or the CDDL or both.
#

MY_CONFIG_FILE=/Library/LaunchDaemons/org.virtualbox.testboxscript.plist

##
# Loads config values from the current installation.
#
os_load_config() {
    if [ -r "${MY_CONFIG_FILE}" ]; then
        # User.
        MY_TMP=`/usr/bin/tr '\n' ' ' < "${MY_CONFIG_FILE}" \
                | /usr/bin/sed \
                  -e 's/  */ /g' \
                  -e 's|\(</[[:alnum:]]*>\)<|\1 <|g' \
                  -e 's|^.*<key>UserName</key> *<string>\([^<>]*\)</string>.*$|\1|'`;
        if [ -n "${MY_TMP}" ]; then
            TESTBOXSCRIPT_USER="${MY_TMP}";
        fi

        # Arguments.
        XMLARGS=`/usr/bin/tr '\n' ' ' < "${MY_CONFIG_FILE}" \
                | /usr/bin/sed \
                  -e 's/  */ /g' \
                  -e 's|\(</[[:alnum:]]*>\)<|\1 <|g' \
                  -e 's|^.*ProgramArguments</key> *<array> *\(.*\)</array>.*$|\1|'`;
        eval common_testboxscript_args_to_config `echo "${XMLARGS}" | sed -e "s/<string>/'/g" -e "s/<\/string>/'/g" `;
    fi
}

##
# Adds an argument ($1) to MY_ARGV (XML plist format).
#
os_add_args() {
    while [ $# -gt 0 ];
    do
        case "$1" in
            *\<* | *\>* | *\&*)
                MY_TMP='`echo "$1" | sed -e 's/&/&amp;/g' -e 's/</&lt;/g' -e 's/>/&gt;/g'`';
                MY_ARGV="${MY_ARGV}  <string>${MY_TMP}</string>";
                ;;
            *)
                MY_ARGV="${MY_ARGV}  <string>$1</string>";
                ;;
        esac
        shift;
    done
    MY_ARGV="${MY_ARGV}"'
  ';
    return 0;
}

os_install_service() {
    # Calc the command line.
    MY_ARGV=""
    common_compile_testboxscript_command_line


    # Note! It's not possible to use screen 4.0.3 with the launchd due to buggy
    #       "setsid off" handling (and possible other things).
    cat > "${MY_CONFIG_FILE}" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key>            <string>org.virtualbox.testboxscript</string>
  <key>UserName</key>         <string>${TESTBOXSCRIPT_USER}</string>
  <key>WorkingDirectory</key> <string>${TESTBOXSCRIPT_DIR}</string>
  <key>Enabled</key>          <true/>
  <key>RunAtLoad</key>        <true/>
  <key>KeepAlive</key>        <true/>
  <key>StandardInPath</key>   <string>/dev/null</string>
  <key>StandardOutPath</key>  <string>/dev/null</string>
  <key>StandardErrorPath</key> <string>/dev/null</string>
  <key>ProgramArguments</key>
  <array>
  ${MY_ARGV}</array>
</dict>
</plist>
EOF

    return 0;
}

os_enable_service() {
    launchctl load -w "${MY_CONFIG_FILE}"
    return 0;
}

os_disable_service() {
    if [ -r "${MY_CONFIG_FILE}" ]; then
        launchctl unload "${MY_CONFIG_FILE}"
    fi
    return 0;
}

os_add_user() {
    NEWUID=$(expr `dscl . -readall /Users UniqueID | sed -ne 's/UniqueID: *\([0123456789]*\) *$/\1/p' | sort -n | tail -1 ` + 1)
    if [ -z "$NEWUID" -o "${NEWUID}" -lt 502 ]; then
        NEWUID=502;
    fi

    dscl . -create "/Users/${TESTBOXSCRIPT_USER}" UserShell         /bin/bash
    dscl . -create "/Users/${TESTBOXSCRIPT_USER}" RealName          "VBox Test User"
    dscl . -create "/Users/${TESTBOXSCRIPT_USER}" UniqueID          ${NEWUID}
    dscl . -create "/Users/${TESTBOXSCRIPT_USER}" PrimaryGroupID    80
    dscl . -create "/Users/${TESTBOXSCRIPT_USER}" NFSHomeDirectory  "/Users/vbox"
    dscl . -passwd "/Users/${TESTBOXSCRIPT_USER}" "password"
    mkdir -p "/Users/${TESTBOXSCRIPT_USER}"
}

os_final_message() {
    cat <<EOF

Additional things to do:"
    1. Change the 'Energy Saver' options to never turn off the computer:
       $ systemsetup -setcomputersleep Never -setdisplaysleep 5 -setharddisksleep 15
    2. Check 'Restart automatically if the computer freezes' if available in
       the 'Energy Saver' settings.
       $ systemsetup -setrestartfreeze on
    3. In the 'Sharing' panel enable (VBox/Oracle):
         a) 'Remote Login' so ssh works.
             $ systemsetup -setremotelogin on
         b) 'Remote Management, tick all the checkboxes in the sheet dialog.
            Open the 'Computer Settings' and check 'Show Remote Management
            status in menu bar', 'Anyone may request permission to control
            screen' and 'VNC viewers may control screen with password'. Set the
            VNC password to 'password'.
    4. Make sure the proxy is configured correctly for your network by going to
       the 'Network' panel, open 'Advanced...'. For Oracle this means 'TCP/IP'
       should be configured by 'DHCP' (IPv4) and 'automatically' (IPv6), and
       the 'Proxies' tab should have 'Automatic Proxy Configuration' checked
       with the URL containing 'http://wpad.oracle.com/wpad.dat'. (Make sure
       to hit OK to close the dialog.)
    5. Configure NTP to the nearest local time source. For VBox/Oracle this
       means wei01-time.de.oracle.com:
       $ systemsetup -setnetworktimeserver wei01-time.de.oracle.com
    6. Configure the vbox (pw:password) account for automatic login.
    7. Configure the kernel to keep symbols:
       $ sudo nvram boot-args="keepsyms=1"

Enjoy!
EOF
}

