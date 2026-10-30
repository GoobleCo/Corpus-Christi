/*
 * MODULO INTERFAZ (raylib)
 * Dibuja la pantalla como una novela visual:
 *
 *   +--------------------------------------------------+
 *   |  IMAGEN del bloque (+ enemigo encima)            |
 *   +-----------------------------+--------------------+
 *   |  TITULO                     |  1  opcion         |
 *   |  descripcion / lo que pasó  |  2  opcion  ...    |
 *   +-----------------------------+--------------------+
 *
 * Minimalista: fuera de las imagenes, todo es blanco sobre negro.
 *
 * Las opciones se eligen con los numeros del teclado o tocando/clickeando.
 * Todo se dibuja en un lienzo de 1280x720 que despues se escala a la ventana.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "raylib.h"
#include "rlgl.h"
#include "juego.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#define ANCHO        1280
#define ALTO         720
#define IMG_ALTO     384
#define TITULO_Y     404
#define TXT_X        40
#define TXT_Y        448
#define TXT_W        700
#define TXT_ALTO     264
#define OPC_X        800
#define OPC_Y        400
#define OPC_W        440
#define OPC_H        34
#define OPC_SEP      2
#define MAX_VISIBLES 8
#define TAM_TITULO   26
#define TAM_TEXTO    18
#define LINEA        23
#define TAM_OPCION   20

/* Colores: solo blanco y negro */
#define C_NEGRO      (Color){ 0, 0, 0, 255 }
#define C_BLANCO     (Color){ 255, 255, 255, 255 }
#define C_APAGADO    (Color){ 255, 255, 255, 80 }

static Juego juego;
static Font fuente, fuente_negrita;
static RenderTexture2D lienzo;

/* estado de la interfaz */
static int pagina = 0;
static int version_vista = -1;
static float scroll = 0;            /* en lineas */
static int estaba_abajo = 0, arrastrando = 0;
static Vector2 inicio_toque, ultimo_toque;
static float scroll_inicio = 0;
static int cerrar = 0;

/* ======================= IMAGENES ======================= */

typedef struct { char clave[64]; Texture2D tex; int ok; } Imagen;
#define MAX_IMAGENES 160
static Imagen imagenes[MAX_IMAGENES];
static int n_imagenes = 0;

/* Carga assets/<clave>.png la primera vez que se pide (si existe) */
static Imagen *imagen(const char *clave)
{
    char ruta[128];
    int i;
    Imagen *im;
    if (clave[0] == '\0') return NULL;
    for (i = 0; i < n_imagenes; i++)
        if (strcmp(imagenes[i].clave, clave) == 0) return &imagenes[i];
    if (n_imagenes >= MAX_IMAGENES) return NULL;
    im = &imagenes[n_imagenes++];
    snprintf(im->clave, sizeof im->clave, "%s", clave);
    snprintf(ruta, sizeof ruta, "assets/%s.png", clave);
    im->ok = 0;
    if (FileExists(ruta)) {
        im->tex = LoadTexture(ruta);
        im->ok = im->tex.id != 0;
        if (im->ok) SetTextureFilter(im->tex, TEXTURE_FILTER_BILINEAR);
    }
    return im;
}

/* Dibuja la imagen cubriendo todo el rectangulo (recorta lo que sobra) */
static void dibujar_cubriendo(Texture2D t, Rectangle dst)
{
    float s = fmaxf(dst.width / t.width, dst.height / t.height);
    float w = dst.width / s, h = dst.height / s;
    Rectangle src = { (t.width - w) / 2, (t.height - h) / 2, w, h };
    DrawTexturePro(t, src, dst, (Vector2){ 0, 0 }, 0, WHITE);
}

/* Dibuja un enemigo apoyado en el borde de abajo de la imagen.
 * escala < 1 y tinte oscuro = esta mas atras, esperando. */
