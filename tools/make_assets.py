#!/usr/bin/env python3
"""
Gera os recursos gráficos do 4 Conexões a partir das folhas em assets/src.

Saídas:
  assets/assets.pak   pixels de todas as sprites (alinhados a 128 bytes)
  src/assets.h        enum das sprites e tabela de glifos da fonte grande
  src/assets.c        tabelas + inclusão do .pak via .incbin

Formatos no .pak:
  PSM 0 = RGBA32 com alfa no padrão do GS (0..128, 128 = opaco)
  PSM 2 = RGBA16 (5551), usado só na capa (opaca)

Requer Python 3, Pillow, numpy e scipy:
  pip install pillow numpy scipy
  python3 tools/make_assets.py
"""
import os
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "assets", "src")

PSM_CT32 = 0
PSM_CT16 = 2


def load(name):
    return Image.open(os.path.join(SRC, name)).convert("RGB")


# ----------------------------------------------------------------------------
# Remoção do fundo branco
# ----------------------------------------------------------------------------
def cut_out(img, box, margin=6):
    """Recorta 'box' (x, y, w, h) e torna transparente o branco ligado à borda."""
    x, y, w, h = box
    x0, y0 = max(0, x - margin), max(0, y - margin)
    x1, y1 = min(img.width, x + w + margin), min(img.height, y + h + margin)
    a = np.asarray(img.crop((x0, y0, x1, y1))).astype(np.float32)

    mn = a.min(axis=2)
    mx = a.max(axis=2)
    whiteish = (mn > 222) & (mx - mn < 30)
    lab, _ = ndimage.label(whiteish)
    edge = np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))
    edge = edge[edge != 0]
    bg = np.isin(lab, edge)

    alpha = np.where(bg, 0.0, 255.0)
    rgb = a.copy()

    # Pixels de borda (antisserrilhado entre contorno preto e fundo branco):
    # assume mistura de preto com branco e recupera o alfa.
    near = ndimage.binary_dilation(bg, iterations=2) & ~bg
    lum = a.mean(axis=2)
    grayish = (mx - mn) < 40
    soft = near & grayish & (lum > 60)
    alpha[soft] = np.clip(255.0 - lum[soft], 0, 255)
    rgb[soft] = 0

    # Descarta pedaços de sprites vizinhas: componentes que tocam a borda
    # do recorte (exceto o maior, que é a própria sprite).
    comp, n = ndimage.label(alpha > 0)
    if n > 1:
        sizes = ndimage.sum(np.ones_like(alpha), comp, range(1, n + 1))
        biggest = int(np.argmax(sizes)) + 1
        border = set(np.concatenate([comp[0], comp[-1], comp[:, 0], comp[:, -1]]).tolist())
        for lbl in border:
            if lbl not in (0, biggest):
                alpha[comp == lbl] = 0

    rgba = np.dstack([rgb, alpha]).astype(np.uint8)
    out = Image.fromarray(rgba, "RGBA")
    bbox = out.getchannel("A").getbbox()
    if bbox:
        out = out.crop(bbox)
    return out


def scale_to(im, height=None, width=None):
    w, h = im.size
    if height is not None:
        nw, nh = max(1, round(w * height / h)), height
    else:
        nw, nh = width, max(1, round(h * width / w))
    pm = im.convert("RGBa").resize((nw, nh), Image.LANCZOS)
    return pm.convert("RGBA")


def pad(im, p=2):
    out = Image.new("RGBA", (im.width + 2 * p, im.height + 2 * p), (0, 0, 0, 0))
    out.paste(im, (p, p))
    return out


# ----------------------------------------------------------------------------
# Sprites
# ----------------------------------------------------------------------------
SPRITES = []  # (nome, Image, psm)


def add(name, im, psm=PSM_CT32):
    SPRITES.append((name, im, psm))


