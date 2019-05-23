/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

/*
 * Oracle GPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the General Public License version 2 (GPLv2) at this time for any software where
 * a choice of GPL license versions is made available with the language indicating
 * that GPLv2 or any later version may be used, or where a choice of which version
 * of the GPL is applied is otherwise unspecified.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <config/general.h>

/** @file
 *
 * 802.11 configuration options
 *
 */

/*
 * Drag in 802.11-specific commands
 *
 */
#ifdef IWMGMT_CMD
REQUIRE_OBJECT ( iwmgmt_cmd );
#endif

/*
 * Drag in 802.11 error message tables
 *
 */
#ifdef ERRMSG_80211
REQUIRE_OBJECT ( wireless_errors );
#endif

/*
 * Drag in 802.11 cryptosystems and handshaking protocols
 *
 */
#ifdef CRYPTO_80211_WEP
REQUIRE_OBJECT ( wep );
#endif

#ifdef CRYPTO_80211_WPA2
#define CRYPTO_80211_WPA
REQUIRE_OBJECT ( wpa_ccmp );
#endif

#ifdef CRYPTO_80211_WPA
REQUIRE_OBJECT ( wpa_psk );
REQUIRE_OBJECT ( wpa_tkip );
#endif
