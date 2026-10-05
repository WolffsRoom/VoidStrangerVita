from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "assets" / "info_load"
OUT = BASE / "generated"
OUT.mkdir(parents=True, exist_ok=True)
FONT_PATH = BASE / "font" / "Alkhemikal.ttf"
if not FONT_PATH.exists():
    raise FileNotFoundError("Alkhemikal.ttf is not distributed. Copy it from a legitimate Void Stranger installation to assets/info_load/font/ before regenerating dialog assets.")
FONT = str(FONT_PATH)

def wrap(draw, text, font, max_width):
    words = text.split()
    lines, current = [], ""
    for word in words:
        candidate = (current + " " + word).strip()
        if draw.textbbox((0, 0), candidate, font=font)[2] <= max_width:
            current = candidate
        else:
            if current:
                lines.append(current)
            current = word
    if current:
        lines.append(current)
    return lines

def save_dialog(name, text, font_size=46, max_width=826):
    font = ImageFont.truetype(FONT, font_size)
    canvas = Image.new("RGBA", (960, 544), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    lines = wrap(draw, text, font, max_width)
    y = 381
    for i, line in enumerate(lines[:3]):
        draw.text((78, y + i * 44), line, font=font, fill="white")
    bbox = canvas.getbbox()
    if not bbox:
        raise RuntimeError(name)
    crop = canvas.crop(bbox)
    crop.save(OUT / name)
    return bbox, crop.size, lines

def save_question(name, selected):
    im = Image.new("RGBA", (728, 342), (0, 0, 0, 0))
    draw = ImageDraw.Draw(im)
    qfont = ImageFont.truetype(FONT, 34)
    ofont = ImageFont.truetype(FONT, 44)
    question = "I noticed you have the files, but they're not organized correctly. Would you like me to fix them for you?"
    lines = wrap(draw, question, qfont, 680)
    y = 8
    for line in lines:
        bb = draw.textbbox((0, 0), line, font=qfont)
        draw.text(((728 - (bb[2]-bb[0]))/2, y), line, font=qfont, fill="white")
        y += 38
    for j, option in enumerate(("Yes", "No")):
        label = f"[{option}]"
        bb = draw.textbbox((0, 0), label, font=ofont)
        fill = "white" if j == selected else (120,120,120,255)
        draw.text(((728 - (bb[2]-bb[0]))/2, 220 + j*58), label, font=ofont, fill=fill)
    im.save(OUT / name)

save_question("organize_question_yes.png", 0)
save_question("organize_question_no.png", 1)
for filename, text in [
    ("dialog_organize_decline.png", "Okay. You can review them yourself. See you later."),
    ("dialog_organize_manual.png", "I found more than one copy of a required file. Please review them yourself. See you later."),
]:
    print(filename, save_dialog(filename, text))
# Runtime dialogue font atlas. The Vita uses these metrics to reveal actual
# glyphs one-by-one instead of horizontally clipping a pre-rendered sentence.
def save_font_atlas():
    font = ImageFont.truetype(FONT, 46)
    first, last = 32, 126
    cell_w, cell_h, cols = 64, 72, 16
    count = last - first + 1
    rows = (count + cols - 1) // cols
    atlas = Image.new("RGBA", (cell_w * cols, cell_h * rows), (0, 0, 0, 0))
    draw = ImageDraw.Draw(atlas)
    metrics = []
    for code in range(first, last + 1):
        ch = chr(code)
        bbox = font.getbbox(ch)
        left, top, right, bottom = bbox
        width = max(0, right - left)
        height = max(0, bottom - top)
        advance = int(round(font.getlength(ch)))
        idx = code - first
        cell_x = (idx % cols) * cell_w
        cell_y = (idx // cols) * cell_h
        if width > 0 and height > 0 and ch != " ":
            draw.text((cell_x - left, cell_y - top), ch, font=font, fill="white")
        metrics.append((cell_x, cell_y, width, height, left, top, advance))
    atlas_path = OUT / "alkhemikal_dialog_atlas.png"
    atlas.save(atlas_path)

    header = ROOT / "src" / "vita-runner" / "source" / "missing_data_font_metrics.h"
    lines = [
        "#ifndef MISSING_DATA_FONT_METRICS_H",
        "#define MISSING_DATA_FONT_METRICS_H",
        "",
        "typedef struct MissingDataGlyphMetric {",
        "    unsigned short x, y, w, h;",
        "    short bearingX, bearingY;",
        "    unsigned short advance;",
        "} MissingDataGlyphMetric;",
        "",
        f"#define MDS_FONT_FIRST_CHAR {first}",
        f"#define MDS_FONT_LAST_CHAR {last}",
        f"#define MDS_FONT_ATLAS_W {cell_w * cols}",
        f"#define MDS_FONT_ATLAS_H {cell_h * rows}",
        "#define MDS_FONT_LINE_HEIGHT 44",
        "static const MissingDataGlyphMetric kMdsGlyphMetrics[] = {",
    ]
    for code, m in zip(range(first, last + 1), metrics):
        x,y,w,h,bx,by,adv = m
        label = chr(code).replace('\\','\\\\').replace("'","\\'")
        lines.append(f"    {{{x},{y},{w},{h},{bx},{by},{adv}}}, /* {code} '{label}' */")
    lines += ["};", "", "#endif", ""]
    header.write_text("\n".join(lines), encoding="ascii")
    print("font_atlas", atlas_path, atlas.size)
    print("font_metrics", header)

save_font_atlas()
