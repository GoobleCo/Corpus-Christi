/*
 * MODULO INTERFAZ (raylib)
 * Dibuja la pantalla como una novela visual:
 *
 *   +--------------------------------------------------+
 *   |  IMAGEN del bloque (+ enemigo encima)       HUD  |
 *   +-----------------------------+--------------------+
 *   |  DESCRIPCION / lo que pasó  |  OPCIONES 1..8     |
 *   +-----------------------------+--------------------+
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
#define IMG_ALTO     400
#define TXT_X        28
#define TXT_Y        418
#define TXT_W        730
#define TXT_ALTO     288
#define OPC_X        790
#define OPC_Y        416
#define OPC_W        466
#define OPC_H        32
#define OPC_SEP      4
#define MAX_VISIBLES 8
#define TAM_TEXTO    20
#define LINEA        25
#define TAM_OPCION   19

/* Colores */
#define C_FONDO      (Color){ 14, 14, 18, 255 }
#define C_PANEL      (Color){ 20, 20, 26, 255 }
#define C_TEXTO      (Color){ 226, 226, 232, 255 }
#define C_TENUE      (Color){ 140, 140, 152, 255 }
#define C_ACENTO     (Color){ 214, 64, 52, 255 }
#define C_BOTON      (Color){ 34, 34, 44, 255 }
#define C_BOTON_HOV  (Color){ 54, 54, 70, 255 }
#define C_BOTON_OFF  (Color){ 24, 24, 30, 255 }
#define C_VIDA       (Color){ 206, 58, 58, 255 }
#define C_INFEC      (Color){ 120, 196, 70, 255 }

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

/* Dibuja la imagen entera dentro de un alto maximo, apoyada en el piso */
static void dibujar_enemigo(Texture2D t)
{
    float alto_max = 350, ancho_max = 520;
    float s = fminf(alto_max / t.height, ancho_max / t.width);
    float w = t.width * s, h = t.height * s;
    Rectangle dst = { ANCHO / 2.0f - w / 2, IMG_ALTO - h - 6, w, h };
    DrawEllipse(ANCHO / 2, IMG_ALTO - 10, w * 0.35f, 12, Fade(BLACK, 0.5f));
    DrawTexturePro(t, (Rectangle){ 0, 0, (float)t.width, (float)t.height }, dst, (Vector2){ 0, 0 }, 0, WHITE);
}

/* ======================= TEXTO ======================= */

static void texto(const char *s, float x, float y, float tam, Color c)
{
    DrawTextEx(fuente, s, (Vector2){ x, y }, tam, 0, c);
}

static void texto_negrita(const char *s, float x, float y, float tam, Color c)
{
    DrawTextEx(fuente_negrita, s, (Vector2){ x + 2, y + 2 }, tam, 0, Fade(BLACK, 0.7f));
    DrawTextEx(fuente_negrita, s, (Vector2){ x, y }, tam, 0, c);
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

    /* pantalla nueva: volver a la primera pagina y mostrar el final del texto */
    if (juego.pant.version != version_vista) {
        version_vista = juego.pant.version;
        pagina = 0;
        envolver(juego.pant.texto, TXT_W);
        scroll = scroll_maximo();
    }
    if (scroll > scroll_maximo()) scroll = scroll_maximo();
    if (scroll < 0) scroll = 0;

    if (juego.quiere_salir) {
#if defined(PLATFORM_WEB)
        juego_nuevo(&juego);
#else
        cerrar = 1;
#endif
    }
}

/* ======================= DIBUJO ======================= */

static void dibujar_barra(float x, float y, float w, float valor, float max, Color c, const char *etiqueta)
{
    char buf[48];
    float frac = max > 0 ? valor / max : 0;
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    DrawRectangleRounded((Rectangle){ x, y, w, 20 }, 0.4f, 6, Fade(BLACK, 0.6f));
    if (frac > 0) DrawRectangleRounded((Rectangle){ x, y, w * frac, 20 }, 0.4f, 6, c);
    snprintf(buf, sizeof buf, "%s", etiqueta);
    texto(buf, x + 8, y + 1, 17, WHITE);
}

static void dibujar_hud(void)
{
    Jugador *j = &juego.j;
    char buf[96];
    float x = ANCHO - 330, y = 14;
    DrawRectangleRounded((Rectangle){ x - 12, y - 8, 330, 104 }, 0.15f, 6, Fade(BLACK, 0.55f));
    snprintf(buf, sizeof buf, "Vida %d/%d", j->vida, j->vida_max);
    dibujar_barra(x, y, 306, (float)j->vida, (float)j->vida_max, C_VIDA, buf);
    snprintf(buf, sizeof buf, "Infección %d%%", j->infeccion);
    dibujar_barra(x, y + 26, 306, (float)j->infeccion, 100, C_INFEC, buf);
    if (j->turno <= LIMITE_HELICOPTERO) snprintf(buf, sizeof buf, "Turno %d / %d", j->turno, LIMITE_HELICOPTERO);
    else snprintf(buf, sizeof buf, "Turno %d (el helicóptero se fue)", j->turno);
    texto(buf, x, y + 52, 17, C_TEXTO);
    {
        char arma[96];
        snprintf(arma, sizeof arma, "Arma: %s", nombre_arma_equipada(j));
        recortar(arma, 306, 17, buf, sizeof buf);
        texto(buf, x, y + 72, 17, C_TENUE);
    }
}

