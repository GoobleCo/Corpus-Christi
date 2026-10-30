/*
 * MODULO BASE: LOGICA DEL JUEGO
 * Creacion del jugador, movimiento entre bloques, loot y combate.
 *
 * El juego es una maquina de estados: cada estado (Estado en juego.h) arma
 * una Pantalla con imagen, texto y opciones. Cuando el jugador elige una
 * opcion, juego_elegir() ejecuta la accion y arma la pantalla siguiente.
 *
 * El juego no muestra numeros (vida, daño, tiradas): describe lo que pasa.
 * La vida y la infeccion solo se ven en "Ver estado". Con el modo debug
 * "Mostrar números" aparecen todos los valores entre [corchetes].
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "juego.h"

/* =========================== UTILIDADES =========================== */

int d20_fijo = 0;      /* 0 = dado normal; 1..20 = "Always d20" (debug) */
int ver_numeros = 0;   /* debug: mostrar los numeros entre corchetes */

int azar(int min, int max) { return min + rand() % (max - min + 1); }

int tirar_d20(void)
{
    if (d20_fijo >= 1 && d20_fijo <= 20) return d20_fijo;
    return azar(1, 20);
}

int minimo(int a, int b) { return a < b ? a : b; }
int maximo(int a, int b) { return a > b ? a : b; }

static void log_va(Juego *g, const char *fmt, va_list ap)
{
    size_t len = strlen(g->msg);
    if (len >= MSG_MAX - 1) return;
    vsnprintf(g->msg + len, MSG_MAX - len, fmt, ap);
}

void log_msg(Juego *g, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_va(g, fmt, ap);
    va_end(ap);
}

void log_num(Juego *g, const char *fmt, ...)
{
    va_list ap;
    if (!ver_numeros) return;
    va_start(ap, fmt);
    log_va(g, fmt, ap);
    va_end(ap);
}

void agregar_opcion(Pantalla *p, int valor, int deshabilitada, const char *fmt, ...)
{
    Opcion *o;
    va_list ap;
    if (p->n_op >= MAX_OPCIONES) return;
    o = &p->op[p->n_op++];
    va_start(ap, fmt);
    vsnprintf(o->texto, sizeof o->texto, fmt, ap);
    va_end(ap);
    o->valor = valor;
    o->deshabilitada = deshabilitada;
}

void agregar_texto(Pantalla *p, const char *fmt, ...)
{
    size_t len = strlen(p->texto);
    va_list ap;
    if (len >= TEXTO_MAX - 1) return;
    va_start(ap, fmt);
    vsnprintf(p->texto + len, TEXTO_MAX - len, fmt, ap);
    va_end(ap);
}