static void dibujar_enemigo(Texture2D t, float centro_x, float escala, Color tinte)
{
    float alto_max = (IMG_ALTO - 14) * escala, ancho_max = 560 * escala;
    float s = fminf(alto_max / t.height, ancho_max / t.width);
    float w = t.width * s, h = t.height * s;
    Rectangle dst = { centro_x - w / 2, IMG_ALTO - h, w, h };
    DrawTexturePro(t, (Rectangle){ 0, 0, (float)t.width, (float)t.height }, dst, (Vector2){ 0, 0 }, 0, tinte);
}

/* ======================= TEXTO ======================= */

static void texto(const char *s, float x, float y, float tam, Color c)
{
    DrawTextEx(fuente, s, (Vector2){ x, y }, tam, 0, c);
}

static float ancho_texto(Font f, const char *s, float tam)
{
    return MeasureTextEx(f, s, tam, 0).x;
}

#define MAX_LINEAS 400
#define LARGO_LINEA 256
static char lineas[MAX_LINEAS][LARGO_LINEA];
static int n_lineas = 0;

static void empujar_linea(const char *s)
{
    if (n_lineas >= MAX_LINEAS) return;
    snprintf(lineas[n_lineas++], LARGO_LINEA, "%s", s);
}

/* Parte el texto en lineas que entren en el ancho (respeta los \n) */
static void envolver(const char *txt, float ancho_max)
{
    char actual[LARGO_LINEA] = "", palabra[LARGO_LINEA], prueba[LARGO_LINEA];
    const char *p = txt;
    n_lineas = 0;
    while (*p) {
        int n = 0, inicio_parrafo = (actual[0] == '\0');
        size_t la, lp;
        if (*p == '\n') { empujar_linea(actual); actual[0] = '\0'; p++; continue; }
        /* los espacios al principio de un parrafo se mantienen (sangria) */
        if (inicio_parrafo) while (*p == ' ' && n < LARGO_LINEA / 2) palabra[n++] = *p++;
        else while (*p == ' ') p++;
        while (*p && *p != ' ' && *p != '\n' && n < LARGO_LINEA / 2) palabra[n++] = *p++;
        palabra[n] = '\0';
        if (n == 0) continue;
        la = strlen(actual);
        lp = strlen(palabra);
        if (la == 0) { memcpy(actual, palabra, lp + 1); continue; }
        if (la + 1 + lp < LARGO_LINEA) {
            /* probar si la palabra entra en la linea actual */
            memcpy(prueba, actual, la);
            prueba[la] = ' ';
            memcpy(prueba + la + 1, palabra, lp + 1);
            if (ancho_texto(fuente, prueba, TAM_TEXTO) <= ancho_max) {
                memcpy(actual, prueba, la + lp + 2);
                continue;
            }
        }
        empujar_linea(actual);   /* no entra: la palabra pasa a la linea siguiente */
        memcpy(actual, palabra, lp + 1);
    }
    if (actual[0] != '\0') empujar_linea(actual);
    /* sacar lineas vacias del final */
    while (n_lineas > 0 && lineas[n_lineas - 1][0] == '\0') n_lineas--;
}

/* Recorta un texto para que entre en un ancho, agregando "…" */
static void recortar(const char *s, float ancho_max, float tam, char *out, int tam_out)
{
    int len;
    snprintf(out, tam_out, "%s", s);
    if (ancho_texto(fuente, out, tam) <= ancho_max) return;
    len = (int)strlen(out);
    while (len > 0) {
        len--;
        while (len > 0 && (out[len] & 0xC0) == 0x80) len--;   /* no cortar un caracter UTF-8 */
        out[len] = '\0';
        {
            char tmp[160];
            snprintf(tmp, sizeof tmp, "%s…", out);
            if (ancho_texto(fuente, tmp, tam) <= ancho_max) { snprintf(out, tam_out, "%s", tmp); return; }
        }
    }
}

/* ======================= OPCIONES ======================= */

/* Que opciones se ven en la pagina actual.
 * Si hay mas de 8, se muestran 7 y la octava es "Más opciones". */