def build_sprites():
    pers = load("personagens.png")
    iface = load("interface.png")
    sheet = load("sprites.png")
    fonte = load("fonte.png")

    # Logo
    add("LOGO", pad(scale_to(cut_out(fonte, (350, 8, 715, 335)), width=400)))

    # Personagens grandes (tela de título)
    add("BOY_BIG", pad(scale_to(cut_out(pers, (72, 19, 217, 343)), height=230)))
    add("GIRL_BIG", pad(scale_to(cut_out(pers, (1148, 14, 240, 348)), height=230)))

    # Poses de corpo inteiro (laterais da partida)
    BH = 132
    boy = {
        "IDLE": (279, 145, 92, 172),
        "CHEER": (579, 129, 111, 188),
        "SAD": (685, 157, 97, 158),
        "THINK": (787, 143, 91, 174),
        "SURPRISE": (900, 118, 92, 199),
    }
    girl = {
        "IDLE": (57, 407, 124, 207),
        "CHEER": (468, 376, 140, 183),
        "SAD": (635, 422, 125, 191),
        "THINK": (785, 406, 123, 211),
        "SURPRISE": (939, 362, 125, 254),
    }
    for k, b in boy.items():
        im = cut_out(pers, b)
        h = BH if k not in ("SAD",) else int(BH * 0.85)
        add("BOY_" + k, pad(scale_to(im, height=h)))
    for k, b in girl.items():
        im = cut_out(pers, b)
        h = BH if k not in ("SAD",) else int(BH * 0.85)
        if k == "SURPRISE":
            h = int(BH * 1.15)
        add("GIRL_" + k, pad(scale_to(im, height=h)))

    # Rostos (8 expressões de cada)
    boy_x = [24, 206, 382, 562, 732, 909, 1086, 1266]
    girl_x = [19, 198, 375, 554, 731, 905, 1078, 1261]
    for i, x in enumerate(boy_x):
        add("BOY_FACE%d" % i, pad(scale_to(cut_out(pers, (x, 667, 167, 158)), height=72)))
    for i, x in enumerate(girl_x):
        add("GIRL_FACE%d" % i, pad(scale_to(cut_out(pers, (x, 876, 168, 168)), height=72)))

    # Peças
    add("DISC_RED", pad(scale_to(cut_out(iface, (10, 272, 97, 98)), height=64)))
    add("DISC_YELLOW", pad(scale_to(cut_out(iface, (113, 272, 98, 98)), height=64)))

    # Ícones
    icons = {
        "ICON_1P": (iface, (279, 567, 58, 61)),
        "ICON_2P": (iface, (341, 568, 68, 57)),
        "ICON_PAD": (iface, (414, 572, 68, 53)),
        "ICON_TROPHY": (iface, (486, 567, 63, 59)),
        "ICON_CROWN": (iface, (558, 568, 66, 57)),
        "ICON_HELP": (iface, (688, 563, 44, 61)),
        "ICON_STAR": (iface, (1130, 775, 111, 90)),
        "BTN_CROSS": (sheet, (843, 670, 43, 41)),
        "BTN_CIRCLE": (sheet, (1001, 669, 43, 42)),
        "BTN_TRIANGLE": (sheet, (1139, 669, 44, 43)),
        "BTN_SQUARE": (sheet, (1288, 668, 44, 43)),
    }
    for name, (src, box) in icons.items():
        h = 28 if name.startswith("BTN_") else 40
        add(name, pad(scale_to(cut_out(src, box, margin=3), height=h)))

    # Capa (tela de abertura), 640x448 em 16 bits
    capa = load("capa.png")
    a = np.asarray(capa).copy()
    # Esconde os selos do canto (classificação e região) copiando o fundo ao lado.
    a[900:1070, 40:160] = a[900:1070, 165:166]
    a[18:64, 1270:1440] = a[70:116, 1270:1440]
    capa = Image.fromarray(a).resize((640, 448), Image.LANCZOS).convert("RGBA")
    add("COVER", capa, PSM_CT16)


# ----------------------------------------------------------------------------
# Fonte grande (atlas)
# ----------------------------------------------------------------------------
GLYPH_H = 34  # altura das maiúsculas no atlas


def accent_image(kind, size):
    """Desenha acentos no estilo da fonte (amarelo com contorno preto)."""
    S = 8
    w, h = size
    im = Image.new("RGBA", (w * S, h * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    yellow = (255, 214, 10, 255)
    black = (0, 0, 0, 255)
    o = 3 * S

    def stroke(points, width):
        d.line(points, fill=black, width=width + 2 * o, joint="curve")
        for p in (points[0], points[-1]):
            r = (width + 2 * o) / 2
            d.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=black)
        d.line(points, fill=yellow, width=width, joint="curve")
        for p in (points[0], points[-1]):
            r = width / 2
            d.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=yellow)

    W, H = w * S, h * S
    t = 5 * S
    if kind == "ACUTE":
        stroke([(W * 0.35, H * 0.8), (W * 0.7, H * 0.25)], t)
    elif kind == "GRAVE":
        stroke([(W * 0.3, H * 0.25), (W * 0.65, H * 0.8)], t)
    elif kind == "CIRC":
        stroke([(W * 0.2, H * 0.8), (W * 0.5, H * 0.25), (W * 0.8, H * 0.8)], t)
    elif kind == "TILDE":
        pts = []
        for i in range(17):
            u = i / 16
            pts.append((W * (0.15 + 0.7 * u), H * (0.55 - 0.25 * np.sin(u * 2 * np.pi))))
        stroke(pts, t)
    elif kind == "CEDIL":
        stroke([(W * 0.5, H * 0.1), (W * 0.5, H * 0.45), (W * 0.75, H * 0.6),
                (W * 0.4, H * 0.9)], t)
    return im.resize((w, h), Image.LANCZOS)


