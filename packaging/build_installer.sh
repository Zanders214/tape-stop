#!/usr/bin/env bash
#
# Build a macOS .pkg installer for Tape Stop.
#
# The installer places each plug-in format in its standard system location so
# hosts find it automatically (no manual copying):
#
#   VST3       -> /Library/Audio/Plug-Ins/VST3
#   Audio Unit -> /Library/Audio/Plug-Ins/Components
#   Standalone -> /Applications
#
# It is a distribution installer with a "Customize" screen, so users can
# deselect formats. All components are non-relocatable, so they always land in
# the exact folder above rather than tracking down a pre-existing copy.
#
# Prerequisites: a completed Release build (see README "Building"), e.g.
#   cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
#   cmake --build build --config Release
#
# Usage:
#   packaging/build_installer.sh
#
# Environment overrides:
#   BUILD_DIR      build tree                (default: <repo>/build)
#   VERSION        installer version         (default: parsed from CMakeLists.txt)
#   OUTPUT         output .pkg path          (default: <BUILD_DIR>/installer/TapeStop-macOS-v<VERSION>-Installer.pkg)
#   SIGN_IDENTITY  "Developer ID Installer: Name (TEAMID)" to sign the .pkg
#                  (leave unset for an unsigned build)
#
set -euo pipefail

# --- Resolve paths -----------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
REL="$BUILD_DIR/TapeStop_artefacts/Release"

# --- Config ------------------------------------------------------------------
PRODUCT="Tape Stop"
PKG_BASE="com.zandersaudio.tapestop"

if [[ -z "${VERSION:-}" ]]; then
  VERSION="$(grep -Eo 'project\(TapeStop VERSION [0-9]+\.[0-9]+\.[0-9]+' \
    "$REPO_ROOT/CMakeLists.txt" | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
fi
[[ -n "$VERSION" ]] || { echo "error: could not determine VERSION" >&2; exit 1; }

OUTPUT="${OUTPUT:-$BUILD_DIR/installer/TapeStop-macOS-v${VERSION}-Installer.pkg}"

VST3="$REL/VST3/$PRODUCT.vst3"
AU="$REL/AU/$PRODUCT.component"
APP="$REL/Standalone/$PRODUCT.app"

for b in "$VST3" "$AU" "$APP"; do
  if [[ ! -d "$b" ]]; then
    echo "error: missing build artifact: $b" >&2
    echo "       build the Release config first (see README > Building)." >&2
    exit 1
  fi
done

# --- Stage each format under a root that mirrors its install destination -----
WORK="$BUILD_DIR/installer/work"
COMP="$WORK/components"
ROOTS="$WORK/roots"
rm -rf "$WORK" "$OUTPUT"
mkdir -p "$COMP" "$ROOTS/vst3" "$ROOTS/au" "$ROOTS/app" "$(dirname "$OUTPUT")"

ditto "$VST3" "$ROOTS/vst3/$PRODUCT.vst3"
ditto "$AU"   "$ROOTS/au/$PRODUCT.component"
ditto "$APP"  "$ROOTS/app/$PRODUCT.app"

# --- Force non-relocatable installs (always land in the exact folder) --------
mk_plist () {  # $1 = staging root, $2 = output component plist
  pkgbuild --analyze --root "$1" "$2" >/dev/null
  local n i=0
  n=$(/usr/libexec/PlistBuddy -c "Print" "$2" | grep -c "BundleIsRelocatable" || true)
  while [[ "$i" -lt "$n" ]]; do
    /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$2" 2>/dev/null || true
    i=$((i + 1))
  done
}
mk_plist "$ROOTS/vst3" "$WORK/vst3.plist"
mk_plist "$ROOTS/au"   "$WORK/au.plist"
mk_plist "$ROOTS/app"  "$WORK/app.plist"

# --- Build the three component packages --------------------------------------
pkgbuild --root "$ROOTS/vst3" --component-plist "$WORK/vst3.plist" \
  --identifier "$PKG_BASE.vst3" --version "$VERSION" \
  --install-location "/Library/Audio/Plug-Ins/VST3" "$COMP/TapeStop-VST3.pkg"

pkgbuild --root "$ROOTS/au" --component-plist "$WORK/au.plist" \
  --identifier "$PKG_BASE.au" --version "$VERSION" \
  --install-location "/Library/Audio/Plug-Ins/Components" "$COMP/TapeStop-AU.pkg"

pkgbuild --root "$ROOTS/app" --component-plist "$WORK/app.plist" \
  --identifier "$PKG_BASE.standalone" --version "$VERSION" \
  --install-location "/Applications" "$COMP/TapeStop-Standalone.pkg"

# --- Distribution definition (title + per-format Customize choices) ----------
cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>$PRODUCT $VERSION</title>
    <organization>com.zandersaudio</organization>
    <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <welcome mime-type="text/plain">$PRODUCT $VERSION installs the plug-ins in your system Audio Plug-Ins folders so your DAW finds them automatically. Click Customize to choose formats.</welcome>
    <choices-outline>
        <line choice="vst3"/>
        <line choice="au"/>
        <line choice="standalone"/>
    </choices-outline>
    <choice id="vst3" title="VST3 Plug-In" description="Installs to /Library/Audio/Plug-Ins/VST3">
        <pkg-ref id="$PKG_BASE.vst3"/>
    </choice>
    <choice id="au" title="Audio Unit (AU)" description="Installs to /Library/Audio/Plug-Ins/Components">
        <pkg-ref id="$PKG_BASE.au"/>
    </choice>
    <choice id="standalone" title="Standalone App" description="Installs to /Applications">
        <pkg-ref id="$PKG_BASE.standalone"/>
    </choice>
    <pkg-ref id="$PKG_BASE.vst3" version="$VERSION">TapeStop-VST3.pkg</pkg-ref>
    <pkg-ref id="$PKG_BASE.au" version="$VERSION">TapeStop-AU.pkg</pkg-ref>
    <pkg-ref id="$PKG_BASE.standalone" version="$VERSION">TapeStop-Standalone.pkg</pkg-ref>
</installer-gui-script>
XML

# --- Combine into the final product installer --------------------------------
# (Two branches rather than an array so an unset SIGN_IDENTITY stays clean under
# `set -u` on macOS's bash 3.2, where empty-array expansion is "unbound".)
if [[ -n "${SIGN_IDENTITY:-}" ]]; then
  productbuild --distribution "$WORK/distribution.xml" --package-path "$COMP" \
    --sign "$SIGN_IDENTITY" "$OUTPUT"
else
  productbuild --distribution "$WORK/distribution.xml" --package-path "$COMP" \
    "$OUTPUT"
fi

echo "Built installer: $OUTPUT"
