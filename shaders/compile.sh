#!/usr/bin/env sh
set -eu

# Requires the SDL_shadercross CLI on PATH.
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
SOURCE_DIR="$ROOT/Source"
OUTPUT_DIR="$ROOT/Compiled"

if ! command -v shadercross >/dev/null 2>&1; then
    printf '%s\n' "error: SDL_shadercross (shadercross) was not found on PATH." >&2
    exit 127
fi

mkdir -p "$OUTPUT_DIR/SPIRV" "$OUTPUT_DIR/MSL" "$OUTPUT_DIR/DXIL"

found_source=0
for source in "$SOURCE_DIR"/*.vert.hlsl "$SOURCE_DIR"/*.frag.hlsl; do
    [ -f "$source" ] || continue
    found_source=1
    name="$(basename "$source" .hlsl)"

    printf 'Compiling %s\n' "$name"
    shadercross "$source" -o "$OUTPUT_DIR/SPIRV/$name.spv"
    shadercross "$source" -o "$OUTPUT_DIR/MSL/$name.msl"
    shadercross "$source" -o "$OUTPUT_DIR/DXIL/$name.dxil"

    for output in \
        "$OUTPUT_DIR/SPIRV/$name.spv" \
        "$OUTPUT_DIR/MSL/$name.msl" \
        "$OUTPUT_DIR/DXIL/$name.dxil"; do
        if [ ! -s "$output" ]; then
            printf 'error: shadercross did not create a non-empty output: %s\n' "$output" >&2
            exit 1
        fi
    done
done

if [ "$found_source" -eq 0 ]; then
    printf 'error: no .vert.hlsl or .frag.hlsl files found in %s\n' "$SOURCE_DIR" >&2
    exit 1
fi

printf 'Shader compilation completed. Outputs: %s\n' "$OUTPUT_DIR"