static int opciones_visibles(int *indices, int *hay_mas)
{
    int n = juego.pant.n_op, i, k = 0, por_pagina, paginas;
    *hay_mas = 0;
    if (n <= MAX_VISIBLES) {
        for (i = 0; i < n; i++) indices[k++] = i;
        return k;
    }
    por_pagina = MAX_VISIBLES - 1;
    paginas = (n + por_pagina - 1) / por_pagina;
    if (pagina >= paginas) pagina = 0;
    for (i = pagina * por_pagina; i < n && k < por_pagina; i++) indices[k++] = i;
    *hay_mas = 1;
    return k;
}

static int total_paginas(void)
{
    int n = juego.pant.n_op;
    if (n <= MAX_VISIBLES) return 1;
    return (n + MAX_VISIBLES - 2) / (MAX_VISIBLES - 1);
}

static Rectangle rect_boton(int i)
{
    return (Rectangle){ OPC_X, OPC_Y + i * (OPC_H + OPC_SEP), OPC_W, OPC_H };
}

/* Activa el boton numero i (0..7) de la pagina actual */
static void activar_boton(int i)
{
    int indices[MAX_VISIBLES], hay_mas, n = opciones_visibles(indices, &hay_mas);
    if (i < n) juego_elegir(&juego, indices[i]);
    else if (hay_mas && i == n) pagina = (pagina + 1) % total_paginas();
}

/* ======================= MUSICA =======================
 * assets/musica/paz.ogg       zonas de H.E.R.O. (celestes), el inicio y la portada
 * assets/musica/incierto.ogg  bloques con peligro
 * assets/musica/combate.ogg   durante un combate (si falta, sigue la de incierto)
 * assets/musica/final.ogg     pantalla de final
 * Si falta un archivo, simplemente no suena. Al cambiar de tema hay un fundido. */

enum { M_PAZ, M_INCIERTO, M_COMBATE, M_FINAL, NUM_MUSICAS };
static const char *ARCHIVO_MUSICA[NUM_MUSICAS] = {
    "assets/musica/paz.ogg", "assets/musica/incierto.ogg",
    "assets/musica/combate.ogg", "assets/musica/final.ogg",
};
#define VOLUMEN_MUSICA 0.6f
#define FUNDIDO        1.5f    /* segundos que tarda en entrar o salir un tema */

static Music musica[NUM_MUSICAS];
static int musica_ok[NUM_MUSICAS];
static float volumen[NUM_MUSICAS];
static int hay_musica = 0, silencio = 0;
static const Rectangle BOTON_MUSICA = { ANCHO - 64, 10, 54, 46 };

static void cargar_musica(void)
{
    int i;
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    for (i = 0; i < NUM_MUSICAS; i++) {
        musica_ok[i] = 0;
        if (!FileExists(ARCHIVO_MUSICA[i])) continue;
        musica[i] = LoadMusicStream(ARCHIVO_MUSICA[i]);
        musica_ok[i] = IsMusicValid(musica[i]);
        if (musica_ok[i]) { SetMusicVolume(musica[i], 0); hay_musica = 1; }
    }
}

/* Que tema corresponde a lo que esta pasando en el juego */
static int musica_deseada(void)
{
    int b = juego.j.bloque_actual, m;
    if (juego.estado == E_FINAL) m = M_FINAL;
    else if (juego.en_combate) m = M_COMBATE;
    else if (juego.estado == E_NOMBRE || juego.estado == E_CLASE) m = M_PAZ;
    else if (mapa[b].color == C_CELESTE || b == B_INICIO) m = M_PAZ;
    else m = M_INCIERTO;
    if (m == M_COMBATE && !musica_ok[M_COMBATE]) m = M_INCIERTO;
    return musica_ok[m] ? m : -1;
}

