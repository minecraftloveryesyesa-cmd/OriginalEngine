#!/usr/bin/env sh
set -eu

# Requires the SDL_shadercross command-line tool on PATH.
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
SOURCE_DIR="$ROOT/Source"
OUTPUT_DIR="$ROOT/Compiled"

mkdir -p "$OUTPUT_DIR/SPIRV" "$OUTPUT_DIR/MSL" "$OUTPUT_DIR/DXIL"

for source in "$SOURCE_DIR"/*.vert.hlsl "$SOURCE_DIR"/*.frag.hlsl; do
    [ -f "$source" ] || continue
    name="$(basename "$source" .hlsl)"
    shadercross "$source" -o "$OUTPUT_DIR/SPIRV/$name.spv"
    shadercross "$source" -o "$OUTPUT_DIR/MSL/$name.msl"
    shadercross "$source" -o "$OUTPUT_DIR/DXIL/$name.dxil"
done

printf '%s\n' "Shader compilation completed."
