#!/bin/sh
set -eu
if [ "$(id -u)" -ne 0 ]; then
  echo 'Ejecuta: sudo sh tools/habilitar-usb.sh'
  exit 1
fi
cat > /etc/udev/rules.d/70-vex-v5.rules <<'RULES'
SUBSYSTEM=="tty", ATTRS{idVendor}=="2888", TAG+="uaccess"
SUBSYSTEM=="usb", ATTR{idVendor}=="2888", TAG+="uaccess"
RULES
chmod 644 /etc/udev/rules.d/70-vex-v5.rules
udevadm control --reload-rules
udevadm trigger --subsystem-match=tty
udevadm trigger --subsystem-match=usb --attr-match=idVendor=2888
echo 'Acceso USB VEX habilitado. Desconecta y vuelve a conectar el Brain o el control.'