static void actualizar_musica(void)
{
    int i, deseada;
    float paso = GetFrameTime() / FUNDIDO;
    if (!hay_musica) return;
    deseada = musica_deseada();
    for (i = 0; i < NUM_MUSICAS; i++) {
        if (!musica_ok[i]) continue;
        if (i == deseada) {
            /* paz e incierto se pausan y siguen donde estaban;
             * combate y final se detienen, asi que vuelven a empezar desde el principio */
            if (!IsMusicStreamPlaying(musica[i])) PlayMusicStream(musica[i]);
            volumen[i] = fminf(1, volumen[i] + paso);
        } else {
            volumen[i] = fmaxf(0, volumen[i] - paso);
            if (volumen[i] <= 0 && IsMusicStreamPlaying(musica[i])) {
                if (i == M_COMBATE || i == M_FINAL) StopMusicStream(musica[i]);
                else PauseMusicStream(musica[i]);
            }
        }
        if (IsMusicStreamPlaying(musica[i])) {
            SetMusicVolume(musica[i], silencio ? 0 : volumen[i] * VOLUMEN_MUSICA);
            UpdateMusicStream(musica[i]);
        }
    }
}

static void descargar_musica(void)
{
    int i;
    for (i = 0; i < NUM_MUSICAS; i++) if (musica_ok[i]) UnloadMusicStream(musica[i]);
    if (IsAudioDeviceReady()) CloseAudioDevice();
}

/* ======================= ENTRADA ======================= */

static Vector2 escala_y_offset(float *escala)
{
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float s = fminf(sw / ANCHO, sh / ALTO);
    *escala = s;
    return (Vector2){ (sw - ANCHO * s) / 2, (sh - ALTO * s) / 2 };
}

static Vector2 a_lienzo(Vector2 p)
{
    float s;
    Vector2 o = escala_y_offset(&s);
    return (Vector2){ (p.x - o.x) / s, (p.y - o.y) / s };
}

static Vector2 puntero(void)
{
    if (GetTouchPointCount() > 0) return a_lienzo(GetTouchPosition(0));
    return a_lienzo(GetMousePosition());
}

static int en_area_texto(Vector2 p)
{
    return p.x >= TXT_X - 10 && p.x <= TXT_X + TXT_W + 10 && p.y >= TXT_Y - 6 && p.y <= TXT_Y + TXT_ALTO;
}

static int lineas_visibles(void)
{
    int alto = juego.pant.pide_nombre ? TXT_ALTO - 70 : TXT_ALTO;
    return alto / LINEA;
}

static float scroll_maximo(void)
{
    return (float)maximo(0, n_lineas - lineas_visibles());
}

static void tocar(Vector2 p)
{
    int i, indices[MAX_VISIBLES], hay_mas, n = opciones_visibles(indices, &hay_mas);
    if (hay_musica && CheckCollisionPointRec(p, BOTON_MUSICA)) { silencio = !silencio; return; }
    for (i = 0; i < n + hay_mas; i++)
        if (CheckCollisionPointRec(p, rect_boton(i))) { activar_boton(i); return; }
}

static void escribir_nombre(void)
{
    char *nom = juego.j.nombre;
    int c, len;
    while ((c = GetCharPressed()) > 0) {
        int bytes = 0;
        const char *utf8;
        if (c < 32) continue;
        utf8 = CodepointToUTF8(c, &bytes);
        len = (int)strlen(nom);
        if (len + bytes < MAX_NOMBRE - 1) { memcpy(nom + len, utf8, bytes); nom[len + bytes] = '\0'; }
    }
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        len = (int)strlen(nom);
        while (len > 0) {
            len--;
            if ((nom[len] & 0xC0) != 0x80) break;   /* borrar un caracter UTF-8 entero */
        }
        nom[len] = '\0';
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) activar_boton(0);
}

