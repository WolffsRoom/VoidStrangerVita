from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import math, qrcode

BASE = Path(r"C:\Users\wolff\Documents\SDKVita\VoidStranger\assets\info_load")
OUT = BASE / "preview" / "missing_files_scene_preview.gif"
W,H = 960,544
FPS = 20
DT = 1000//FPS

bg = Image.open(BASE/'background'/'background.png').convert('RGBA')
effect = Image.open(BASE/'background'/'effect.png').convert('RGBA')
textbox = Image.open(BASE/'ui'/'textbox.png').convert('RGBA')
sera = [Image.open(BASE/'character'/f'spr_sera_{i:03d}.png').convert('RGBA') for i in range(4)]
portrait = Image.open(BASE/'portrait'/'spr_sera_port_neutral_000.png').convert('RGBA')
font_path = str(BASE/'font'/'Alkhemikal.ttf')
# Original game uses Alkhemikal at logical size 12. The Vita presentation scale is ~3.8x.
dialog_font = ImageFont.truetype(font_path, 46)
question_font = ImageFont.truetype(font_path, 42)
option_font = ImageFont.truetype(font_path, 44)
small_font = ImageFont.truetype(font_path, 28)

qr = qrcode.QRCode(version=4, box_size=4, border=2)
qr.add_data('https://github.com/WolffsRoom/VoidStrangerVita')
qr.make(fit=True)
qr_img = qr.make_image(fill_color='white', back_color='black').convert('RGBA')
qr_img = qr_img.resize((156,156), Image.Resampling.NEAREST)

frames=[]

def compose(t, sera_y=210):
    im = bg.copy()
    # User-requested effect opacity oscillation: 5%-15% over 3.5s.
    a = 0.10 + 0.05*math.sin((t/3.5)*math.tau)
    fx = effect.copy()
    alpha = fx.getchannel('A').point(lambda p: int(p*a))
    fx.putalpha(alpha)
    im.alpha_composite(fx)
    idx = int(t*12) % 4
    spr = sera[idx].resize((64,64), Image.Resampling.NEAREST)
    im.alpha_composite(spr, (W//2-32, int(sera_y)-32))
    return im

def draw_textbox(im, text, reveal=1.0, portrait_on=True):
    im.alpha_composite(textbox)
    if portrait_on:
        p = portrait.resize((192,192), Image.Resampling.NEAREST)
        # Keep portrait above the box, centered like the original Sera composition.
        im.alpha_composite(p, (W//2-96, 88))
    d=ImageDraw.Draw(im)
    shown = text[:max(0, min(len(text), int(round(len(text)*reveal))))]
    # Textbox prepared art begins around y=344; place text with original-like inset.
    x=78; y=374
    maxw=826
    words=shown.split(' ')
    lines=[]; cur=''
    for w in words:
        test=(cur+' '+w).strip()
        if d.textbbox((0,0),test,font=dialog_font)[2] <= maxw:
            cur=test
        else:
            if cur: lines.append(cur)
            cur=w
    if cur: lines.append(cur)
    for i,line in enumerate(lines[:3]):
        d.text((x,y+i*44), line, font=dialog_font, fill='white')
    return im

def add_hold(im, secs):
    n=max(1,int(round(secs*FPS)))
    frames.extend([im.copy() for _ in range(n)])

# 0) Sera enters from top over ~3.2 s, matching original movement feel.
for i in range(int(3.2*FPS)):
    t=i/FPS
    y=-32 + (96*(i/(3.2*FPS-1)))
    # map logical endpoint y=64 to a visually similar Vita-space position
    im=compose(t, sera_y=120 + (210-120)*(i/(3.2*FPS-1)))
    frames.append(im)

# 1) Hello. typewriter
text='Hello.'
for i in range(max(1,int(len(text)*0.10*FPS))):
    r=min(1,(i+1)/max(1,int(len(text)*0.10*FPS)))
    frames.append(draw_textbox(compose(3.2+i/FPS), text, r))
add_hold(draw_textbox(compose(4.0), text, 1), 0.8)

# 2) Main line, same dialog font size as game presentation
text='You need the game files, you know?'
for i in range(max(1,int(len(text)*0.07*FPS))):
    r=min(1,(i+1)/max(1,int(len(text)*0.07*FPS)))
    frames.append(draw_textbox(compose(5.0+i/FPS), text, r))
add_hold(draw_textbox(compose(7.5), text, 1), 1.0)

# 3) Question screen: no textbox, game-like centered resolve choice.
for sel in (0,1):
    im=compose(9.0+sel*0.8, sera_y=228)
    d=ImageDraw.Draw(im)
    q='Do you already have the required game files?'
    qb=d.textbbox((0,0),q,font=question_font)
    d.text(((W-(qb[2]-qb[0]))//2,82),q,font=question_font,fill='white')
    opts=['Yes','No']
    for j,o in enumerate(opts):
        fill='white' if j==sel else (125,125,125,255)
        label=f'[{o}]'
        bb=d.textbbox((0,0),label,font=option_font)
        d.text(((W-(bb[2]-bb[0]))//2,330+j*58),label,font=option_font,fill=fill)
    add_hold(im,0.9)

# 4) No path -> explanation + QR
text="You'll need a legitimate copy of Void Stranger."
for i in range(max(1,int(len(text)*0.055*FPS))):
    r=min(1,(i+1)/max(1,int(len(text)*0.055*FPS)))
    frames.append(draw_textbox(compose(11+i/FPS), text, r))
add_hold(draw_textbox(compose(13.5),text,1),0.8)

text='Use the Vita Patcher to prepare the required files.'
for i in range(max(1,int(len(text)*0.05*FPS))):
    r=min(1,(i+1)/max(1,int(len(text)*0.05*FPS)))
    frames.append(draw_textbox(compose(14+i/FPS), text, r))
add_hold(draw_textbox(compose(16.5),text,1),0.7)

im=compose(17.2,sera_y=215)
d=ImageDraw.Draw(im)
im.alpha_composite(qr_img,(W-188,140))
url='github.com/WolffsRoom/VoidStrangerVita'
bb=d.textbbox((0,0),url,font=small_font)
d.text((W-20-(bb[2]-bb[0]),315),url,font=small_font,fill='white')
im=draw_textbox(im,'Use the Vita Patcher to prepare the required files.',1)
add_hold(im,2.2)

# Save optimized GIF.
frames[0].save(OUT, save_all=True, append_images=frames[1:], duration=DT, loop=0, disposal=2, optimize=True)
print(f'OUT={OUT}')
print(f'FRAMES={len(frames)}')
print(f'DIALOG_FONT=46')
print(f'SIZE={OUT.stat().st_size}')
