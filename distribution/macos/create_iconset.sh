#!/bin/bash
# requires: Inkscape

mkdir -p "flare.iconset"
cd "flare.iconset"

sizes=(1024 512 256 128 64 32)

for size in ${sizes[@]}; do
	/Applications/Inkscape.app/Contents/MacOS/inkscape -w ${size} -h ${size} -o "icon_${size}x${size}.png" "../../flare_logo_icon.svg"
	let half=size/2
	cp "icon_${size}x${size}.png" "icon_${half}x${half}@2x.png"
done

cd ../
iconutil -c icns "flare.iconset"
rm -rf "flare.iconset"