static void actualizar(void)
{
    Vector2 p = puntero();
    int abajo = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0;
    int i;

#if defined(PLATFORM_WEB)
    /* el lienzo del navegador ocupa toda la ventana */
    {
        double cw = 0, ch = 0;
        int w, h;
        emscripten_get_element_css_size("#canvas", &cw, &ch);
        w = (int)cw;
        h = (int)ch;
        if (w > 0 && h > 0 && (w != GetScreenWidth() || h != GetScreenHeight())) SetWindowSize(w, h);
    }
#endif

    /* teclado */
    if (juego.pant.pide_nombre) {
        escribir_nombre();
    } else {
        if (IsKeyPressed(KEY_M)) silencio = !silencio;
        for (i = 0; i < 9; i++)
            if (IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i)) { activar_boton(i); break; }
    }

    /* rueda del mouse: scroll del texto */
    scroll -= GetMouseWheelMove() * 2;

    /* toque / click (y arrastrar el texto para leer lo de arriba) */
    if (abajo && !estaba_abajo) { inicio_toque = p; arrastrando = 0; scroll_inicio = scroll; }
    if (abajo) {
        if (!arrastrando && en_area_texto(inicio_toque) && fabsf(p.y - inicio_toque.y) > 12) arrastrando = 1;
        if (arrastrando) scroll = scroll_inicio - (p.y - inicio_toque.y) / LINEA;
        ultimo_toque = p;
    }
    if (!abajo && estaba_abajo && !arrastrando) tocar(ultimo_toque);
    estaba_abajo = abajo;

    /* pantalla nueva: volver a la primera pagina y al principio del texto */
    if (juego.pant.version != version_vista) {
        version_vista = juego.pant.version;
        pagina = 0;
        envolver(juego.pant.texto, TXT_W);
        scroll = 0;
    }
    if (scroll > scroll_maximo()) scroll = scroll_maximo();
    if (scroll < 0) scroll = 0;

    actualizar_musica();

    if (juego.quiere_salir) {
#if defined(PLATFORM_WEB)
        juego_nuevo(&juego);
#else
        cerrar = 1;
#endif
    }
}

/* ======================= DIBUJO ======================= */

static void dibujar_imagen(void)
{
    Pantalla *p = &juego.pant;
    Imagen *fondo = imagen(p->imagen), *enem = imagen(p->imagen_enemigo);
    if ((!fondo || !fondo->ok) && p->imagen_alt[0]) fondo = imagen(p->imagen_alt);   /* imagen del bioma */
    Rectangle zona = { 0, 0, ANCHO, IMG_ALTO };

    if (fondo && fondo->ok) dibujar_cubriendo(fondo->tex, zona);
    else texto(TextFormat("(falta assets/%s.png)", p->imagen), TXT_X, IMG_ALTO / 2.0f, 18, C_APAGADO);

    if (!enem && p->imagen_npc2[0]) {   /* cientifico, a la izquierda */
        Imagen *npc = imagen(p->imagen_npc2);
        if (npc && npc->ok) dibujar_enemigo(npc->tex, ANCHO * 0.30f, 0.80f, WHITE);
    }
    if (!enem && p->imagen_npc[0]) {    /* soldado, a la derecha */
        Imagen *npc = imagen(p->imagen_npc);
        if (npc && npc->ok) dibujar_enemigo(npc->tex, ANCHO * 0.72f, 0.95f, WHITE);
    }

    if (enem) {
        /* posicion, tamaño y brillo de los que esperan detras (del mas cercano al mas lejano) */
        static const float pos_x[MAX_COLA_VISIBLE] = { -230, 230, -420, 420 };
        static const float escala[MAX_COLA_VISIBLE] = { 0.82f, 0.74f, 0.64f, 0.58f };
        static const unsigned char brillo[MAX_COLA_VISIBLE] = { 70, 55, 42, 34 };
        int k;
        DrawRectangle(0, 0, ANCHO, IMG_ALTO, Fade(BLACK, 0.45f));   /* oscurecer el fondo en combate */
        for (k = p->n_cola - 1; k >= 0; k--) {
            Imagen *atras = imagen(p->imagen_cola[k]);
            unsigned char b = brillo[k];
            if (atras && atras->ok)
                dibujar_enemigo(atras->tex, ANCHO / 2.0f + pos_x[k], escala[k], (Color){ b, b, b, 255 });
        }
        if (enem->ok) dibujar_enemigo(enem->tex, ANCHO / 2.0f, 1.0f, WHITE);
        else texto(TextFormat("(falta assets/%s.png)", p->imagen_enemigo), ANCHO / 2.0f - 150, 200, 18, C_APAGADO);
    }

    /* la imagen se funde con el negro de abajo */
    DrawRectangleGradientV(0, IMG_ALTO - 80, ANCHO, 80, Fade(BLACK, 0), C_NEGRO);

    /* boton de musica (arriba a la derecha): se toca o se aprieta M */
    if (hay_musica) {
        Rectangle r = BOTON_MUSICA;
        DrawTextEx(fuente, "♪", (Vector2){ r.x + 16, r.y + 4 }, 34, 0, silencio ? C_APAGADO : C_BLANCO);
        if (silencio) DrawLineEx((Vector2){ r.x + 10, r.y + r.height - 8 }, (Vector2){ r.x + r.width - 10, r.y + 8 }, 3, C_BLANCO);
    }
}

