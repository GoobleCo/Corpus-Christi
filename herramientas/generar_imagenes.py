"""
Genera imagenes DE PRUEBA para todos los bloques, enemigos y finales.

Solo crea las que faltan: si ya pusiste tu propia imagen, no la toca.
Para regenerar todo:  python3 herramientas/generar_imagenes.py --todas

Tamanos recomendados para tus imagenes:
  assets/lugares/<slug>.png   1280 x 400  (se recorta para cubrir la zona)
  assets/enemigos/<slug>.png  PNG con fondo transparente, ~600 x 700
  assets/finales/<slug>.png   1280 x 400
  assets/portada.png          1280 x 400
"""
import os, re, sys, random, hashlib
from PIL import Image, ImageDraw, ImageFont, ImageFilter

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(RAIZ, "assets")
TODAS = "--todas" in sys.argv
FUENTE = os.path.join(ASSETS, "fuente_negrita.ttf")

COLORES = {
    "C_CELESTE": (46, 150, 170), "C_VERDE": (52, 120, 60), "C_NARANJA": (170, 96, 34),
    "C_ROJO": (150, 36, 36), "C_VIOLETA": (120, 40, 130),
}

def fuente(t):
    return ImageFont.truetype(FUENTE, t)

def guardar(img, ruta):
    os.makedirs(os.path.dirname(ruta), exist_ok=True)
    if os.path.exists(ruta) and not TODAS:
        return False
    if img.mode == "RGBA":
        # enemigos: silueta oscura con un borde gris apenas visible
        alfa = img.split()[3]
        borde = alfa.filter(ImageFilter.MaxFilter(5))
        sil = Image.new("RGBA", img.size, (70, 70, 70, 0))
        sil.putalpha(borde)
        cuerpo = Image.new("RGBA", img.size, (8, 8, 8, 0))
        cuerpo.putalpha(alfa)
        img = Image.alpha_composite(sil, cuerpo)
    else:
        img = img.convert("L").convert("RGB")   # imagenes de prueba en blanco y negro
    img.save(ruta)
    return True

def degradado(w, h, arriba, abajo):
    img = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(img)
    for y in range(h):
        t = y / (h - 1)
        d.line([(0, y), (w, y)], fill=tuple(int(arriba[i] * (1 - t) + abajo[i] * t) for i in range(3)))
    return img

def oscuro(c, f):
    return tuple(int(v * f) for v in c)

