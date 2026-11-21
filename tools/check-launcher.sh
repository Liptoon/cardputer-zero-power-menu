#!/usr/bin/env bash
# Diagnostics to run ON the CardputerZero. Read-only, no root needed.
# Shows whether the package, launcher entry, icon and binary are in place,
# and how APPLaunch is actually started on this device.

echo "== package =="
dpkg -s power-menu 2>/dev/null | grep -E '^(Package|Status|Version|Architecture):' || echo "power-menu not installed"

echo
echo "== launcher files =="
ls -l /usr/share/APPLaunch/applications/power-menu.desktop \
      /usr/share/APPLaunch/bin/power-menu \
      /usr/share/APPLaunch/share/images/power-menu.png 2>&1

echo
echo "== desktop entry =="
cat /usr/share/APPLaunch/applications/power-menu.desktop 2>/dev/null

echo
echo "== processes =="
ps aux | grep -i applaunch | grep -v grep || echo "no APPLaunch process found"

echo
echo "== systemd units =="
systemctl list-units --all --no-pager 2>/dev/null | grep -i applaunch || echo "no running APPLaunch unit"
systemctl list-unit-files --no-pager 2>/dev/null | grep -i applaunch || echo "no APPLaunch unit file"
