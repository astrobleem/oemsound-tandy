#!/usr/bin/env python3
"""Convert a local video into compact IBM/Tandy RGBI animation assets.

Requires ffmpeg, ffprobe, Python 3, and Pillow. No network access is performed.
WZV1: magic, little-endian u16 width/height/fps/frame_count, then packed frames.
Each packed row is left-to-right, high nibble first, with no row padding.
Frames are top-to-bottom. Odd-width rows have a zero low nibble at the end.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

from PIL import Image, ImageDraw


RGBI = [
    (0x00, 0x00, 0x00), (0x00, 0x00, 0xAA),
    (0x00, 0xAA, 0x00), (0x00, 0xAA, 0xAA),
    (0xAA, 0x00, 0x00), (0xAA, 0x00, 0xAA),
    (0xAA, 0x55, 0x00), (0xAA, 0xAA, 0xAA),
    (0x55, 0x55, 0x55), (0x55, 0x55, 0xFF),
    (0x55, 0xFF, 0x55), (0x55, 0xFF, 0xFF),
    (0xFF, 0x55, 0x55), (0xFF, 0x55, 0xFF),
    (0xFF, 0xFF, 0x55), (0xFF, 0xFF, 0xFF),
]
NAMES = ["black", "blue", "green", "cyan", "red", "magenta", "brown",
         "light gray", "dark gray", "light blue", "light green", "light cyan",
         "light red", "light magenta", "yellow", "white"]
PALETTE_BYTES = [value for color in RGBI for value in color]


def indexed_rgbi(image: Image.Image, dither: str) -> Image.Image:
    palette = Image.new("P", (16, 1))
    # Fill otherwise-unused palette slots with black. Remap any duplicate black
    # indices, so every output pixel is provably in 0..15.
    palette.putpalette(PALETTE_BYTES + [0] * (768 - len(PALETTE_BYTES)))
    mode = Image.Dither.FLOYDSTEINBERG if dither == "fs" else Image.Dither.NONE
    result = image.convert("RGB").quantize(palette=palette, dither=mode)
    result = result.point(list(range(16)) + [0] * 240)
    result.putpalette(PALETTE_BYTES + [0] * (768 - len(PALETTE_BYTES)))
    return result


def pack_frame(image: Image.Image) -> bytes:
    width, height = image.size
    source = image.tobytes()
    result = bytearray()
    for y in range(height):
        row = source[y * width:(y + 1) * width]
        for x in range(0, width, 2):
            high = row[x]
            low = row[x + 1] if x + 1 < width else 0
            if high > 15 or low > 15:
                raise ValueError("Palette index outside RGBI range")
            result.append((high << 4) | low)
    return bytes(result)


def unpack_frame(frame: bytes, width: int, height: int) -> Image.Image:
    row_bytes = (width + 1) // 2
    if len(frame) != row_bytes * height:
        raise ValueError("Incorrect packed frame size")
    result = bytearray()
    for y in range(height):
        row = bytearray()
        for value in frame[y * row_bytes:(y + 1) * row_bytes]:
            row.extend((value >> 4, value & 15))
        result.extend(row[:width])
    image = Image.frombytes("P", (width, height), bytes(result))
    image.putpalette(PALETTE_BYTES + [0] * (768 - len(PALETTE_BYTES)))
    return image


def dib_pixels(frame: bytes, width: int, height: int) -> bytes:
    row_bytes = (width + 1) // 2
    stride = ((width * 4 + 31) // 32) * 4
    return b"".join(frame[y * row_bytes:(y + 1) * row_bytes]
                    + bytes(stride - row_bytes)
                    for y in reversed(range(height)))


def dib_header(width: int, height: int) -> bytes:
    stride = ((width * 4 + 31) // 32) * 4
    # BITMAPINFOHEADER. Positive height means bottom-up. BI_RGB, 4bpp.
    header = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 4, 0,
                         stride * height, 0, 0, 16, 16)
    return header + b"".join(bytes((b, g, r, 0)) for r, g, b in RGBI)


def read_exact(stream, size: int) -> bytes:
    data = bytearray()
    while len(data) < size:
        part = stream.read(size - len(data))
        if not part:
            break
        data.extend(part)
    return bytes(data)


def probe(source: Path) -> dict:
    result = subprocess.run(["ffprobe", "-v", "error", "-show_format",
                             "-show_streams", "-of", "json", str(source)],
                            check=True, capture_output=True, text=True)
    return json.loads(result.stdout)


def file_sha256(source: Path) -> str:
    digest = hashlib.sha256()
    with source.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def filter_string(width: int, height: int, fps: int, fit: str) -> str:
    if fit == "stretch":
        scale = f"scale={width}:{height}:flags=lanczos"
    elif fit == "crop":
        scale = (f"scale={width}:{height}:force_original_aspect_ratio=increase:"
                 f"flags=lanczos,crop={width}:{height}")
    else:
        scale = (f"scale={width}:{height}:force_original_aspect_ratio=decrease:"
                 f"flags=lanczos,pad={width}:{height}:(ow-iw)/2:(oh-ih)/2:black")
    return f"fps={fps},{scale},setsar=1"


def contact_sheet(frames: list[Image.Image], start: float, fps: int,
                  destination: Path) -> None:
    # Show every output frame, enlarged with nearest-neighbor pixels.
    columns = 8
    zoom = max(2, min(4, 256 // frames[0].width))
    tile_w, tile_h = frames[0].width * zoom, frames[0].height * zoom
    margin, caption = 12, 22
    rows = math.ceil(len(frames) / columns)
    sheet = Image.new("RGB", (columns * (tile_w + margin) + margin,
                              rows * (tile_h + caption + margin) + margin),
                      (24, 24, 28))
    draw = ImageDraw.Draw(sheet)
    for i, frame in enumerate(frames):
        x = margin + (i % columns) * (tile_w + margin)
        y = margin + (i // columns) * (tile_h + caption + margin)
        sheet.paste(frame.convert("RGB").resize((tile_w, tile_h),
                    Image.Resampling.NEAREST), (x, y))
        draw.text((x, y + tile_h + 4),
                  f"{i:02d} | source {start + i / fps:.2f}s", fill="white")
    sheet.save(destination)


def convert(args: argparse.Namespace) -> dict:
    source = Path(args.input).resolve()
    if not source.is_file():
        raise FileNotFoundError(f"Local input video not found: {source}")
    if any(not (1 <= n <= 65535) for n in (args.width, args.height, args.fps)):
        raise ValueError("Width, height, and fps must fit nonzero unsigned 16-bit")
    if args.start < 0 or args.duration <= 0:
        raise ValueError("Start must be >= 0, and duration must be > 0")
    if args.duration * args.fps > 65535:
        raise ValueError("Too many frames for WZV1")
    out = Path(args.output_dir).resolve()
    out.mkdir(parents=True, exist_ok=True)
    source_metadata = probe(source)
    command = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-ss",
               str(args.start), "-i", str(source), "-t", str(args.duration),
               "-map", "0:v:0", "-vf",
               filter_string(args.width, args.height, args.fps, args.fit),
               "-an", "-sn", "-dn", "-pix_fmt", "rgb24", "-f", "rawvideo", "pipe:1"]
    frames = []
    packed_frames = []
    frame_bytes = args.width * args.height * 3
    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    try:
        assert process.stdout is not None
        while True:
            raw = read_exact(process.stdout, frame_bytes)
            if not raw:
                break
            if len(raw) != frame_bytes:
                raise ValueError("ffmpeg returned an incomplete RGB frame")
            rgb = Image.frombytes("RGB", (args.width, args.height), raw)
            frame = indexed_rgbi(rgb, args.dither)
            packed = pack_frame(frame)
            if unpack_frame(packed, args.width, args.height).tobytes() != frame.tobytes():
                raise AssertionError("Packed RGBI frame failed round-trip validation")
            frames.append(frame)
            packed_frames.append(packed)
            if len(frames) > 65535:
                raise ValueError("Too many frames for WZV1")
    except Exception:
        process.kill()
        process.wait()
        raise
    finally:
        if process.stdout is not None:
            process.stdout.close()
    if process.wait() != 0:
        raise RuntimeError("ffmpeg conversion failed; see diagnostic above")
    if not frames:
        raise ValueError("No frames at the requested source time")

    stream_name = args.name + ".wzv"
    payload = struct.pack("<4sHHHH", b"WZV1", args.width, args.height,
                          args.fps, len(frames)) + b"".join(packed_frames)
    (out / stream_name).write_bytes(payload)
    # Headerless bottom-up, DWORD-aligned 4bpp frame pixels for SetDIBitsToDevice.
    (out / (args.name + ".dibframes")).write_bytes(b"".join(
        dib_pixels(frame, args.width, args.height) for frame in packed_frames))
    info = dib_header(args.width, args.height)
    (out / "rgbi_bitmapinfo.bin").write_bytes(info)
    dib_dir = out / "dib_frames"
    dib_dir.mkdir(exist_ok=True)
    for i, packed in enumerate(packed_frames):
        (dib_dir / f"frame_{i:04d}.dib").write_bytes(
            info + dib_pixels(packed, args.width, args.height))
    first_dib = info + dib_pixels(packed_frames[0], args.width, args.height)
    (out / "first_frame.bmp").write_bytes(
        struct.pack("<2sIHHI", b"BM", len(first_dib) + 14, 0, 0, 14 + len(info))
        + first_dib)
    frames[0].resize((args.width * 6, args.height * 6),
                     Image.Resampling.NEAREST).save(out / "first_frame.png")
    contact_sheet(frames, args.start, args.fps, out / "contact_sheet.png")
    preview = [frame.resize((args.width * 4, args.height * 4),
                           Image.Resampling.NEAREST) for frame in frames]
    preview[0].save(out / "preview.gif", save_all=True, append_images=preview[1:],
                    duration=round(1000 / args.fps), loop=0, optimize=False,
                    disposal=2)
    video = next(stream for stream in source_metadata["streams"]
                 if stream["codec_type"] == "video")
    stride = ((args.width * 4 + 31) // 32) * 4
    metadata = {
        "source_filename": source.name,
        "source_sha256": file_sha256(source),
        "source_video": {key: video.get(key) for key in
                         ("codec_name", "width", "height", "avg_frame_rate",
                          "duration", "nb_frames")},
        "source_container_duration_seconds": source_metadata.get("format", {}).get("duration"),
        "start_seconds": args.start, "requested_duration_seconds": args.duration,
        "output_duration_seconds": len(frames) / args.fps,
        "width": args.width, "height": args.height, "fps": args.fps,
        "frame_count": len(frames), "fit": args.fit, "dither": args.dither,
        "stream": stream_name, "stream_bytes": len(payload),
        "stream_sha256": hashlib.sha256(payload).hexdigest(),
        "stream_header_bytes": 12,
        "packed_frame_bytes": len(packed_frames[0]),
        "packed_rows": "top-down, high nibble is left pixel, no row padding",
        "dib_stride_bytes": stride, "dib_frame_bytes": stride * args.height,
        "dib_rows": "bottom-up, DWORD row alignment, high nibble is left pixel",
        "palette_rgb": RGBI, "palette_names": NAMES,
        "audio": "No audio is included. This is an independently timed visual clip.",
        "synchronization": args.note,
        "ffmpeg_command": command,
        "privacy": "Converted locally from supplied source; see ATTRIB.TXT.",
    }
    (out / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
    return metadata


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="User-supplied local video path")
    parser.add_argument("--output-dir", default="generated")
    parser.add_argument("--name", default="WEEZER")
    parser.add_argument("--start", type=float, default=0.0)
    parser.add_argument("--duration", type=float, default=8.0)
    parser.add_argument("--width", type=int, default=64)
    parser.add_argument("--height", type=int, default=48)
    parser.add_argument("--fps", type=int, default=4)
    parser.add_argument("--fit", choices=("contain", "crop", "stretch"), default="contain")
    parser.add_argument("--dither", choices=("none", "fs"), default="none")
    parser.add_argument("--note", default="No lip-sync alignment is claimed.")
    args = parser.parse_args()
    try:
        print(json.dumps(convert(args), indent=2))
    except (ValueError, FileNotFoundError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"Conversion failed: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
