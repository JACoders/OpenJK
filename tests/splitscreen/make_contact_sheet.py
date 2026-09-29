#!/usr/bin/env python3
"""Create a labeled contact sheet from game screenshots."""

import argparse
from pathlib import Path

from PIL import Image, ImageDraw


def main() -> None:
	parser = argparse.ArgumentParser()
	parser.add_argument("output", type=Path)
	parser.add_argument("screenshots", nargs="+", type=Path)
	parser.add_argument("--columns", type=int, default=3)
	parser.add_argument("--width", type=int, default=600)
	args = parser.parse_args()

	if args.columns < 1 or args.width < 1:
		parser.error("columns and width must be positive")

	images = []
	for path in args.screenshots:
		with Image.open(path) as source:
			image = source.convert("RGB")
			height = round(args.width * image.height / image.width)
			images.append((path, image.resize((args.width, height))))

	label_height = 34
	cell_height = max(image.height for _, image in images) + label_height
	rows = (len(images) + args.columns - 1) // args.columns
	sheet = Image.new("RGB", (args.columns * args.width, rows * cell_height), (24, 24, 28))
	draw = ImageDraw.Draw(sheet)
	for index, (path, image) in enumerate(images):
		x = (index % args.columns) * args.width
		y = (index // args.columns) * cell_height
		sheet.paste(image, (x, y))
		draw.text((x + 8, y + image.height + 7), path.stem, fill="white")

	args.output.parent.mkdir(parents=True, exist_ok=True)
	sheet.save(args.output)
	print(f"wrote {args.output} with {len(images)} screenshots")


if __name__ == "__main__":
	main()
