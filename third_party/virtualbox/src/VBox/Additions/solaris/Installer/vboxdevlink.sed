# sed class script to modify /etc/devlink.tab
!install
/name=vboxguest/d
$i\
type=ddi_pseudo;name=vboxguest  \\D

!remove
/name=vboxguest/d