static void dibujar_texto(void)
{
    int visibles = lineas_visibles(), i, desde = (int)(scroll + 0.5f);
    char titulo[128];

    recortar(juego.pant.titulo, TXT_W, TAM_TITULO, titulo, sizeof titulo);
    DrawTextEx(fuente_negrita, titulo, (Vector2){ TXT_X, TITULO_Y }, TAM_TITULO, 0, C_BLANCO);

    for (i = 0; i < visibles && desde + i < n_lineas; i++)
        texto(lineas[desde + i], TXT_X, TXT_Y + i * LINEA, TAM_TEXTO, C_BLANCO);

    /* indicador de que hay mas texto arriba/abajo */
    if (n_lineas > visibles) {
        float total = (float)n_lineas, h = TXT_ALTO * visibles / total;
        float y = TXT_Y + (TXT_ALTO - h) * (scroll / fmaxf(1, scroll_maximo()));
        DrawRectangle(TXT_X + TXT_W + 14, (int)y, 2, (int)h, C_APAGADO);
        if (scroll < scroll_maximo() - 0.5f)
            texto("▼ más abajo", TXT_X + TXT_W - 100, ALTO - 26, 16, C_BLANCO);
    }

    if (juego.pant.pide_nombre) {
        float y = TXT_Y + TXT_ALTO - 44;
        int parpadeo = ((int)(GetTime() * 2)) % 2;
        texto(TextFormat("%s%s", juego.j.nombre, parpadeo ? "_" : ""), TXT_X, y, 24, C_BLANCO);
        DrawRectangle(TXT_X, (int)y + 34, 380, 1, C_BLANCO);
    }
}

static void dibujar_opciones(void)
{
    int indices[MAX_VISIBLES], hay_mas, n = opciones_visibles(indices, &hay_mas), i;
    Vector2 m = a_lienzo(GetMousePosition());
    char buf[160];

    for (i = 0; i < n + hay_mas; i++) {
        Rectangle r = rect_boton(i);
        int es_mas = (i == n);
        int off = !es_mas && juego.pant.op[indices[i]].deshabilitada;
        int hover = !off && CheckCollisionPointRec(m, r);
        Color letra = off ? C_APAGADO : (hover ? C_NEGRO : C_BLANCO);

        if (hover) DrawRectangleRec(r, C_BLANCO);   /* resaltado: se invierte */
        texto(TextFormat("%d", i + 1), r.x + 12, r.y + 6, TAM_OPCION, letra);
        if (es_mas) snprintf(buf, sizeof buf, "Más (%d/%d)", pagina + 1, total_paginas());
        else recortar(juego.pant.op[indices[i]].texto, r.width - 56, TAM_OPCION, buf, sizeof buf);
        texto(buf, r.x + 44, r.y + 6, TAM_OPCION, letra);
    }
}