def build_font():
    src = load("logo_personagens.png")
    rows = [
        ("ABCDEFGHIJ", [(78, 405, 119, 113), (227, 405, 112, 113), (366, 405, 110, 114),
                        (496, 405, 117, 113), (638, 406, 109, 112), (768, 406, 110, 112),
                        (897, 406, 118, 113), (1036, 406, 124, 112), (1182, 407, 75, 111),
                        (1272, 407, 109, 113)]),
        ("KLMNOPQRST", [(47, 536, 127, 114), (191, 536, 103, 115), (307, 535, 150, 115),
                        (475, 535, 129, 115), (620, 535, 118, 117), (755, 535, 114, 116),
                        (883, 533, 124, 125), (1025, 535, 118, 115), (1158, 535, 115, 121),
                        (1298, 536, 109, 116)]),
        ("UVWXYZ", [(295, 670, 129, 116), (445, 670, 126, 115), (585, 669, 160, 116),
                    (751, 669, 132, 117), (902, 669, 119, 116), (1029, 670, 124, 114)]),
        ("0123456789", [(166, 803, 112, 121), (295, 805, 81, 118), (384, 804, 107, 120),
                        (500, 804, 107, 122), (613, 805, 112, 120), (733, 807, 105, 119),
                        (848, 805, 104, 122), (965, 806, 96, 118), (1062, 804, 105, 122),
                        (1179, 804, 104, 123)]),
        ("!?:;,.-+=", [(43, 947, 65, 109), (113, 948, 80, 109), (195, 965, 58, 84),
                       (255, 965, 56, 95), (313, 1007, 49, 57), (371, 1003, 48, 47),
                       (429, 985, 77, 42), (576, 961, 80, 81), (656, 974, 75, 63)]),
    ]
    cap = 114.0
    scale = GLYPH_H / cap
    glyphs = []  # (codepoint, Image, yoff)
    for chars, boxes in rows:
        bottoms = sorted(y + h for (x, y, w, h) in boxes)
        base = bottoms[len(bottoms) // 2]
        if chars.startswith("!"):
            base = 1056
        for ch, box in zip(chars, boxes):
            im = cut_out(src, box, margin=3)
            top = box[1]
            yoff = (top - (base - cap)) * scale
            gh = max(1, round(im.height * scale))
            g = scale_to(im, height=gh)
            glyphs.append((ord(ch), g, yoff))

    # ¡ e ¿: ! e ? girados
    lookup = {cp: (g, y) for cp, g, y in glyphs}
    for cp, srccp in ((0xA1, ord("!")), (0xBF, ord("?"))):
        g, _ = lookup[srccp]
        r = g.rotate(180)
        glyphs.append((cp, r, GLYPH_H * 0.25))

    # Acentos (códigos privados 0xE000+)
    acc_w, acc_h = int(GLYPH_H * 0.55), int(GLYPH_H * 0.42)
    for i, kind in enumerate(["ACUTE", "GRAVE", "CIRC", "TILDE", "CEDIL"]):
        size = (acc_w, acc_h) if kind != "TILDE" else (int(acc_w * 1.3), acc_h)
        if kind == "CEDIL":
            size = (int(acc_w * 0.8), int(acc_h * 1.1))
        glyphs.append((0xE000 + i, accent_image(kind, size), 0))

    # Monta o atlas
    AW = 512
    x = y = 0
    row_h = 0
    placed = []
    for cp, g, yoff in glyphs:
        g = pad(g, 1)
        if x + g.width > AW:
            x = 0
            y += row_h
            row_h = 0
        placed.append((cp, x, y, g, yoff))
        x += g.width
        row_h = max(row_h, g.height)
    AH = y + row_h
    atlas = Image.new("RGBA", (AW, AH), (0, 0, 0, 0))
    table = []
    for cp, gx, gy, g, yoff in placed:
        atlas.paste(g, (gx, gy))
        adv = g.width - 7  # contornos encostados, como no logotipo
        table.append((cp, gx, gy, g.width, g.height, int(round(yoff)) - 1, adv))
    add("FONT", atlas)
    return table


# ----------------------------------------------------------------------------
# Saída
# ----------------------------------------------------------------------------
def to_bytes(im, psm):
    a = np.asarray(im.convert("RGBA")).astype(np.uint32)
    if psm == PSM_CT32:
        out = a.copy()
        out[..., 3] = (a[..., 3] * 128 + 127) // 255
        return out.astype(np.uint8).tobytes()
    r, g, b = a[..., 0] >> 3, a[..., 1] >> 3, a[..., 2] >> 3
    v = r | (g << 5) | (b << 10) | (1 << 15)
    return v.astype("<u2").tobytes()


def main():
    build_sprites()
    font = build_font()

    blob = bytearray()
    entries = []
    for name, im, psm in SPRITES:
        while len(blob) % 128:
            blob.append(0)
        off = len(blob)
        blob += to_bytes(im, psm)
        entries.append((name, off, im.width, im.height, psm))

    with open(os.path.join(ROOT, "assets", "assets.pak"), "wb") as f:
        f.write(blob)

    h = []
    h.append("/* Gerado por tools/make_assets.py - não edite. */")
    h.append("#ifndef ASSETS_H\n#define ASSETS_H\n")
    h.append("#define ASSET_PSM_CT32 0\n#define ASSET_PSM_CT16 2\n")
    h.append("enum {")
    for name, *_ in entries:
        h.append("    SPR_%s," % name)
    h.append("    SPR_COUNT\n};\n")
    h.append("typedef struct {\n    unsigned int offset;\n    unsigned short w, h;\n"
             "    unsigned char psm;\n} AssetSprite;\n")
    h.append("typedef struct {\n    unsigned short cp;\n    short x, y, w, h, yoff, adv;\n"
             "} BigGlyph;\n")
    h.append("#define BIGFONT_CAP %d" % GLYPH_H)
    h.append("#define BIGFONT_ACUTE 0xE000\n#define BIGFONT_GRAVE 0xE001\n"
             "#define BIGFONT_CIRC 0xE002\n#define BIGFONT_TILDE 0xE003\n"
             "#define BIGFONT_CEDIL 0xE004\n")
    h.append("extern const AssetSprite g_sprites[SPR_COUNT];")
    h.append("extern const BigGlyph g_glyphs[];")
    h.append("extern const int g_glyph_count;")
    h.append("extern const unsigned char g_asset_pak[];\n")
    h.append("#endif")
    with open(os.path.join(ROOT, "src", "assets.h"), "w") as f:
        f.write("\n".join(h) + "\n")

    c = []
    c.append("/* Gerado por tools/make_assets.py - não edite. */")
    c.append('#include "assets.h"\n')
    c.append("/* Pixels de todas as sprites (assets/assets.pak, %d bytes). */" % len(blob))
    c.append("__asm__(\n"
             '    ".section .rodata\\n"\n'
             '    ".balign 128\\n"\n'
             '    ".global g_asset_pak\\n"\n'
             '    "g_asset_pak:\\n"\n'
             '    ".incbin \\"assets/assets.pak\\"\\n"\n'
             '    ".previous\\n"\n'
             ");\n")
    c.append("const AssetSprite g_sprites[SPR_COUNT] = {")
    for name, off, w, hh, psm in entries:
        c.append("    { %d, %d, %d, %d }, /* %s */" % (off, w, hh, psm, name))
    c.append("};\n")
    c.append("const BigGlyph g_glyphs[] = {")
    for cp, x, y, w, hh, yoff, adv in font:
        c.append("    { 0x%04X, %d, %d, %d, %d, %d, %d }," % (cp, x, y, w, hh, yoff, adv))
    c.append("};\n")
    c.append("const int g_glyph_count = %d;" % len(font))
    with open(os.path.join(ROOT, "src", "assets.c"), "w") as f:
        f.write("\n".join(c) + "\n")

    total = sum(w * hh * (4 if psm == 0 else 2) for _, _, w, hh, psm in entries)
    print("%d sprites, %d glifos, %.1f KB" % (len(entries), len(font), total / 1024))
    for name, off, w, hh, psm in entries:
        print("  %-14s %4dx%-4d" % (name, w, hh))
    previews = [a.split("=", 1)[1] for a in sys.argv[1:] if a.startswith("--preview=")]
    if previews:
        prev = previews[0]
        os.makedirs(prev, exist_ok=True)
        for name, im, psm in SPRITES:
            bgim = Image.new("RGBA", im.size, (90, 40, 150, 255))
            bgim.alpha_composite(im)
            bgim.save(os.path.join(prev, name.lower() + ".png"))


if __name__ == "__main__":
    main()
