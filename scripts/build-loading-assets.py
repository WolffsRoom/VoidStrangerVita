from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps
import shutil

ROOT = Path(__file__).resolve().parents[1]
assets = ROOT / 'assets'
src = assets / 'new_load' / 'runtime'
out = assets / 'load_assets'
font_path = assets / 'new_load' / 'Alkhemikal.ttf'
if not font_path.exists():
    raise FileNotFoundError('Alkhemikal.ttf is not distributed. Copy it from a legitimate Void Stranger installation to assets/new_load/ before regenerating loading assets.')
FONT_SIZE = 51
ATLAS_W, ATLAS_H = 256, 64
DIGIT_CELL_W, DIGIT_CELL_H = 40, 64
DIGIT_ATLAS_W, DIGIT_ATLAS_H = 440, 64
WHITE = (255, 255, 255, 255)

LANG = {
    'en': ('Loading textures ...', 'Loading complete!'),
    'ptbr': ('Carregando texturas ...', 'Carregamento concluído!'),
    'es': ('Cargando texturas ...', '¡Carga completa!'),
    'it': ('Caricamento texture ...', 'Caricamento completato!'),
    'tr': ('Dokular yükleniyor ...', 'Yükleme tamamlandı!'),
}

def rgba(path: Path) -> Image.Image:
    return Image.open(path).convert('RGBA')

def put(atlas, image, x, y):
    atlas.alpha_composite(image, (x, y))

def build_sprite_atlas():
    atlas = Image.new('RGBA', (ATLAS_W, ATLAS_H), (0, 0, 0, 0))
    row0 = [
        'stairs_00.png', 'floor_00.png', 'floor_01.png',
        'player_walk_right_00.png', 'player_walk_right_01.png',
        'player_place_right_00.png', 'player_place_right_01.png',
        'player_front_00.png', 'player_front_01.png', 'void_rod_right.png',
    ] + [f'player_fall_{i:02d}.png' for i in range(6)]
    for i, name in enumerate(row0):
        put(atlas, rgba(src / name), i * 16, 0)
    for i in range(8):
        put(atlas, rgba(src / 'sparkle' / f'spr_sparkle_{i:02d}.png'), i * 16, 16)
    for i in range(5):
        put(atlas, rgba(src / 'sweat' / f'spr_sweat_{i:02d}.png'), 128 + i * 16, 16)
    for i in range(10):
        swipe = rgba(src / 'swipe' / f'spr_player_swipe_{i:02d}.png')
        swipe = ImageOps.mirror(swipe).rotate(90, resample=Image.Resampling.NEAREST, expand=True)
        row = 32 if i < 8 else 48
        col = i if i < 8 else i - 8
        put(atlas, swipe, col * 32, row)
    atlas.save(out / 'loading_sprites.png')

def text_image(text: str, font: ImageFont.FreeTypeFont) -> Image.Image:
    probe = Image.new('RGBA', (1, 1), (0, 0, 0, 0))
    draw = ImageDraw.Draw(probe)
    box = draw.textbbox((0, 0), text, font=font)
    w = max(1, box[2] - box[0]) + 6
    h = max(1, box[3] - box[1]) + 6
    img = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.text((3 - box[0], 3 - box[1]), text, font=font, fill=WHITE)
    return img

def build_text_assets():
    font = ImageFont.truetype(str(font_path), FONT_SIZE)
    dims = {}
    for code, (loading, complete) in LANG.items():
        a = text_image(loading, font)
        b = text_image(complete, font)
        a.save(out / f'loading_{code}.png')
        b.save(out / f'complete_{code}.png')
        dims[code] = (a.size, b.size)

    digits = Image.new('RGBA', (DIGIT_ATLAS_W, DIGIT_ATLAS_H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(digits)
    for i, ch in enumerate('0123456789%'):
        box = draw.textbbox((0, 0), ch, font=font)
        gw, gh = box[2] - box[0], box[3] - box[1]
        x = i * DIGIT_CELL_W + (DIGIT_CELL_W - gw) // 2 - box[0]
        y = (DIGIT_CELL_H - gh) // 2 - box[1]
        draw.text((x, y), ch, font=font, fill=WHITE)
    digits.save(out / 'loading_digits.png')
    return dims

def main():
    out.mkdir(parents=True, exist_ok=True)
    for p in out.iterdir():
        if p.is_file(): p.unlink()
        elif p.is_dir(): shutil.rmtree(p)
    build_sprite_atlas()
    dims = build_text_assets()
    for code, (loading, complete) in dims.items():
        print(f'{code}: loading={loading[0]}x{loading[1]} complete={complete[0]}x{complete[1]}')
    print(f'digits={DIGIT_ATLAS_W}x{DIGIT_ATLAS_H} cell={DIGIT_CELL_W}x{DIGIT_CELL_H}')

if __name__ == '__main__':
    main()
