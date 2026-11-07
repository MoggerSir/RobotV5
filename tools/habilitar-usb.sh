#!/bin/sh
# Este archivo se ejecuta en Linux; no se carga en el Brain.
# Detiene el script si una orden falla o se usa una variable sin definir.
set -eu
# id -u devuelve el numero de usuario. Solo root (numero 0) puede cambiar reglas del sistema.
if [ "$(id -u)" -ne 0 ]; then
  echo 'Ejecuta: sudo sh tools/habilitar-usb.sh'
  exit 1
fi
# Da acceso USB solo a dispositivos VEX a la sesion local activa.
# Guarda dos reglas: una para el puerto serie y otra para el dispositivo USB VEX.
# 2888 identifica al fabricante VEX. uaccess concede acceso a la sesion local activa.
# El bloque entre RULES y RULES se escribe como texto literal en el archivo indicado.
cat > /etc/udev/rules.d/70-vex-v5.rules <<'RULES'
SUBSYSTEM=="tty", ATTRS{idVendor}=="2888", TAG+="uaccess"
SUBSYSTEM=="usb", ATTR{idVendor}=="2888", TAG+="uaccess"
RULES
# Deja las reglas legibles para el sistema y modificables solo por su propietario.
chmod 644 /etc/udev/rules.d/70-vex-v5.rules
# Pide a Linux que vuelva a leer las reglas y las aplique a los dispositivos conectados.
udevadm control --reload-rules
udevadm trigger --subsystem-match=tty
udevadm trigger --subsystem-match=usb --attr-match=idVendor=2888
echo 'Acceso USB VEX habilitado. Desconecta y vuelve a conectar el Brain o el control.'
