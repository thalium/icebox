# Utilisation

___Debian example :___

~~~~
sudo apt-get update
sudo apt-get install -y make gcc zip unzip

wget https://path/to/make_profil.zip
unzip make_profil.zip
./make_profil.sh
~~~~

The profil will be created in profil.zip.<br>
It has to be decompressed in your folder pointed by _PATH _LINUX_SYMBOL_PATH.<br>
Finaly, the required files are :
- _LINUX_SYMBOL_PATH/kernel/sha1_hash_of_guest_banner_kernel/elf (the binary of module.c)
- _LINUX_SYMBOL_PATH/kernel/sha1_hash_of_guest_banner_kernel/System.map (found in /boot)

<br><br>
# Alternatives
- includes can be added to module.c if needed
- for more debug information replace in Makefile _CONFIG_DEBUG_INFO=y_ with _KBUILD_CFLAGS="-g3 -fno-eliminate-unused-debug-types"_
- the module.c of Volatility foundation project could be use instead of present module.c.