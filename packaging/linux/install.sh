#!/bin/sh
# Install Nabra on Linux (standalone app, desktop entry, icon, VST3 and LV2 plug-ins).
#   ./install.sh                 install for the current user into ~/.local (no sudo needed)
#   sudo ./install.sh --system   install for all users into /usr/local
#   ./install.sh --uninstall     remove a per-user install (add --system for /usr/local)
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
MODE=user; ACTION=install
for a in "$@"; do
  case "$a" in
    --system) MODE=system ;;
    --uninstall) ACTION=uninstall ;;
    -h|--help) sed -n '2,5p' "$0"; exit 0 ;;
    *) echo "unknown option: $a" >&2; exit 2 ;;
  esac
done
if [ "$MODE" = system ]; then
  PREFIX=/usr/local; VST3=/usr/local/lib/vst3; LV2=/usr/local/lib/lv2
else
  PREFIX="$HOME/.local"; VST3="$HOME/.vst3"; LV2="$HOME/.lv2"
fi
APPS="$PREFIX/share/applications"; ICONS="$PREFIX/share/icons/hicolor"

if [ "$ACTION" = uninstall ]; then
  rm -rf "$PREFIX/bin/Nabra" "$PREFIX/bin/nabra" "$VST3/Nabra.vst3" "$LV2/Nabra.lv2" "$APPS/nabra.desktop"
  for s in 256 512; do rm -f "$ICONS/${s}x${s}/apps/nabra.png"; done
  echo "Nabra removed from $PREFIX"; exit 0
fi

mkdir -p "$PREFIX/bin" "$VST3" "$LV2" "$APPS"
install -m 755 "$HERE/bin/Nabra" "$PREFIX/bin/Nabra"
ln -sf Nabra "$PREFIX/bin/nabra"
rm -rf "$VST3/Nabra.vst3" "$LV2/Nabra.lv2"
cp -R "$HERE/lib/vst3/Nabra.vst3" "$VST3/"
cp -R "$HERE/lib/lv2/Nabra.lv2" "$LV2/"
for s in 256 512; do
  mkdir -p "$ICONS/${s}x${s}/apps"
  install -m 644 "$HERE/share/icons/hicolor/${s}x${s}/apps/nabra.png" "$ICONS/${s}x${s}/apps/nabra.png"
done
sed "s|@NABRA_EXEC@|$PREFIX/bin/Nabra|" "$HERE/share/applications/nabra.desktop" > "$APPS/nabra.desktop"
command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$APPS" 2>/dev/null || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -q "$ICONS" 2>/dev/null || true
echo "Nabra installed:"
echo "  app      $PREFIX/bin/Nabra (also in your applications menu)"
echo "  VST3     $VST3/Nabra.vst3"
echo "  LV2      $LV2/Nabra.lv2"