/* "el enfermo" -> "El enfermo" */
static const char *mayus(const char *s)
{
    static char buf[64];
    strncpy(buf, s, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    if (buf[0] >= 'a' && buf[0] <= 'z') buf[0] = (char)(buf[0] - 'a' + 'A');
    return buf;
}

/* "el enfermo" -> "al enfermo", "lo que acecha" -> "a lo que acecha" */
static const char *a_quien(const Enemigo *e)
{
    static char buf[80];
    if (strncmp(e->percibido, "el ", 3) == 0) snprintf(buf, sizeof buf, "al %s", e->percibido + 3);
    else                                     snprintf(buf, sizeof buf, "a %s", e->percibido);
    return buf;
}

/* =========================== CLASES =========================== */

typedef struct {
    const char *nombre;
    const char *descripcion;
    int vida, fuerza;
    int items[5];
    int cant[5];
    int n;
} Clase;

static const Clase CLASES[] = {
    {"Vago (sin laburo)", "Aguantás mucho, pero nunca tuviste que pelear. Un cuchillo de cocina, un barbijo y cosas para arreglarlo.",
        120, 0,  {IT_ROPA, IT_BARBIJO, IT_CUCHILLO, IT_CINTA, IT_PIEZAS}, {1, 1, 1, 2, 1}, 5},
    {"Bombero", "Fuerte y acostumbrado al peligro. Tu uniforme, una máscara de gas y un hacha.",
        80, 20, {IT_UNI_BOMBERO, IT_MASCARA_GAS, IT_HACHA}, {1, 1, 1}, 3},
    {"Paramédico", "Sabés cuidarte. Un traje Hazmat y un botiquín.",
        100, 5,  {IT_HAZMAT, IT_BOTIQUIN}, {1, 1}, 2},
    {"Policía", "Tu uniforme, una pistola con diez balas y un barbijo.",
        90, 15, {IT_UNI_POLICIA, IT_9MM, IT_MUN_9MM, IT_BARBIJO}, {1, 1, 10, 1}, 4},
};
#define NUM_CLASES 4

static const char *NOMBRES_AZAR[] = {"Lucía", "Mateo", "Valen", "Sofi", "Nico", "Juli", "Tomi", "Cami"};

/* Acciones de las opciones */
enum { VOLVER = -1 };
enum { A_LOOT = 1, A_MOVER, A_INV, A_USAR, A_ESTADO, A_DEBUG };
enum { CA_ATACAR = 1, CA_USAR, CA_INV, CA_ESCAPAR, CA_DEBUG };

/* =========================== JUEGO NUEVO =========================== */

void juego_nuevo(Juego *g)
{
    int version = g->pant.version;
    memset(g, 0, sizeof *g);
    g->pant.version = version;
    iniciar_mapa();
    d20_fijo = 0;
    g->estado = E_NOMBRE;
    juego_construir_pantalla(g);
}

static void elegir_clase(Juego *g, int c)
{
    Jugador *j = &g->j;
    int k;
    j->clase = c;
    j->vida = j->vida_max = CLASES[c].vida;
    j->fuerza = CLASES[c].fuerza;
    j->n_inv = 0;
    for (k = 0; k < CLASES[c].n; k++)
        agregar_item(j, CLASES[c].items[k], CLASES[c].cant[k]);
    j->bloque_actual = B_INICIO;
    j->turno = 0;
    g->estado = E_BLOQUE;
    log_msg(g, "La radio sigue repitiendo lo mismo. Nadie dice hasta cuándo van a esperar.\n");
}

/* =========================== MUERTE =========================== */

void revisar_muerte(Juego *g)
{
    if (g->j.infeccion >= 100) terminar(g, F_CONVERTIDO);
    else if (g->j.vida <= 0)  terminar(g, F_MUERTE);
}

/* =========================== COMBATE =========================== */

static void iniciar_combate(Juego *g, int bloque)
{
    Combate *c = &g->c;
    int i, n;
    memset(c, 0, sizeof *c);
    c->color = mapa[bloque].color;
    c->subterraneo = mapa[bloque].subterraneo;
    c->sin_escape = mapa[bloque].especial == ESP_SIN_ESCAPE;
    n = cantidad_enemigos(c->color, c->subterraneo);
    for (i = 0; i < n; i++) c->cola[c->n++] = enemigo_aleatorio(c->color, c->subterraneo);

    g->j.turno++;   /* cada evento de combate cuenta como un turno */
    g->en_combate = 1;
    g->estado = E_COMBATE;

    log_msg(g, "\n");
    if (c->subterraneo) {
        if (c->n == 1) log_msg(g, "Una voz en la oscuridad: «Quieto ahí.»\n");
        else           log_msg(g, "Linternas. Voces. Te rodean antes de que puedas volver.\n");
    } else if (c->sin_escape) {
        log_msg(g, "Las puertas se cierran detrás tuyo. Algo se mueve en la terminal.\n");
    } else {
        if (c->n == 1)      log_msg(g, "Algo se separa de las sombras y viene hacia vos.\n");
        else if (c->n <= 3) log_msg(g, "Hay movimiento por todos lados. No estás solo.\n");
        else                log_msg(g, "Las sombras empiezan a moverse. Son demasiadas.\n");
    }
    log_num(g, "  [%d enemigos:", c->n);
    for (i = 0; i < c->n; i++) log_num(g, "%s %s", i ? "," : "", c->cola[i].nombre);
    log_num(g, "]\n");
}

static void soltar_arma_vandalo(Juego *g, const Enemigo *e)
{
    const ItemDef *a = &ITEMS[e->arma];
    int s;
    log_msg(g, "Entre sus cosas encontrás:\n");
    if (a->tipo == T_FUEGO) {
        recibir_loot(g, e->arma, 1);
        recibir_loot(g, a->municion, azar(2, 6));
    } else {
        log_msg(g, "  · %s\n", a->nombre);
        s = agregar_item(&g->j, e->arma, 1);
        if (s < 0) log_msg(g, "    (No te entra nada más. Lo dejás ahí.)\n");
        else if (a->usos > 0) g->j.inv[s].usos = azar(1, a->usos);   /* viene gastada */
    }
}

/* Ataque del jugador. Devuelve 1 si gasto el turno. */
static int atacar(Juego *g)
{
    Jugador *j = &g->j;
    Combate *c = &g->c;
    Enemigo *e = &c->cola[c->actual];
    int s = equipado_en(j, 0);
    int d;

    if (s >= 0 && ITEMS[j->inv[s].id].tipo == T_FUEGO) {
        const ItemDef *arma = &ITEMS[j->inv[s].id];
        int m = buscar_item(j, arma->municion);
        if (m < 0) {
            log_msg(g, "Click. No te quedan balas para tu %s.\n", arma->nombre);
            return 0;
        }
        quitar_slot(j, m, 1);
        if (e->evadiendo) {
            log_msg(g, "Disparás, pero ya no está ahí.\n");
            e->evadiendo = 0;
        } else {
            /* Arma de fuego: daño base + (d20 - armadura del enemigo) */
            int d20 = tirar_d20();
            d = maximo(0, arma->valor + (d20 - e->armadura));
            e->vida -= d;
            log_msg(g, "Disparás contra %s.\n", e->percibido);
            log_num(g, "  [d20 = %d, %d de daño → %d/%d]\n", d20, d, maximo(0, e->vida), e->vida_max);
        }
        /* los disparos hacen ruido */
        if (azar(1, 100) <= CHANCE_RUIDO && c->n < MAX_ENEMIGOS) {
            c->cola[c->n] = enemigo_aleatorio(c->color, c->subterraneo);
            log_msg(g, "  El estruendo rebota entre las paredes. Algo más viene.\n");
            c->n++;
        }
        return 1;
    }

    if (e->evadiendo) {
        log_msg(g, "Se mueve antes de que lo toques.\n");
        e->evadiendo = 0;
    } else if (s < 0) {
        d = maximo(1, DANIO_PUNOS + j->fuerza - e->armadura);
        e->vida -= d;
        log_msg(g, "Le pegás %s con los puños.\n", a_quien(e));
        log_num(g, "  [%d de daño → %d/%d]\n", d, maximo(0, e->vida), e->vida_max);
        return 1;
    } else {
        /* Arma blanca: daño base + fuerza - armadura del enemigo */
        d = maximo(1, ITEMS[j->inv[s].id].valor + j->fuerza - e->armadura);
        e->vida -= d;
        log_msg(g, "Golpeás %s con tu %s.\n", a_quien(e), ITEMS[j->inv[s].id].nombre);
        log_num(g, "  [%d de daño → %d/%d]\n", d, maximo(0, e->vida), e->vida_max);
    }
    /* desgaste del arma blanca */
    if (s >= 0 && j->inv[s].usos > 0) {
        j->inv[s].usos--;
        if (j->inv[s].usos == 0) {
            log_msg(g, "  Tu %s se rompe.\n", ITEMS[j->inv[s].id].nombre);
            quitar_slot(j, s, 1);
        }
    }
    return 1;
}

/* Despues de una accion del jugador que gasta el turno: le toca al enemigo */
static void resolver_turno(Juego *g)
{
    Combate *c = &g->c;
    Enemigo *e = &c->cola[c->actual];

    g->estado = E_COMBATE;
    if (e->vida <= 0) {
        if (e->tipo == EN_VANDALO) {
            log_msg(g, "%s cae y no se levanta.\n", mayus(e->percibido));
            soltar_arma_vandalo(g, e);
        } else {
            log_msg(g, "%s se desploma. Por un momento sigue moviéndose. Después, nada.\n", mayus(e->percibido));
        }
        c->actual++;   /* el jugador siempre empieza contra el próximo enemigo */
    } else if (turno_enemigo(g, e)) {
        c->actual++;
    }

    /* los que esperan detras tambien pueden alcanzarte */
    if (c->actual < c->n) ataque_horda(g);
    revisar_muerte(g);
    if (g->estado == E_FINAL) return;

    if (c->actual >= c->n) {
        g->en_combate = 0;
        g->estado = E_BLOQUE;
        log_msg(g, "\nSilencio otra vez. Te tiemblan las manos.\n");
    }
}

static int necesita_escape(const Juego *g)
{
    return (g->c.n - g->c.actual) * 5;   /* cada enemigo suma 5 */
}

static void elegir_combate(Juego *g, int v)
{
    switch (v) {
    case CA_ATACAR:
        if (atacar(g)) resolver_turno(g);
        break;
    case CA_USAR:
        g->volver = E_COMBATE; g->estado = E_USAR;
        break;
    case CA_INV:
        g->volver = E_COMBATE; g->estado = E_INVENTARIO;
        break;
    case CA_ESCAPAR: {
        int necesita = necesita_escape(g);
        int d20 = tirar_d20();
        log_msg(g, "Corrés.\n");
        log_num(g, "  [d20 = %d, necesitabas más de %d]\n", d20, necesita);
        if (d20 > necesita) {
            log_msg(g, "Los perdés de vista, pero no podés quedarte acá.\n");
            g->en_combate = 0;
            g->forzado = 1;
            g->estado = E_MOVER;
        } else {
            log_msg(g, "No hay por dónde pasar.\n");
            resolver_turno(g);
        }
        break;
    }
    case CA_DEBUG:
        g->volver = E_COMBATE; g->estado = E_DEBUG;
        break;
    }
}

/* =========================== BLOQUES =========================== */

static void lootear(Juego *g)
{
    Bloque *b = &mapa[g->j.bloque_actual];
    int n, i, id;
    if (b->looteado) { log_msg(g, "Ya revisaste este lugar. No queda nada.\n"); return; }
    b->looteado = 1;
    log_msg(g, "Revisás el lugar y encontrás:\n");
    if (b->item_garantizado >= 0)
        recibir_loot(g, b->item_garantizado, cantidad_loot(b->item_garantizado));
    n = azar(1, 3);
    for (i = 0; i < n; i++) {
        id = item_aleatorio(b->color);
        recibir_loot(g, id, cantidad_loot(id));
    }
}

static void hablar_checkpoint(Juego *g)
{
    log_msg(g, "Un soldado con traje protector te apunta con un termómetro y un fusil al mismo tiempo.\n");
    log_msg(g, "«Quieto. Control.»\n");
    log_num(g, "  [infección: %d%%, límite: %d%%]\n", g->j.infeccion, LIMITE_CUARENTENA);
    if (g->j.infeccion > LIMITE_CUARENTENA) {
        terminar(g, F_CUARENTENA);
        return;
    }
    if (g->j.infeccion > 0)
        log_msg(g, "Mira el termómetro un segundo de más antes de bajarlo.\n");
    if (contar_item(&g->j, IT_PASE) > 0)
        log_msg(g, "Revisa tu pase y baja el fusil. «Adelante.»\n");
    else
        log_msg(g, "«Sin pase no pasás. Volvé por donde viniste.»\n");
}

static void entrar_bloque(Juego *g, int destino)
{
    Bloque *b = &mapa[destino];
    Jugador *j = &g->j;
    int d20;

    g->forzado = 0;
    g->estado = E_BLOQUE;
    j->bloque_actual = destino;
    j->turno++;   /* cada cambio de bloque cuenta como un turno */

    switch (b->especial) {
    case ESP_TREN: terminar(g, F_TREN); return;
    case ESP_BOTE: terminar(g, F_BOTE); return;
    case ESP_HELIPUERTO:
        if (j->turno <= LIMITE_HELICOPTERO) terminar(g, F_HELICOPTERO);
        return;
    case ESP_CHECKPOINT:
        hablar_checkpoint(g);
        return;
    default:
        break;
    }

    if (b->color == C_CELESTE) return;

    /* Tirada de peligro: si el d20 supera el valor del bloque, no pasa nada */
    d20 = tirar_d20();
    log_num(g, "  [peligro: d20 = %d, necesitabas más de %d]\n", d20, PELIGRO_COLOR[b->color]);
    if (d20 > PELIGRO_COLOR[b->color]) return;
    iniciar_combate(g, destino);
}

/* Prueba de fuerza en los atajos. Devuelve 1 si se puede pasar.
 * Si no alcanza la fuerza, se pierde un turno intentándolo. */
static int puede_pasar(Juego *g, int destino)
{
    Paso *p = buscar_paso(g->j.bloque_actual, destino);
    if (p == NULL || p->abierto) return 1;
    log_num(g, "  [fuerza %d, necesitás %d]\n", g->j.fuerza, p->fuerza);
    if (g->j.fuerza >= p->fuerza) {
        p->abierto = 1;
        log_msg(g, "%s\n\n", p->exito);
        return 1;
    }
    p->intentado = 1;
    g->j.turno++;   /* el intento lleva tiempo */
    log_msg(g, "%s\n", p->falla);
    if (g->forzado) log_msg(g, "Tenés que buscar otro camino. Rápido.\n");
    else { log_msg(g, "Volvés sobre tus pasos.\n"); g->estado = E_BLOQUE; }
    return 0;
}

static int paso_bloqueado(const Juego *g, int destino)
{
    const Bloque *b = &mapa[g->j.bloque_actual];
    return b->especial == ESP_CHECKPOINT && b->protegido == destino && contar_item(&g->j, IT_PASE) == 0;
}

/* =========================== ELEGIR =========================== */

static int hay_armas_reparables(const Jugador *j)
{
    int i;
    for (i = 0; i < j->n_inv; i++)
        if (ITEMS[j->inv[i].id].tipo == T_MELEE && j->inv[i].usos >= 0) return 1;
    return 0;
}

/* Vuelve al combate (le toca al enemigo) o a la pantalla anterior */
static void despues_de_usar(Juego *g)
{
    if (g->volver == E_COMBATE && g->en_combate) resolver_turno(g);
    else g->estado = g->volver;
}

void juego_elegir(Juego *g, int indice)
{
    int v;
    if (indice < 0 || indice >= g->pant.n_op) return;
    if (g->pant.op[indice].deshabilitada) return;
    v = g->pant.op[indice].valor;
    g->msg[0] = '\0';

    switch (g->estado) {
    case E_NOMBRE:
        if (v == 1) {
            if (g->j.nombre[0] == '\0') strcpy(g->j.nombre, "Sobreviviente");
            g->estado = E_CLASE;
        } else {
            strcpy(g->j.nombre, NOMBRES_AZAR[azar(0, 7)]);
        }
        break;

    case E_CLASE:
        elegir_clase(g, v);
        break;

    case E_BLOQUE:
        switch (v) {
        case A_LOOT:   lootear(g); break;
        case A_MOVER:  g->forzado = 0; g->estado = E_MOVER; break;
        case A_INV:    g->volver = E_BLOQUE; g->estado = E_INVENTARIO; break;
        case A_USAR:   g->volver = E_BLOQUE; g->estado = E_USAR; break;
        case A_ESTADO: g->estado = E_ESTADO; break;
        case A_DEBUG:  g->volver = E_BLOQUE; g->estado = E_DEBUG; break;
        }
        break;

    case E_MOVER:
        if (v == VOLVER) g->estado = E_BLOQUE;
        else if (puede_pasar(g, v)) entrar_bloque(g, v);
        break;

    case E_INVENTARIO:
        if (v == VOLVER) { g->estado = g->volver; break; }
        if (categoria_equipo(g->j.inv[v].id) < 0) {
            log_msg(g, "%s no se equipa. Usalo desde \"Usar ítem\".\n", ITEMS[g->j.inv[v].id].nombre);
        } else if (g->j.inv[v].equipado) {
            g->j.inv[v].equipado = 0;
            log_msg(g, "Te sacás: %s.\n", ITEMS[g->j.inv[v].id].nombre);
        } else {
            equipar(&g->j, v);
            log_msg(g, "Te equipás: %s.\n", ITEMS[g->j.inv[v].id].nombre);
        }
        break;

    case E_USAR:
        if (v == VOLVER) { g->estado = g->volver; break; }
        if (ITEMS[g->j.inv[v].id].tipo == T_MATERIAL) {
            if (!hay_armas_reparables(&g->j)) {
                log_msg(g, "No tenés nada que reparar.\n");
            } else {
                g->slot_material = v;
                g->estado = E_REPARAR;
            }
        } else if (usar_item(g, v)) {
            despues_de_usar(g);
        }
        break;

    case E_REPARAR:
        if (v == VOLVER) { g->estado = E_USAR; break; }
        reparar_arma(g, v);
        despues_de_usar(g);
        break;

    case E_ESTADO:
        g->estado = E_BLOQUE;
        break;

    case E_COMBATE:
        elegir_combate(g, v);
        break;

    case E_FINAL:
        if (v == 1) juego_nuevo(g);
        else g->quiere_salir = 1;
        break;

    default:   /* pantallas de debug */
        debug_elegir(g, v);
        break;
    }

    juego_construir_pantalla(g);
}

/* =========================== PANTALLAS =========================== */

/* Pistas de que el tiempo se acaba, sin decir nada del helicoptero */
static void pista_del_tiempo(Juego *g, Pantalla *p)
{
    const Bloque *b = &mapa[g->j.bloque_actual];
    int t = g->j.turno;
    if (b->subterraneo) return;
    if (t > LIMITE_HELICOPTERO)
        agregar_texto(p, "\n\nHace rato que no se escucha nada en el cielo.");
    else if (t >= LIMITE_HELICOPTERO - 3)
        agregar_texto(p, "\n\nEl sonido de unas aspas, en algún lugar. Cada vez más débil.");
    else if (t >= LIMITE_HELICOPTERO - 8 && t % 2 == 0)
        agregar_texto(p, "\n\nPor un momento, un zumbido de aspas pasa sobre los techos.");
}

static void pantalla_bloque(Juego *g, Pantalla *p)
{
    const Bloque *b = &mapa[g->j.bloque_actual];
    snprintf(p->titulo, sizeof p->titulo, "%s", b->nombre);
    if (b->especial == ESP_HELIPUERTO && g->j.turno > LIMITE_HELICOPTERO)
        agregar_texto(p, "La terraza está vacía. Solo quedan marcas en el piso y el viento.");
    else
        agregar_texto(p, "%s", b->descripcion);
    if (b->especial == ESP_CHECKPOINT && b->protegido >= 0)
        agregar_texto(p, contar_item(&g->j, IT_PASE) ? " Tu pase te deja seguir."
                                                      : " Sin pase no te dejan seguir.");
    pista_del_tiempo(g, p);

    if (b->looteado) agregar_opcion(p, A_LOOT, 1, "Investigar el área");
    else             agregar_opcion(p, A_LOOT, 0, "Investigar el área");
    agregar_opcion(p, A_MOVER, 0, "Moverse");
    agregar_opcion(p, A_INV, 0, "Inventario");
    agregar_opcion(p, A_USAR, 0, "Usar ítem");
    agregar_opcion(p, A_ESTADO, 0, "Ver estado");
    if (DEBUG_HABILITADO) agregar_opcion(p, A_DEBUG, 0, "Debug");
}

static void pantalla_mover(Juego *g, Pantalla *p)
{
    const Bloque *b = &mapa[g->j.bloque_actual];
    int i, tiene_mapa = contar_item(&g->j, IT_MAPA) > 0;
    const Paso *paso;

    snprintf(p->titulo, sizeof p->titulo, "¿A dónde?");
    if (g->forzado) agregar_texto(p, "No podés quedarte acá.");
    else            agregar_texto(p, "Desde acá podés ir a:");

    for (i = 0; i < b->n_con; i++) {
        int d = b->conexiones[i];
        const Bloque *db = &mapa[d];
        char info[48] = "";
        if (tiene_mapa && db->color != C_CELESTE)
            snprintf(info, sizeof info, " · %d%% de peligro", PELIGRO_COLOR[db->color] * 5);
        else if (tiene_mapa)
            snprintf(info, sizeof info, " · seguro");
        if (paso_bloqueado(g, d))
            agregar_opcion(p, d, 1, "%s (sin pase)", db->nombre);
        else if ((paso = buscar_paso(g->j.bloque_actual, d)) && !paso->abierto && paso->intentado)
            agregar_opcion(p, d, 0, "%s (%s)", db->nombre, paso->obstaculo);
        else
            agregar_opcion(p, d, 0, "%s%s", db->nombre, info);
    }
    if (!g->forzado) agregar_opcion(p, VOLVER, 0, "Quedarse");
}

static void pantalla_inventario(Juego *g, Pantalla *p)
{
    Jugador *j = &g->j;
    char buf[96];
    int i;
    snprintf(p->titulo, sizeof p->titulo, "Inventario");
    agregar_texto(p, "Elegí algo para equiparlo o sacártelo. [E] = equipado.");
    if (g->volver == E_COMBATE) agregar_texto(p, " Cambiar el equipo no gasta tu turno.");
    if (j->n_inv == 0) agregar_texto(p, "\n\nNo tenés nada.");
    for (i = 0; i < j->n_inv; i++) {
        describir_slot(j, i, buf, sizeof buf);
        agregar_opcion(p, i, 0, "%s", buf);
    }
    agregar_opcion(p, VOLVER, 0, "Volver");
}

static void pantalla_usar(Juego *g, Pantalla *p)
{
    Jugador *j = &g->j;
    char buf[96];
    int i, n = 0;
    snprintf(p->titulo, sizeof p->titulo, "Usar ítem");
    for (i = 0; i < j->n_inv; i++) {
        if (!es_usable(j->inv[i].id)) continue;
        describir_slot(j, i, buf, sizeof buf);
        agregar_opcion(p, i, 0, "%s", buf);
        n++;
    }
    if (n == 0) agregar_texto(p, "No tenés nada para usar.");
    else if (g->volver == E_COMBATE) agregar_texto(p, "Usar algo gasta tu turno.");
    agregar_opcion(p, VOLVER, 0, "Volver");
}

static void pantalla_reparar(Juego *g, Pantalla *p)
{
    Jugador *j = &g->j;
    char buf[96];
    int i;
    snprintf(p->titulo, sizeof p->titulo, "Reparar");
    agregar_texto(p, "¿Qué querés reparar con %s? (+%d usos)",
                  ITEMS[j->inv[g->slot_material].id].nombre, ITEMS[j->inv[g->slot_material].id].valor);
    for (i = 0; i < j->n_inv; i++) {
        if (ITEMS[j->inv[i].id].tipo != T_MELEE || j->inv[i].usos < 0) continue;
        describir_slot(j, i, buf, sizeof buf);
        agregar_opcion(p, i, 0, "%s", buf);
    }
    agregar_opcion(p, VOLVER, 0, "Cancelar");
}

static void pantalla_estado(Juego *g, Pantalla *p)
{
    Jugador *j = &g->j;
    snprintf(p->titulo, sizeof p->titulo, "%s", j->nombre);
    agregar_texto(p, "%s\n\n", CLASES[j->clase].nombre);
    agregar_texto(p, "Vida: %d / %d\n", j->vida, j->vida_max);
    agregar_texto(p, "Infección: %d%%\n", j->infeccion);
    agregar_texto(p, "Fuerza: %d · Armadura: %d · Protección: -%d por herida\n",
                  j->fuerza, armadura_total(j), proteccion_infeccion(j));
    agregar_texto(p, "Arma: %s", nombre_arma_equipada(j));
    if (ver_numeros) agregar_texto(p, "\n\n[turno %d]", j->turno);
    agregar_opcion(p, VOLVER, 0, "Volver");
}

/* Como se ve el enemigo, segun cuanta vida le queda */
static const char *aspecto(const Enemigo *e)
{
    int pct = e->vida_max > 0 ? maximo(0, e->vida) * 100 / e->vida_max : 0;
    if (pct > 75) return "No tiene ni un rasguño.";
    if (pct > 40) return "Tambalea.";
    return "Apenas se mantiene en pie.";
}

static void pantalla_combate(Juego *g, Pantalla *p)
{
    Jugador *j = &g->j;
    Combate *c = &g->c;
    Enemigo *e = &c->cola[c->actual];
    int s = equipado_en(j, 0), sin_balas = 0, necesita = necesita_escape(g);
    int quedan = c->n - c->actual - 1;

    snprintf(p->titulo, sizeof p->titulo, "%s", mapa[j->bloque_actual].nombre);
    agregar_texto(p, "Frente a vos: %s. %s", e->percibido, aspecto(e));
    if (e->tipo == EN_VANDALO)
        agregar_texto(p, ITEMS[e->arma].tipo == T_FUEGO ? " Tiene un arma de fuego." : " Tiene algo filoso en la mano.");
    if (e->evadiendo) agregar_texto(p, " Está agazapado, atento a lo que hagas.");
    if (quedan == 1)      agregar_texto(p, "\nDetrás hay alguien más.");
    else if (quedan <= 3 && quedan > 0) agregar_texto(p, "\nDetrás hay más.");
    else if (quedan > 3)  agregar_texto(p, "\nDetrás, demasiadas formas para contarlas.");
    if (ver_numeros)
        agregar_texto(p, "\n[%s %d/%d · arm %d · quedan %d · vos %d/%d]", e->nombre, maximo(0, e->vida),
                      e->vida_max, e->armadura, quedan, j->vida, j->vida_max);

    if (s >= 0 && ITEMS[j->inv[s].id].tipo == T_FUEGO)
        sin_balas = contar_item(j, ITEMS[j->inv[s].id].municion) == 0;
    if (sin_balas) agregar_opcion(p, CA_ATACAR, 1, "Atacar (sin balas)");
    else if (s < 0) agregar_opcion(p, CA_ATACAR, 0, "Atacar con los puños");
    else           agregar_opcion(p, CA_ATACAR, 0, "Atacar con %s", nombre_arma_equipada(j));
    agregar_opcion(p, CA_USAR, 0, "Usar ítem");
    agregar_opcion(p, CA_INV, 0, "Inventario");
    if (c->sin_escape)
        agregar_opcion(p, CA_ESCAPAR, 1, "Escapar (no hay salida)");
    else if (necesita >= 20 && d20_fijo == 0)
        agregar_opcion(p, CA_ESCAPAR, 1, "Escapar (te rodean)");
    else
        agregar_opcion(p, CA_ESCAPAR, 0, "Escapar");
    if (DEBUG_HABILITADO) agregar_opcion(p, CA_DEBUG, 0, "Debug");
}

static void pantalla_final(Juego *g, Pantalla *p)
{
    snprintf(p->imagen, sizeof p->imagen, "finales/%s", final_slug(g->final));
    p->imagen_alt[0] = '\0';
    p->imagen_enemigo[0] = '\0';
    p->imagen_npc[0] = '\0';
    snprintf(p->titulo, sizeof p->titulo, "%s", final_titulo(g->final));
    agregar_texto(p, "%s", final_texto(g->final));
    agregar_opcion(p, 1, 0, "Jugar de nuevo");
    agregar_opcion(p, 2, 0, "Salir");
}

void juego_construir_pantalla(Juego *g)
{
    Pantalla *p = &g->pant;
    int version = p->version + 1, i;

    memset(p, 0, sizeof *p);
    p->version = version;
    snprintf(p->imagen, sizeof p->imagen, "lugares/%s", mapa[g->j.bloque_actual].slug);
    if (BIOMA[g->j.bloque_actual])
        snprintf(p->imagen_alt, sizeof p->imagen_alt, "biomas/%s", BIOMA[g->j.bloque_actual]);
    if (g->en_combate) {
        snprintf(p->imagen_enemigo, sizeof p->imagen_enemigo, "enemigos/%s", g->c.cola[g->c.actual].slug);
        /* los que esperan detras se ven oscurecidos */
        for (i = g->c.actual + 1; i < g->c.n && p->n_cola < MAX_COLA_VISIBLE; i++)
            snprintf(p->imagen_cola[p->n_cola++], sizeof p->imagen_cola[0], "enemigos/%s", g->c.cola[i].slug);
    }
    /* soldado de H.E.R.O. parado en los checkpoints con guardia y en la Universidad */
    if (!g->en_combate && (mapa[g->j.bloque_actual].especial == ESP_CHECKPOINT ||
                           g->j.bloque_actual == B_UNIVERSIDAD))
        snprintf(p->imagen_npc, sizeof p->imagen_npc, "npc/soldado_hero");
    if (g->msg[0] && g->estado != E_FINAL) agregar_texto(p, "%s\n", g->msg);

    switch (g->estado) {
    case E_NOMBRE:
        snprintf(p->imagen, sizeof p->imagen, "portada");
        p->imagen_alt[0] = '\0';
        p->imagen_npc[0] = '\0';
        snprintf(p->titulo, sizeof p->titulo, "Escape Corpus Christi");
        agregar_texto(p, "Algo pasó en la ciudad durante la noche. Desde el refugio se escuchan pasos "
                         "que no terminan de sonar humanos.\n\n"
                         "La radio, entre estática, repite un mismo mensaje: «...H.E.R.O. ... evacuación "
                         "en curso ... diríjanse a...» El resto se pierde.\n\n"
                         "¿Cómo te llamás?");
        p->pide_nombre = 1;
        agregar_opcion(p, 1, 0, "Comenzar");
        agregar_opcion(p, 2, 0, "Nombre al azar");
        break;
    case E_CLASE:
        snprintf(p->imagen, sizeof p->imagen, "portada");
        p->imagen_alt[0] = '\0';
        p->imagen_npc[0] = '\0';
        snprintf(p->titulo, sizeof p->titulo, "¿A qué te dedicabas, %s?", g->j.nombre);
        for (i = 0; i < NUM_CLASES; i++) {
            agregar_texto(p, "%s. %s\n", CLASES[i].nombre, CLASES[i].descripcion);
            agregar_opcion(p, i, 0, "%s", CLASES[i].nombre);
        }
        break;
    case E_BLOQUE:      pantalla_bloque(g, p); break;
    case E_MOVER:       pantalla_mover(g, p); break;
    case E_INVENTARIO:  pantalla_inventario(g, p); break;
    case E_USAR:        pantalla_usar(g, p); break;
    case E_REPARAR:     pantalla_reparar(g, p); break;
    case E_ESTADO:      pantalla_estado(g, p); break;
    case E_COMBATE:     pantalla_combate(g, p); break;
    case E_FINAL:       pantalla_final(g, p); break;
    default:            debug_pantalla(g); break;
    }
}