static void dibujar_imagen(void)
{
    Pantalla *p = &juego.pant;
    Imagen *fondo = imagen(p->imagen), *enem = imagen(p->imagen_enemigo);
    Rectangle zona = { 0, 0, ANCHO, IMG_ALTO };

    if (fondo && fondo->ok) {
        dibujar_cubriendo(fondo->tex, zona);
    } else {
        DrawRectangleGradientV(0, 0, ANCHO, IMG_ALTO, (Color){ 40, 40, 52, 255 }, (Color){ 12, 12, 16, 255 });
        texto(TextFormat("(falta assets/%s.png)", p->imagen), 40, IMG_ALTO / 2.0f, 22, C_TENUE);
    }

    if (enem) {
        DrawRectangle(0, 0, ANCHO, IMG_ALTO, Fade(BLACK, 0.35f));   /* oscurecer el fondo en combate */
        if (enem->ok) dibujar_enemigo(enem->tex);
        else texto(TextFormat("(falta assets/%s.png)", p->imagen_enemigo), ANCHO / 2.0f - 150, 200, 22, C_TENUE);
    }

    /* degradados para que se lea el titulo y la imagen se funda con el panel */
    DrawRectangleGradientV(0, 0, ANCHO, 110, Fade(BLACK, 0.8f), Fade(BLACK, 0));
    DrawRectangleGradientV(0, IMG_ALTO - 60, ANCHO, 60, Fade(C_PANEL, 0), C_PANEL);

    {
        char titulo[128];
        recortar(p->titulo, p->mostrar_hud ? ANCHO - 400 : ANCHO - 60, 34, titulo, sizeof titulo);
        texto_negrita(titulo, 30, 22, 34, WHITE);
    }
    if (p->mostrar_hud) dibujar_hud();
}

static void dibujar_texto(void)
{
    int visibles = lineas_visibles(), i, desde = (int)(scroll + 0.5f);
    for (i = 0; i < visibles && desde + i < n_lineas; i++) {
        const char *l = lineas[desde + i];
        Color c = C_TEXTO;
        if (l[0] == ' ' || strncmp(l, "▶", 3) == 0) c = (Color){ 236, 170, 120, 255 };   /* lo que hacen los enemigos */
        texto(l, TXT_X, TXT_Y + i * LINEA, TAM_TEXTO, c);
    }
    /* indicador de que hay mas texto arriba/abajo */
    if (n_lineas > visibles) {
        float total = (float)n_lineas, h = TXT_ALTO * visibles / total;
        float y = TXT_Y + (TXT_ALTO - h) * (scroll / fmaxf(1, scroll_maximo()));
        DrawRectangleRounded((Rectangle){ TXT_X + TXT_W + 12, y, 4, h }, 1, 4, Fade(C_TENUE, 0.6f));
    }

    if (juego.pant.pide_nombre) {
        Rectangle caja = { TXT_X, TXT_Y + TXT_ALTO - 56, 420, 46 };
        int parpadeo = ((int)(GetTime() * 2)) % 2;
        DrawRectangleRounded(caja, 0.2f, 6, C_BOTON);
        DrawRectangleRoundedLinesEx(caja, 0.2f, 6, 2, C_ACENTO);
        texto(TextFormat("%s%s", juego.j.nombre, parpadeo ? "_" : ""), caja.x + 14, caja.y + 11, 24, WHITE);
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
        Color fondo = off ? C_BOTON_OFF : (hover ? C_BOTON_HOV : C_BOTON);
        Color letra = off ? (Color){ 92, 92, 102, 255 } : C_TEXTO;

        DrawRectangleRounded(r, 0.25f, 6, fondo);
        if (hover) DrawRectangle((int)r.x, (int)r.y + 4, 4, (int)r.height - 8, C_ACENTO);
        texto(TextFormat("%d", i + 1), r.x + 14, r.y + 6, TAM_OPCION, off ? letra : C_ACENTO);
        if (es_mas) snprintf(buf, sizeof buf, "Más opciones ▶ (%d/%d)", pagina + 1, total_paginas());
        else recortar(juego.pant.op[indices[i]].texto, r.width - 52, TAM_OPCION, buf, sizeof buf);
        texto(buf, r.x + 40, r.y + 6, TAM_OPCION, es_mas ? C_TENUE : letra);
    }
}

static void dibujar(void)
{
    float s;
    Vector2 o = escala_y_offset(&s);

    BeginTextureMode(lienzo);
    /* mezclar el color normalmente pero dejar el lienzo opaco */
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    ClearBackground(C_PANEL);
    dibujar_imagen();
    DrawLine(OPC_X - 16, IMG_ALTO + 14, OPC_X - 16, ALTO - 14, (Color){ 44, 44, 54, 255 });
    dibujar_texto();
    dibujar_opciones();
    EndBlendMode();
    EndTextureMode();

    BeginDrawing();
    ClearBackground(C_FONDO);
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
    static const char *extra = "áéíóúÁÉÍÓÚñÑüÜ¿¡°·—–…▶◀✔∞“”";
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
    InitWindow(ANCHO, ALTO, "Escape de la Ciudad");
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(ANCHO, ALTO, "Escape de la Ciudad");
    SetWindowMinSize(640, 360);
    ChangeDirectory(GetApplicationDirectory());   /* para encontrar la carpeta assets */
#endif
    SetExitKey(KEY_NULL);

    fuente = cargar_fuente("assets/fuente.ttf");
    fuente_negrita = cargar_fuente("assets/fuente_negrita.ttf");
    lienzo = LoadRenderTexture(ANCHO, ALTO);
    SetTextureFilter(lienzo.texture, TEXTURE_FILTER_BILINEAR);

    juego_nuevo(&juego);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(cuadro, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose() && !cerrar) cuadro();
#endif

    UnloadRenderTexture(lienzo);
    CloseWindow();
    return 0;
}