def ciudad(img, rnd, color):
    d = ImageDraw.Draw(img, "RGBA")
    w, h = img.size
    for capa, (base, alt, alpha) in enumerate([(h - 60, 220, 120), (h - 30, 170, 200), (h, 120, 255)]):
        x = -20
        while x < w:
            bw = rnd.randint(60, 150)
            bh = rnd.randint(alt // 2, alt + 60)
            col = oscuro(color, 0.18 + 0.08 * (2 - capa)) + (alpha,)
            d.rectangle([x, base - bh, x + bw, base], fill=col)
            if capa == 2:
                for wy in range(base - bh + 14, base - 10, 22):
                    for wx in range(x + 10, x + bw - 10, 18):
                        if rnd.random() < 0.18:
                            d.rectangle([wx, wy, wx + 8, wy + 10], fill=(230, 200, 120, 160))
            x += bw + rnd.randint(-10, 25)

def tunel(img, rnd, color):
    d = ImageDraw.Draw(img, "RGBA")
    w, h = img.size
    cx = w // 2 + rnd.randint(-200, 200)
    for i in range(9, 0, -1):
        rw, rh = 90 * i, 46 * i
        d.ellipse([cx - rw, h * 0.55 - rh, cx + rw, h * 0.55 + rh],
                  outline=oscuro(color, 0.5) + (150,), width=10, fill=oscuro(color, 0.05 * i) + (255,))
    for i in range(14):
        x = rnd.randint(0, w)
        d.line([(x, 0), (x + rnd.randint(-60, 60), rnd.randint(30, 140))], fill=(20, 20, 20, 180), width=3)
    d.ellipse([cx - 60, h * 0.55 - 30, cx + 60, h * 0.55 + 30], fill=(250, 230, 160, 60))

def reflectores(img, rnd):
    capa = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(capa)
    w, h = img.size
    for _ in range(3):
        x = rnd.randint(100, w - 100)
        d.polygon([(x, h), (x - 30, h), (x + rnd.randint(-400, 400), 0), (x + rnd.randint(-400, 400) + 120, 0)],
                  fill=(220, 240, 255, 40))
    capa = capa.filter(ImageFilter.GaussianBlur(6))
    img.paste(Image.alpha_composite(img.convert("RGBA"), capa).convert("RGB"))

def rotulo(img, texto, sub):
    d = ImageDraw.Draw(img, "RGBA")
    w, h = img.size
    f = fuente(44)
    tw = d.textlength(texto, font=f)
    d.text(((w - tw) / 2 + 3, h * 0.42 + 3), texto, font=f, fill=(0, 0, 0, 160))
    d.text(((w - tw) / 2, h * 0.42), texto, font=f, fill=(255, 255, 255, 110))
    f2 = fuente(16)
    sw = d.textlength(sub, font=f2)
    d.text(((w - sw) / 2, h * 0.42 + 58), sub, font=f2, fill=(255, 255, 255, 90))

def bloques():
    src = open(os.path.join(RAIZ, "src", "mapa.c"), encoding="utf-8").read()
    patron = re.compile(r'\[(B_\w+)\]\s*=\s*\{"(\w+)",\s*"([^"]+)",\s*(C_\w+),\s*(\d)')
    con_bioma = set(re.findall(r'\[(B_\w+)\]\s*=\s*"\w+"', src))   # usan assets/biomas/
    n = 0
    for enum, slug, nombre, color, sub in patron.findall(src):
        if enum in con_bioma:
            continue
        rnd = random.Random(int(hashlib.md5(slug.encode()).hexdigest(), 16))
        c = COLORES[color]
        img = degradado(1280, 400, oscuro(c, 0.55), (10, 10, 14))
        if sub == "1":
            tunel(img, rnd, c)
        else:
            ciudad(img, rnd, c)
            if color == "C_CELESTE":
                reflectores(img, rnd)
        rotulo(img, nombre, "imagen de prueba · reemplazá assets/lugares/%s.png" % slug)
        n += guardar(img, os.path.join(ASSETS, "lugares", slug + ".png"))
    return n

def figura(color, tipo):
    img = Image.new("RGBA", (600, 700), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    piel = color
    sombra = oscuro(color[:3], 0.6) + (255,)
    if tipo == "stalker":
        d.polygon([(150, 640), (230, 430), (380, 380), (470, 470), (520, 640), (440, 640), (400, 520), (300, 520), (230, 640)], fill=sombra)
        d.polygon([(230, 430), (90, 560), (110, 580), (260, 470)], fill=piel)
        d.polygon([(380, 380), (560, 520), (540, 540), (370, 430)], fill=piel)
        d.ellipse([320, 280, 430, 390], fill=piel)
        ojos = [(350, 325), (395, 325)]
    else:
        d.rounded_rectangle([200, 260, 400, 520], 40, fill=sombra)          # cuerpo
        d.rectangle([215, 500, 290, 690], fill=oscuro(color[:3], 0.45) + (255,))        # piernas
        d.rectangle([310, 500, 385, 690], fill=oscuro(color[:3], 0.45) + (255,))
        if tipo == "vandalo":
            d.polygon([(200, 300), (110, 420), (140, 440), (230, 350)], fill=sombra)
            d.polygon([(400, 300), (500, 390), (480, 420), (380, 350)], fill=sombra)
            d.line([(470, 400), (590, 330)], fill=(40, 40, 40, 255), width=16)   # arma
            d.ellipse([230, 110, 370, 270], fill=(30, 30, 34, 255))              # capucha
            d.ellipse([255, 150, 345, 255], fill=piel)
        else:
            d.polygon([(200, 290), (60, 330), (60, 360), (220, 340)], fill=piel)   # brazos hacia adelante
            d.polygon([(400, 290), (540, 320), (540, 350), (380, 340)], fill=piel)
            d.ellipse([235, 120, 365, 270], fill=piel)
            if tipo == "policia":
                d.rectangle([225, 115, 375, 150], fill=(20, 30, 70, 255))
                d.rectangle([215, 145, 385, 160], fill=(15, 20, 50, 255))
                d.ellipse([320, 320, 350, 350], fill=(220, 190, 60, 255))
        ojos = [(275, 195), (325, 195)]
    for (x, y) in ojos:
        d.ellipse([x - 10, y - 7, x + 10, y + 7], fill=(255, 60, 40, 255))
    return img

def enemigos():
    datos = {
        "infectado_comun": ((110, 140, 100, 255), "comun"),
        "policia_infectado": ((70, 90, 150, 255), "policia"),
        "stalker": ((120, 90, 130, 255), "stalker"),
        "vandalo": ((150, 60, 50, 255), "vandalo"),
    }
    n = 0
    for slug, (c, tipo) in datos.items():
        n += guardar(figura(c, tipo), os.path.join(ASSETS, "enemigos", slug + ".png"))
    return n

def finales():
    datos = {
        "infeccion_tardia": ((90, 120, 60), "INFECCIÓN TARDÍA"),
        "cuarentena": ((60, 90, 110), "CUARENTENA"),
        "helicoptero": ((50, 120, 160), "HELICÓPTERO"),
        "tren": ((110, 70, 40), "EL TREN"),
        "bote": ((40, 90, 140), "EL BOTE"),
        "muerte": ((120, 20, 20), "MUERTE"),
        "convertido": ((70, 110, 40), "CONVERTIDO"),
    }
    n = 0
    for slug, (c, titulo) in datos.items():
        rnd = random.Random(slug)
        img = degradado(1280, 400, c, (8, 8, 10))
        ciudad(img, rnd, c)
        rotulo(img, titulo, "imagen de prueba · reemplazá assets/finales/%s.png" % slug)
        n += guardar(img, os.path.join(ASSETS, "finales", slug + ".png"))
    return n

def portada():
    rnd = random.Random("portada")
    img = degradado(1280, 400, (110, 30, 26), (8, 8, 10))
    ciudad(img, rnd, (150, 40, 30))
    reflectores(img, rnd)
    rotulo(img, "ESCAPE DE LA CIUDAD", "imagen de prueba · reemplazá assets/portada.png")
    return guardar(img, os.path.join(ASSETS, "portada.png"))

if __name__ == "__main__":
    total = bloques() + enemigos() + finales() + portada()
    print("Imagenes creadas:", total)
