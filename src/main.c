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
 *
 * Animaciones y sonidos: la logica deja en juego.fx lo que acaba de pasar
 * (ver EFECTOS en juego.h) y aca se reproduce en orden: flash, temblor de
 * pantalla, el enemigo que se sacude, cae o aparece, y un sonido para cada cosa.
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

#ifndef VERSION_JUEGO
#define VERSION_JUEGO "dev"   /* la compilacion de GitHub pone aca el numero de version */
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
static void dibujar_enemigo_mov(Texture2D t, float centro_x, float escala, Color tinte, float dx, float dy)
{
    float alto_max = (IMG_ALTO - 14) * escala, ancho_max = 560 * escala;
    float s = fminf(alto_max / t.height, ancho_max / t.width);
    float w = t.width * s, h = t.height * s;
    Rectangle dst = { centro_x - w / 2 + dx, IMG_ALTO - h + dy, w, h };
    DrawTexturePro(t, (Rectangle){ 0, 0, (float)t.width, (float)t.height }, dst, (Vector2){ 0, 0 }, 0, tinte);
}

static void dibujar_enemigo(Texture2D t, float centro_x, float escala, Color tinte)
{
    dibujar_enemigo_mov(t, centro_x, escala, tinte, 0, 0);
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

/* ======================= SONIDOS =======================
 * Se generan con codigo al arrancar (no hace falta ningun archivo).
 * Para usar uno propio, poner assets/sonidos/<nombre>.wav (o .ogg) con el
 * nombre de la tabla NOMBRE_SONIDO: si existe, se usa ese en vez del generado. */

enum {
    S_GOLPE, S_DISPARO, S_DISPARO_LEJOS, S_WHOOSH, S_CLICK, S_ROTA, S_CAE,
    S_APARECE, S_REFUERZO, S_EMBESTIDA, S_DANIO, S_INFECCION, S_CURA, S_LOOT,
    S_NADA, S_ESCAPE, S_PASOS, S_FIN_COMBATE, S_FINAL_MALO, S_FINAL_BUENO,
    NUM_SONIDOS
};
static const char *NOMBRE_SONIDO[NUM_SONIDOS] = {
    "golpe", "disparo", "disparo_enemigo", "esquivar", "click", "arma_rota", "cae",
    "aparece", "refuerzo", "embestida", "danio", "infeccion", "cura", "loot",
    "nada", "escape", "pasos", "fin_combate", "final_malo", "final_bueno",
};
/* volumen de cada sonido generado (pico, de 0 a 1) */
static const float VOLUMEN_SONIDO[NUM_SONIDOS] = {
    0.80f, 0.95f, 0.60f, 0.45f, 0.50f, 0.60f, 0.75f,
    0.85f, 0.45f, 0.50f, 0.90f, 0.40f, 0.35f, 0.40f,
    0.30f, 0.55f, 0.35f, 0.25f, 0.80f, 0.45f,
};
#define VOLUMEN_SONIDOS 0.8f
#define SR 22050                 /* frecuencia de muestreo de los sonidos generados */

static Sound sonido[NUM_SONIDOS];
static int sonido_ok[NUM_SONIDOS];
static int hay_sonidos = 0;

static unsigned int semilla_ruido = 12345;
static float ruido(void)
{
    semilla_ruido = semilla_ruido * 1664525u + 1013904223u;
    return (float)(semilla_ruido >> 8) / 8388608.0f - 1.0f;
}
static float seno(float fase) { return sinf(fase * 6.2831853f); }
static float sierra(float fase) { return 2.0f * (fase - floorf(fase)) - 1.0f; }

/* Golpe sordo: un tono grave que cae + un poco de ruido. Se usa para varios sonidos. */
static float golpe_sordo(float t, float f0, float f1, float caida, float *fase)
{
    float f, e;
    if (t < 0) return 0;
    f = f1 + (f0 - f1) * expf(-t * 18);
    *fase += f / SR;
    e = expf(-t * caida);
    return seno(*fase) * e;
}

static Sound generar_sonido(int cual)
{
    static float m[SR * 2];
    float dur, lp = 0, lp2 = 0, fase = 0, fase2 = 0, fase3 = 0, fase4 = 0, pico = 0, ganancia;
    int n, i;
    Wave w;
    short *datos;
    Sound snd;
    switch (cual) {
    case S_DISPARO: case S_DISPARO_LEJOS: dur = 0.6f; break;
    case S_APARECE:     dur = 1.1f; break;
    case S_REFUERZO:    dur = 0.9f; break;
    case S_INFECCION:   dur = 0.8f; break;
    case S_CURA:        dur = 0.7f; break;
    case S_ESCAPE:      dur = 0.8f; break;
    case S_PASOS:       dur = 0.5f; break;
    case S_FIN_COMBATE: dur = 0.9f; break;
    case S_FINAL_MALO:  dur = 1.8f; break;
    case S_FINAL_BUENO: dur = 1.8f; break;
    case S_CAE:         dur = 0.6f; break;
    default:            dur = 0.35f; break;
    }
    n = (int)(dur * SR);
    for (i = 0; i < n; i++) {
        float t = (float)i / SR, x = 0, r = ruido(), k = t / dur;
        switch (cual) {
        case S_GOLPE:      /* puñetazo / cuchillazo: golpe seco */
            lp += (r - lp) * 0.25f;
            x = golpe_sordo(t, 160, 55, 22, &fase) * 0.9f + lp * expf(-t * 70) * 0.9f;
            break;
        case S_DISPARO: case S_DISPARO_LEJOS: {   /* estampido + eco */
            float corte = (cual == S_DISPARO ? 0.85f : 0.35f) * expf(-t * 9) + 0.04f;
            lp += (r - lp) * corte;
            x = lp * (t < 0.002f ? t / 0.002f : expf(-t * 7)) * 1.4f
              + golpe_sordo(t, 120, 40, 9, &fase) * 0.9f;
            x = tanhf(x * 1.8f) * (cual == S_DISPARO ? 1.0f : 0.6f);
            break;
        }
        case S_WHOOSH:     /* algo pasa rapido al lado tuyo */
            lp += (r - lp) * (0.08f + 0.25f * k);
            lp2 += (lp - lp2) * 0.04f;
            x = (lp - lp2) * powf(sinf(3.14159f * k), 2) * 2.2f;
            break;
        case S_CLICK:      /* arma sin balas */
            x = (r * expf(-t * 500) + seno(t * 2400) * expf(-t * 300)) * 0.7f
              + (t > 0.07f ? seno(t * 1800) * expf(-(t - 0.07f) * 400) * 0.5f : 0);
            break;
        case S_ROTA:       /* crujido: chasquidos agudos */
            lp += (r - lp) * 0.5f;
            x = (r - lp) * (ruido() > 0.6f ? 1.0f : 0.25f) * expf(-t * 12) * 1.5f;
            break;
        case S_CAE:        /* cuerpo que cae: dos golpes graves */
            lp += (r - lp) * 0.06f;
            x = golpe_sordo(t, 100, 35, 9, &fase) * 0.9f + lp * expf(-t * 10) * 1.2f
              + golpe_sordo(t - 0.2f, 80, 30, 14, &fase2) * 0.5f;
            break;
        case S_APARECE:    /* golpe de tension: graves desafinados que crecen */
            lp += ((sierra(fase) + sierra(fase2) + sierra(fase3)) / 3 - lp) * 0.035f;
            fase += 55.0f / SR; fase2 += 58.3f / SR; fase3 += 82.6f / SR;
            lp2 += (r - lp2) * 0.02f;
            x = (lp * 2.2f + lp2 * 1.5f) * (t < 0.04f ? t / 0.04f : expf(-(t - 0.04f) * 2.6f))
              + golpe_sordo(t, 70, 30, 6, &fase4) * 0.7f;
            break;
        case S_REFUERZO:   /* gruñido a lo lejos */
            fase += (72 + 6 * seno(t * 7)) / SR;
            lp += (sierra(fase) - lp) * 0.03f;
            x = lp * sinf(3.14159f * k) * 1.6f;
            break;
        case S_EMBESTIDA:  /* se te tira encima: gruñido corto */
            fase += (110 - 40 * k) / SR;
            lp += (sierra(fase) * 0.7f + r * 0.3f - lp) * 0.08f;
            x = lp * sinf(3.14159f * k) * 1.8f;
            break;
        case S_DANIO:      /* te lastiman: golpe pesado y sucio */
            lp += (r - lp) * 0.35f;
            x = golpe_sordo(t, 130, 38, 11, &fase) * 1.1f + lp * expf(-t * 28) * 1.1f;
            x = tanhf(x * 2.2f) * 0.9f;
            break;
        case S_INFECCION:  /* tono enfermo que baja y vibra */
            fase += 620 * (1 - 0.35f * k) * (1 + 0.012f * seno(t * 9)) / SR;
            fase2 += 655 * (1 - 0.35f * k) / SR;
            x = (seno(fase) + seno(fase2)) * 0.22f * (t < 0.08f ? t / 0.08f : 1) * (1 - k);
            break;
        case S_CURA: {     /* tres notas suaves */
            static const float notas[3] = { 523.3f, 659.3f, 784.0f };
            int q;
            for (q = 0; q < 3; q++) {
                float tq = t - q * 0.09f;
                if (tq > 0) x += seno(tq * notas[q]) * expf(-tq * 5) * 0.22f;
            }
            break;
        }
        case S_LOOT:       /* encontrar algo: dos notas rapidas */
            if (t < 0.07f)       x = seno(t * 660) * 0.35f;
            else if (t < 0.22f)  x = seno(t * 990) * 0.35f * expf(-(t - 0.07f) * 18);
            lp += (r - lp) * 0.3f;
            x += lp * expf(-t * 40) * 0.3f;
            break;
        case S_NADA:       /* no hay nada */
            x = seno(t * 196) * 0.35f * expf(-t * 14);
            break;
        case S_ESCAPE: {   /* pasos corriendo */
            int q;
            lp += (r - lp) * 0.2f;
            for (q = 0; q < 5; q++) {
                float tq = t - q * 0.13f;
                if (tq > 0 && tq < 0.1f) x += (seno(tq * 90) + lp) * expf(-tq * 45) * (1 - q * 0.15f);
            }
            break;
        }
        case S_PASOS: {    /* dos pasos */
            int q;
            lp += (r - lp) * 0.15f;
            for (q = 0; q < 2; q++) {
                float tq = t - q * 0.24f;
                if (tq > 0 && tq < 0.12f) x += (seno(tq * 80) * 0.6f + lp) * expf(-tq * 40) * 0.6f;
            }
            break;
        }
        case S_FIN_COMBATE: /* alivio: un tono que baja suave */
            fase += (330 - 110 * k) / SR;
            x = seno(fase) * 0.2f * sinf(3.14159f * k);
            break;
        case S_FINAL_MALO: /* todo se apaga */
            fase += (110 - 75 * k) / SR;
            lp += (sierra(fase) - lp) * 0.04f;
            lp2 += (r - lp2) * 0.02f;
            x = (lp * 1.8f + lp2) * (t < 0.03f ? t / 0.03f : (1 - k));
            break;
        case S_FINAL_BUENO: { /* acorde que crece y se va */
            static const float notas[4] = { 261.6f, 329.6f, 392.0f, 523.3f };
            int q;
            for (q = 0; q < 4; q++) x += seno(t * notas[q]) * 0.12f;
            x *= sinf(3.14159f * k);
            break;
        }
        }
        m[i] = x;
        if (fabsf(x) > pico) pico = fabsf(x);
    }
    ganancia = pico > 0 ? VOLUMEN_SONIDO[cual] / pico : 0;   /* todos al volumen de la tabla */
    datos = (short *)malloc(sizeof(short) * n);
    for (i = 0; i < n; i++) {
        float v = fmaxf(-1, fminf(1, m[i] * ganancia));
        if (i > n - 200) v *= (float)(n - i) / 200;   /* sin chasquido al final */
        datos[i] = (short)(v * 30000);
    }
    w.frameCount = (unsigned int)n;
    w.sampleRate = SR;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = datos;
    snd = LoadSoundFromWave(w);
    free(datos);
    return snd;
}

static void cargar_sonidos(void)
{
    int i;
    if (!IsAudioDeviceReady()) return;
    for (i = 0; i < NUM_SONIDOS; i++) {
        char wav[96], ogg[96];
        snprintf(wav, sizeof wav, "assets/sonidos/%s.wav", NOMBRE_SONIDO[i]);
        snprintf(ogg, sizeof ogg, "assets/sonidos/%s.ogg", NOMBRE_SONIDO[i]);
        if (FileExists(wav))      sonido[i] = LoadSound(wav);
        else if (FileExists(ogg)) sonido[i] = LoadSound(ogg);
        else                      sonido[i] = generar_sonido(i);
        sonido_ok[i] = IsSoundValid(sonido[i]);
        if (sonido_ok[i]) { SetSoundVolume(sonido[i], VOLUMEN_SONIDOS); hay_sonidos = 1; }
    }
}

static void sonar(int cual)
{
    if (silencio || cual < 0 || !sonido_ok[cual]) return;
    SetSoundPitch(sonido[cual], 0.92f + 0.16f * (float)rand() / RAND_MAX);   /* que no suene siempre igual */
    PlaySound(sonido[cual]);
}

static void descargar_sonidos(void)
{
    int i;
    for (i = 0; i < NUM_SONIDOS; i++) if (sonido_ok[i]) UnloadSound(sonido[i]);
}

/* ======================= ANIMACIONES =======================
 * Todo es simple: flashes de color, temblor de pantalla, y el enemigo que
 * se sacude, se te tira encima, cae o aparece. Los efectos de una misma
 * jugada se reproducen uno detras del otro (ver PAUSA_FX). */

static int sin_animaciones = 0;    /* las capturas de prueba las apagan */
static double ahora = 0;

typedef struct { Efecto e; double cuando; int hecho; } FxPendiente;
static FxPendiente pendientes[MAX_EFECTOS];
static int n_pendientes = 0;

/* estado de las animaciones (tiempos de inicio; -100 = nunca) */
static double temblor_t = -100;  static float temblor_fuerza = 0;
static double flash_t = -100;    static float flash_dur = 0.2f, flash_alfa = 0; static Color flash_color;
static double golpeado_t = -100;   /* el enemigo recibe un golpe */
static double esquiva_t = -100;    /* el enemigo se corre */
static double embestida_t = -100;  /* el enemigo se te tira encima */
static double enemigo_desde = -100;/* el enemigo actual aparece desde este momento */
static double cola_desde = -100;
static double fondo_t = -100;    static float fondo_dur = 0.45f;
static double texto_t = -100;
static char fondo_visto[64] = "", enemigo_visto[64] = "";
static char fantasma[64] = "";   /* el enemigo que cae o huye */
static double fantasma_t = -100; static int fantasma_huye = 0;
static float oscuridad = 0;      /* oscurecer el fondo en combate (sube y baja suave) */

/* 0 -> 1 a medida que pasa la animacion (1 = termino o no empezo nunca) */
static float progreso(double inicio, float dur)
{
    float k;
    if (sin_animaciones) return 1;
    k = (float)((ahora - inicio) / dur);
    return k < 0 ? 0 : (k > 1 ? 1 : k);
}
static int activa(double inicio, float dur)
{
    return !sin_animaciones && ahora >= inicio && ahora < inicio + dur;
}

static void temblar(float fuerza)
{
    float resto = activa(temblor_t, 0.4f) ? temblor_fuerza * (1 - progreso(temblor_t, 0.4f)) : 0;
    temblor_t = ahora;
    temblor_fuerza = fmaxf(fuerza, resto);
}

static void destello(Color c, float alfa, float dur)
{
    flash_t = ahora; flash_color = c; flash_alfa = alfa; flash_dur = dur;
}

/* Cuanto esperar despues de un efecto antes de mostrar el siguiente */
static float pausa_fx(TipoEfecto t)
{
    switch (t) {
    case FX_ATAQUE_ENEMIGO: return 0.16f;   /* el golpe llega enseguida */
    case FX_PASOS:          return 0.25f;
    case FX_LOOT:           return 0.15f;
    case FX_DANIO:          return 0.30f;
    default:                return 0.34f;
    }
}

static void disparar(const Efecto *e)
{
    switch (e->tipo) {
    case FX_GOLPE:
        sonar(S_GOLPE); golpeado_t = ahora; temblar(5);
        destello((Color){ 255, 255, 255, 255 }, 0.12f, 0.10f);
        break;
    case FX_DISPARO:
        sonar(S_DISPARO); golpeado_t = ahora; temblar(11);
        destello((Color){ 255, 240, 200, 255 }, 0.55f, 0.14f);
        break;
    case FX_FALLO:          sonar(S_WHOOSH); esquiva_t = ahora; break;
    case FX_SIN_BALAS:      sonar(S_CLICK); break;
    case FX_ARMA_ROTA:
        sonar(S_ROTA); temblar(4);
        destello((Color){ 255, 255, 255, 255 }, 0.25f, 0.15f);
        break;
    case FX_ENEMIGO_CAE:    sonar(S_CAE); fantasma_t = ahora; fantasma_huye = 0; temblar(6); break;
    case FX_ENEMIGO_HUYE:   sonar(S_WHOOSH); fantasma_t = ahora; fantasma_huye = 1; break;
    case FX_APARECE:        sonar(S_APARECE); temblar(3); break;
    case FX_REFUERZO:       sonar(S_REFUERZO); temblar(2); break;
    case FX_ATAQUE_ENEMIGO: sonar(S_EMBESTIDA); embestida_t = ahora; break;
    case FX_DISPARO_ENEMIGO:
        sonar(S_DISPARO_LEJOS);
        destello((Color){ 255, 230, 180, 255 }, 0.45f, 0.12f);
        break;
    case FX_DANIO: {
        float pct = (float)e->valor;
        sonar(S_DANIO);
        temblar(fminf(26, 8 + pct * 0.7f));
        destello((Color){ 200, 0, 0, 255 }, fminf(0.75f, 0.3f + pct * 0.015f), 0.35f);
        break;
    }
    case FX_NO_TE_ALCANZA:  sonar(S_WHOOSH); break;
    case FX_INFECCION:
        sonar(S_INFECCION);
        destello((Color){ 70, 160, 40, 255 }, 0.35f, 0.7f);
        break;
    case FX_CURA:
        sonar(S_CURA);
        destello((Color){ 210, 255, 225, 255 }, 0.18f, 0.6f);
        break;
    case FX_LOOT:           sonar(S_LOOT); break;
    case FX_NADA:           sonar(S_NADA); break;
    case FX_ESCAPE:         sonar(S_ESCAPE); temblar(3); break;
    case FX_ESCAPE_FALLA:   sonar(S_WHOOSH); temblar(5); break;
    case FX_PASOS:          sonar(S_PASOS); break;
    case FX_FIN_COMBATE:    sonar(S_FIN_COMBATE); break;
    case FX_FINAL_MALO:
        sonar(S_FINAL_MALO); temblar(16);
        destello((Color){ 150, 0, 0, 255 }, 0.85f, 1.2f);
        break;
    case FX_FINAL_BUENO:
        sonar(S_FINAL_BUENO);
        destello((Color){ 255, 255, 255, 255 }, 0.6f, 1.4f);
        break;
    default: break;
    }
}

/* Pantalla nueva: preparar los efectos de lo que acaba de pasar */
static void nuevos_efectos(void)
{
    Pantalla *p = &juego.pant;
    double t = ahora, cae_t = -1, aparece_t = -1, final_t = -1;
    int i;

    n_pendientes = 0;
    for (i = 0; i < juego.n_fx; i++) {
        FxPendiente *f = &pendientes[n_pendientes++];
        f->e = juego.fx[i];
        f->cuando = t;
        f->hecho = 0;
        if ((f->e.tipo == FX_ENEMIGO_CAE || f->e.tipo == FX_ENEMIGO_HUYE) && cae_t < 0) {
            cae_t = t;
            /* el que cae se sigue viendo (quieto) hasta que le toca caer */
            snprintf(fantasma, sizeof fantasma, "%s", f->e.imagen);
            fantasma_t = t;
            fantasma_huye = f->e.tipo == FX_ENEMIGO_HUYE;
        }
        if (f->e.tipo == FX_APARECE && aparece_t < 0) aparece_t = t;
        if (f->e.tipo == FX_FINAL_MALO || f->e.tipo == FX_FINAL_BUENO) final_t = t;
        t += pausa_fx(f->e.tipo);
    }
    juego.n_fx = 0;   /* ya los tomamos */

    /* escapaste (o termino el combate de otra forma): el enemigo se desvanece en vez de cortarse */
    if (cae_t < 0 && enemigo_visto[0] && !p->imagen_enemigo[0] && juego.estado != E_FINAL) {
        snprintf(fantasma, sizeof fantasma, "%s", enemigo_visto);
        fantasma_t = ahora;
        fantasma_huye = 1;
    }

    /* cuando se empieza a ver el enemigo actual */
    if (strcmp(p->imagen_enemigo, enemigo_visto) != 0 || aparece_t >= 0 || cae_t >= 0) {
        if (cae_t >= 0)          enemigo_desde = cae_t + 0.45;
        else if (aparece_t >= 0) enemigo_desde = aparece_t;
        else                     enemigo_desde = ahora;
        if (aparece_t >= 0) cola_desde = aparece_t + 0.2;
    }
    snprintf(enemigo_visto, sizeof enemigo_visto, "%s", p->imagen_enemigo);

    /* cambio el lugar: la imagen entra desde el negro */
    if (strcmp(p->imagen, fondo_visto) != 0) {
        fondo_t = final_t >= 0 ? final_t : ahora;   /* el final entra despues del ultimo golpe */
        fondo_dur = juego.estado == E_FINAL ? 1.6f : 0.45f;
        snprintf(fondo_visto, sizeof fondo_visto, "%s", p->imagen);
    }
    texto_t = final_t >= 0 ? final_t + 0.6 : ahora;
}

static void actualizar_efectos(void)
{
    int i, hay_enemigo = juego.pant.imagen_enemigo[0] != '\0';
    float objetivo;
    ahora = GetTime();
    for (i = 0; i < n_pendientes; i++)
        if (!pendientes[i].hecho && ahora >= pendientes[i].cuando) {
            pendientes[i].hecho = 1;
            if (!sin_animaciones) disparar(&pendientes[i].e);
        }
    /* el fantasma desaparece cuando termina de caer */
    if (fantasma[0] && ahora > fantasma_t + 0.6) fantasma[0] = '\0';
    /* el fondo se oscurece de a poco al empezar un combate y se aclara al terminar */
    objetivo = (hay_enemigo || fantasma[0]) ? 0.45f : 0;
    if (sin_animaciones) oscuridad = objetivo;
    else oscuridad += (objetivo - oscuridad) * fminf(1, GetFrameTime() * 5);
}

/* Desplazamiento de la pantalla por el temblor */
static Vector2 temblor_actual(void)
{
    float k, a;
    if (!activa(temblor_t, 0.4f)) return (Vector2){ 0, 0 };
    k = progreso(temblor_t, 0.4f);
    a = temblor_fuerza * (1 - k) * (1 - k);
    return (Vector2){ a * sinf((float)ahora * 97), a * 0.7f * cosf((float)ahora * 131) };
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
    if ((hay_musica || hay_sonidos) && CheckCollisionPointRec(p, BOTON_MUSICA)) { silencio = !silencio; return; }
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
        nuevos_efectos();
    }
    if (scroll > scroll_maximo()) scroll = scroll_maximo();
    if (scroll < 0) scroll = 0;

    actualizar_musica();
    actualizar_efectos();

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

    /* oscurecer el fondo en combate (entra y sale suave) */
    if (oscuridad > 0.01f) DrawRectangle(0, 0, ANCHO, IMG_ALTO, Fade(BLACK, oscuridad));

    if (enem) {
        /* posicion, tamaño y brillo de los que esperan detras (del mas cercano al mas lejano) */
        static const float pos_x[MAX_COLA_VISIBLE] = { -230, 230, -420, 420 };
        static const float escala[MAX_COLA_VISIBLE] = { 0.82f, 0.74f, 0.64f, 0.58f };
        static const unsigned char brillo[MAX_COLA_VISIBLE] = { 70, 55, 42, 34 };
        float a_cola = progreso(cola_desde, 0.8f);
        float a = progreso(enemigo_desde, 0.45f);
        int k;
        for (k = p->n_cola - 1; k >= 0; k--) {
            Imagen *atras = imagen(p->imagen_cola[k]);
            unsigned char b = brillo[k];
            if (atras && atras->ok)
                dibujar_enemigo_mov(atras->tex, ANCHO / 2.0f + pos_x[k], escala[k],
                                    (Color){ b, b, b, (unsigned char)(255 * a_cola) }, 0, 12 * (1 - a_cola));
        }
        if (enem->ok && a > 0) {
            /* aparece: sube un poco y se aclara */
            float dx = 0, dy = 18 * (1 - a), esc = 0.92f + 0.08f * a;
            Color tinte = { 255, 255, 255, (unsigned char)(255 * a) };
            if (!fantasma[0]) {
                if (activa(golpeado_t, 0.28f)) {        /* recibe un golpe: se sacude y se pone rojo */
                    float k2 = progreso(golpeado_t, 0.28f);
                    dx = 16 * (1 - k2) * sinf((float)ahora * 80);
                    tinte.g = tinte.b = (unsigned char)(255 * (0.35f + 0.65f * k2));
                }
                if (activa(esquiva_t, 0.35f))           /* esquiva: se corre a un costado */
                    dx += 90 * sinf(3.14159f * progreso(esquiva_t, 0.35f));
                if (activa(embestida_t, 0.26f))         /* se te tira encima: se agranda */
                    esc *= 1 + 0.16f * sinf(3.14159f * progreso(embestida_t, 0.26f));
            }
            dibujar_enemigo_mov(enem->tex, ANCHO / 2.0f, esc, tinte, dx, dy);
        } else if (!enem->ok) {
            texto(TextFormat("(falta assets/%s.png)", p->imagen_enemigo), ANCHO / 2.0f - 150, 200, 18, C_APAGADO);
        }
    }

    /* el enemigo que acaba de caer (o de irse) */
    if (fantasma[0]) {
        Imagen *f = imagen(fantasma);
        if (f && f->ok) {
            float k = progreso(fantasma_t, 0.55f), dx = 0, dy = 0;
            Color tinte = { 255, 255, 255, 255 };
            if (ahora < fantasma_t) {                  /* todavia no le toca: quieto */
                if (activa(golpeado_t, 0.28f)) {
                    float k2 = progreso(golpeado_t, 0.28f);
                    dx = 16 * (1 - k2) * sinf((float)ahora * 80);
                    tinte.g = tinte.b = (unsigned char)(255 * (0.35f + 0.65f * k2));
                }
                if (activa(esquiva_t, 0.35f)) dx += 90 * sinf(3.14159f * progreso(esquiva_t, 0.35f));
            } else if (fantasma_huye) {                /* se va hacia un costado */
                dx = 420 * k * k;
                tinte.a = (unsigned char)(255 * (1 - k));
            } else {                                   /* cae: se hunde, se oscurece y desaparece */
                dy = 140 * k * k;
                tinte.r = (unsigned char)(255 * (1 - 0.3f * k));
                tinte.g = tinte.b = (unsigned char)(140 * (1 - k));
                tinte.a = (unsigned char)(255 * (1 - k));
            }
            dibujar_enemigo_mov(f->tex, ANCHO / 2.0f, 1.0f, tinte, dx, dy);
        }
    }

    /* al cambiar de lugar, la imagen entra desde el negro */
    {
        float k = progreso(fondo_t, fondo_dur);
        if (k < 1) DrawRectangle(0, 0, ANCHO, IMG_ALTO, Fade(BLACK, 1 - k));
    }

    /* la imagen se funde con el negro de abajo */
    DrawRectangleGradientV(0, IMG_ALTO - 80, ANCHO, 80, Fade(BLACK, 0), C_NEGRO);

    /* boton de musica (arriba a la derecha): se toca o se aprieta M */
    if (hay_musica || hay_sonidos) {
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
    DrawTextEx(fuente_negrita, titulo, (Vector2){ TXT_X, TITULO_Y }, TAM_TITULO, 0,
               Fade(C_BLANCO, progreso(texto_t, 0.2f)));

    /* el texto nuevo aparece linea por linea (rapido) */
    for (i = 0; i < visibles && desde + i < n_lineas; i++) {
        float k = progreso(texto_t + 0.035 * i, 0.18f);
        texto(lineas[desde + i], TXT_X + 8 * (1 - k), TXT_Y + i * LINEA, TAM_TEXTO, Fade(C_BLANCO, k));
    }

    /* indicador de que hay mas texto arriba/abajo */
    if (n_lineas > visibles) {
        float total = (float)n_lineas, h = TXT_ALTO * visibles / total;
        float y = TXT_Y + (TXT_ALTO - h) * (scroll / fmaxf(1, scroll_maximo()));
        DrawRectangle(TXT_X + TXT_W + 14, (int)y, 2, (int)h, C_APAGADO);
        if (scroll < scroll_maximo() - 0.5f)
            texto("▼ más abajo", TXT_X + TXT_W - 100, ALTO - 26, 16, C_BLANCO);
    }

    if (juego.pant.pide_nombre)   /* version, chiquito abajo (para saber si se actualizo) */
        texto("versión " VERSION_JUEGO, ANCHO - 150, ALTO - 24, 14, C_APAGADO);

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
    if (activa(flash_t, flash_dur)) {   /* destello de color (disparo, daño, infeccion...) */
        float k = progreso(flash_t, flash_dur);
        DrawRectangle(0, 0, ANCHO, ALTO, Fade(flash_color, flash_alfa * (1 - k) * (1 - k)));
    }
    EndBlendMode();
    EndTextureMode();

    {   /* temblor de pantalla */
        Vector2 t = temblor_actual();
        o.x += t.x * s;
        o.y += t.y * s;
    }

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
    cargar_sonidos();
    juego_nuevo(&juego);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(cuadro, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose() && !cerrar) cuadro();
#endif

    descargar_sonidos();
    descargar_musica();
    UnloadRenderTexture(lienzo);
    CloseWindow();
    return 0;
}
