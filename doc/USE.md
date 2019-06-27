___Before use, it should be necessary to supply debug information to Icebox.___<br><br>

## Linux guests

Debug information (or profil) for a specific kernel could be get with the script supplied in doc/make_profil_linux/make_profil.zip
<br>
All instructions are given in doc/make_profil_linux/make_profil.md.
<br><br>
Once the profil created, decompress it in a folder on the host and create an environment variable nammed _LINUX_SYMBOL_PATH pointing to it.<br><br>
Finaly, the required files are :
- _LINUX_SYMBOL_PATH/kernel/sha1_hash_of_guest_banner_kernel/elf
- _LINUX_SYMBOL_PATH/kernel/sha1_hash_of_guest_banner_kernel/System.map