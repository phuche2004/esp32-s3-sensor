#!/usr/bin/env python3
"""Crop screenshots to remove left/right margins and focus on content"""
from PIL import Image
import os

def auto_crop_horizontal_margins(input_path, output_path, margin_percent=15):
    """
    Auto crop horizontal margins (left/right empty space)
    margin_percent: percentage of width to crop from each side (default 15%)
    """
    img = Image.open(input_path)
    width, height = img.size
    
    # Tính toán vùng crop: bỏ margin_percent từ mỗi bên trái-phải
    margin = int(width * margin_percent / 100)
    left = margin
    right = width - margin
    top = 0
    bottom = height
    
    crop_box = (left, top, right, bottom)
    cropped = img.crop(crop_box)
    cropped.save(output_path, optimize=True, quality=90)
    
    cropped_percent = (1 - cropped.width / width) * 100
    print(f"✓ {os.path.basename(input_path)}: {img.size} → {cropped.size} (-{cropped_percent:.0f}% width)")
    return cropped

# Đường dẫn folder
images_dir = "docs/images"

screenshots = [
    "tab1-wifi-setup.png",
    "tab2-dashboard.png",
    "tab3-led-control.png",
    "tab4-history-chart.png",
]

print("🔧 Cropping screenshots - removing left/right margins...\n")

for filename in screenshots:
    input_path = os.path.join(images_dir, filename)
    output_path = os.path.join(images_dir, filename)
    
    if os.path.exists(input_path):
        # Crop 20% từ mỗi bên (tổng 40% width)
        auto_crop_horizontal_margins(input_path, output_path, margin_percent=20)
    else:
        print(f"⚠ File not found: {filename}")

print("\n✨ Done! All screenshots cropped to remove side margins.")
