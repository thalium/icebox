IMG_VERS=4
docker build -t thelittlefireman/ubuntu_build_vbox:$IMG_VERS .
docker push thelittlefireman/ubuntu_build_vbox:$IMG_VERS