/* Bordes rojos que aparecen al perder vida y laten cuando queda poca */
static void dibujar_heridas(void)
{
    Jugador *j = &juego.j;
    float frac, fuerza, ancho;
    unsigned char alfa;
    Color rojo, nada = { 150, 0, 0, 0 };
    if (juego.estado == E_NOMBRE || juego.estado == E_CLASE || juego.estado == E_RADIO ||
        juego.estado == E_FINAL || j->vida_max <= 0) return;
    frac = (float)j->vida / j->vida_max;
    if (frac >= 0.7f) return;
    fuerza = (0.7f - frac) / 0.7f;                               /* 0 = sano, 1 = casi muerto */
    if (frac < 0.3f) fuerza *= 0.8f + 0.2f * sinf((float)GetTime() * 5);   /* late */
    ancho = 60 + 220 * fuerza;
    alfa = (unsigned char)(200 * fuerza);
    rojo = (Color){ 150, 0, 0, alfa };
    DrawRectangleGradientH(0, 0, (int)ancho, ALTO, rojo, nada);
    DrawRectangleGradientH(ANCHO - (int)ancho, 0, (int)ancho, ALTO, nada, rojo);
    DrawRectangleGradientV(0, 0, ANCHO, (int)(ancho * 0.6f), rojo, nada);
    DrawRectangleGradientV(0, ALTO - (int)(ancho * 0.6f), ANCHO, (int)(ancho * 0.6f), nada, rojo);
}

static void dibujar(void)
{
    float s;
    Vector2 o = escala_y_offset(&s);

    BeginTextureMode(lienzo);
    /* mezclar el color normalmente pero dejar el lienzo opaco */
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    ClearBackground(C_NEGRO);
    dibujar_imagen();
    dibujar_texto();
    dibujar_opciones();
    dibujar_heridas();
    EndBlendMode();
    EndTextureMode();

    BeginDrawing();
    ClearBackground(C_NEGRO);
    DrawTexturePro(lienzo.texture, (Rectangle){ 0, 0, ANCHO, -ALTO },
                   (Rectangle){ o.x, o.y, ANCHO * s, ALTO * s }, (Vector2){ 0, 0 }, 0, WHITE);
    EndDrawing();
}

static void cuadro(void)
{
    actualizar();
    dibujar();
}

/* ======================= FUENTES ======================= */

static Font cargar_fuente(const char *ruta)
{
    /* ASCII + acentos, eñe y simbolos que usa el juego */
    static const char *extra = "áéíóúÁÉÍÓÚñÑüÜ¿¡°·—–…▶◀✔∞“”«»›♪▼▲";
    int n_extra = 0, i, total;
    int *cp_extra = LoadCodepoints(extra, &n_extra);
    int *cps = (int *)malloc(sizeof(int) * (95 + n_extra));
    Font f;
    for (i = 0; i < 95; i++) cps[i] = 32 + i;
    for (i = 0; i < n_extra; i++) cps[95 + i] = cp_extra[i];
    total = 95 + n_extra;
    UnloadCodepoints(cp_extra);
    if (FileExists(ruta)) f = LoadFontEx(ruta, 48, cps, total);
    else f = GetFontDefault();
    free(cps);
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    return f;
}

/* ======================= MAIN ======================= */

int main(void)
{
    srand((unsigned)time(NULL));

#if defined(PLATFORM_WEB)
    InitWindow(ANCHO, ALTO, "Escape Corpus Christi");
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(ANCHO, ALTO, "Escape Corpus Christi");
    SetWindowMinSize(640, 360);
    ChangeDirectory(GetApplicationDirectory());   /* para encontrar la carpeta assets */
#endif
    SetExitKey(KEY_NULL);

    fuente = cargar_fuente("assets/fuente.ttf");
    fuente_negrita = cargar_fuente("assets/fuente_negrita.ttf");
    lienzo = LoadRenderTexture(ANCHO, ALTO);
    SetTextureFilter(lienzo.texture, TEXTURE_FILTER_BILINEAR);

    cargar_musica();
    juego_nuevo(&juego);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(cuadro, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose() && !cerrar) cuadro();
#endif

    descargar_musica();
    UnloadRenderTexture(lienzo);
    CloseWindow();
    return 0;
}
