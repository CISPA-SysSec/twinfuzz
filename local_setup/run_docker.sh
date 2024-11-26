sudo docker build -t twinframe-linux-intel .
sudo docker run --rm --device=/dev/dri -e LIBVA_DRIVER_NAME="iHD" -it twinframe-linux-intel /bin/bash
