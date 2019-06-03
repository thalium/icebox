# Utilisation

___Debian example :___

~~~~
sudo apt-get update
sudo apt-get install -y make gcc zip unzip

wget https://path/to/make_profil.zip
unzip make_profil.zip
./make_profil.sh
~~~~

The profil will be created in profil.zip
<br><br>
# Alternatives
- includes can be added to module.c if needed
- for more debug information replace in Makefile _CONFIG_DEBUG_INFO=y_ with _KBUILD_CFLAGS="-g3 -fno-eliminate-unused-debug-types"_
- the module.c of Volatility foundation project could be use instead of present module.c.