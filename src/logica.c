/*
 * MODULO BASE: LOGICA DEL JUEGO
 * Creacion del jugador, movimiento entre bloques, loot y combate.
 *
 * El juego es una maquina de estados: cada estado (Estado en juego.h) arma
 * una Pantalla con imagen, texto y opciones. Cuando el jugador elige una
 * opcion, juego_elegir() ejecuta la accion y arma la pantalla siguiente.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "juego.h"

/* =========================== UTILIDADES =========================== */

int d20_fijo = 0;   /* 0 = dado normal; 1..20 = "Always d20" (debug) */

int azar(int min, int max) { return min + rand() % (max - min + 1); }

int tirar_d20(void)
{
    if (d20_fijo >= 1 && d20_fijo <= 20) return d20_fijo;
    return azar(1, 20);
}

int minimo(int a, int b) { return a < b ? a : b; }
int maximo(int a, int b) { return a > b ? a : b; }

void log_msg(Juego *g, const char *fmt, ...)
{
    size_t len = strlen(g->msg);
    va_list ap;
    if (len >= MSG_MAX - 1) return;
    va_start(ap, fmt);
    vsnprintf(g->msg + len, MSG_MAX - len, fmt, ap);
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
    {"Vago (sin laburo)", "Mucha vida, pero sin fuerza ni armas. Barbijo y materiales.",
        120, 0,  {IT_ROPA, IT_BARBIJO, IT_CINTA, IT_PIEZAS}, {1, 1, 2, 1}, 4},
    {"Bombero", "Fuerte y protegido contra la infección. Hacha y máscara de gas.",
        80, 20, {IT_UNI_BOMBERO, IT_MASCARA_GAS, IT_HACHA}, {1, 1, 1}, 3},
    {"Paramédico", "Traje Hazmat contra la infección y un botiquín.",
        100, 5,  {IT_HAZMAT, IT_BOTIQUIN}, {1, 1}, 2},
    {"Policía", "La mejor armadura. Pistola con 10 balas y barbijo.",
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
    log_msg(g, "Buena suerte, %s. El helicóptero de HERO sale en el turno %d.\n",
            j->nombre, LIMITE_HELICOPTERO);
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

    log_msg(g, "\n¡%s! Te rodean %d enemigo%s:", c->subterraneo ? "EMBOSCADA" : "INFECTADOS",
            c->n, c->n > 1 ? "s" : "");
    for (i = 0; i < c->n; i++) log_msg(g, "%s %s", i ? "," : "", c->cola[i].nombre);
    log_msg(g, ".\n");
}

static void soltar_arma_vandalo(Juego *g, const Enemigo *e)
{
    const ItemDef *a = &ITEMS[e->arma];
    int s;
    log_msg(g, "  El Vándalo soltó su %s.\n", a->nombre);
    if (a->tipo == T_FUEGO) {
        recibir_loot(g, e->arma, 1);
        recibir_loot(g, a->municion, azar(2, 6));
    } else {
        log_msg(g, "  + %s  [%s]\n", a->nombre, NOMBRE_RAREZA[a->rareza]);
        s = agregar_item(&g->j, e->arma, 1);
        if (s < 0) log_msg(g, "    (Tu inventario está lleno, lo dejaste ahí.)\n");
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
            log_msg(g, "¡No tenés %s! Cambiá de arma desde el inventario.\n", ITEMS[arma->municion].nombre);
            return 0;
        }
        quitar_slot(j, m, 1);
        if (e->evadiendo) {
            log_msg(g, "Disparás tu %s... ¡pero el Stalker esquiva la bala!\n", arma->nombre);
            e->evadiendo = 0;
        } else {
            /* Arma de fuego: daño base + (d20 - armadura del enemigo) */
            int d20 = tirar_d20();
            d = maximo(0, arma->valor + (d20 - e->armadura));
            e->vida -= d;
            log_msg(g, "Disparás tu %s (d20: %d): %d de daño a %s.\n", arma->nombre, d20, d, e->nombre);
        }
        /* los disparos hacen ruido */
        if (azar(1, 100) <= CHANCE_RUIDO && c->n < MAX_ENEMIGOS) {
            c->cola[c->n] = enemigo_aleatorio(c->color, c->subterraneo);
            log_msg(g, "  ¡El ruido del disparo atrajo a un %s!\n", c->cola[c->n].nombre);
            c->n++;
        }
        return 1;
    }

    if (e->evadiendo) {
        log_msg(g, "Atacás... ¡pero el Stalker esquiva tu golpe!\n");
        e->evadiendo = 0;
    } else if (s < 0) {
        d = maximo(1, DANIO_PUNOS + j->fuerza - e->armadura);
        e->vida -= d;
        log_msg(g, "Golpeás con los puños a %s: %d de daño.\n", e->nombre, d);
        return 1;
    } else {
        /* Arma blanca: daño base + fuerza - armadura del enemigo */
        d = maximo(1, ITEMS[j->inv[s].id].valor + j->fuerza - e->armadura);
        e->vida -= d;
        log_msg(g, "Atacás con tu %s: %d de daño a %s.\n", ITEMS[j->inv[s].id].nombre, d, e->nombre);
    }
    /* desgaste del arma blanca */
    if (s >= 0 && j->inv[s].usos > 0) {
        j->inv[s].usos--;
        if (j->inv[s].usos == 0) {
            log_msg(g, "  ¡Tu %s se rompió!\n", ITEMS[j->inv[s].id].nombre);
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
        log_msg(g, "✔ Derrotaste a %s.\n", e->nombre);
        if (e->tipo == EN_VANDALO) soltar_arma_vandalo(g, e);
        c->actual++;   /* el jugador siempre empieza contra el próximo enemigo */
    } else if (turno_enemigo(g, e)) {
        c->actual++;
    } else {
        revisar_muerte(g);
        if (g->estado == E_FINAL) return;
    }

    if (c->actual >= c->n) {
        g->en_combate = 0;
        g->estado = E_BLOQUE;
        log_msg(g, "\nTodos los enemigos fueron derrotados. Respirás hondo.\n");
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
        log_msg(g, "Intentás escapar (d20: %d, necesitás más de %d)...\n", d20, necesita);
        if (d20 > necesita) {
            log_msg(g, "¡Lograste escapar! Tenés que moverte a otro bloque.\n");
            g->en_combate = 0;
            g->forzado = 1;
            g->estado = E_MOVER;
        } else {
            log_msg(g, "¡No pudiste escapar!\n");
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
    if (b->looteado) { log_msg(g, "Ya revisaste este lugar, no queda nada.\n"); return; }
    b->looteado = 1;
    log_msg(g, "Revisás %s y encontrás:\n", b->nombre);
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
    log_msg(g, "Un soldado de HERO te apunta con un termómetro y un fusil al mismo tiempo.\n");
    log_msg(g, "\"Credenciales y control de infección. No te muevas.\"\n");
    log_msg(g, "Infección detectada: %d%%\n", g->j.infeccion);
    if (g->j.infeccion > LIMITE_CUARENTENA) {
        terminar(g, F_CUARENTENA);
        return;
    }
    if (contar_item(&g->j, IT_PASE) > 0)
        log_msg(g, "\"Pase de Prioridad válido. Podés pasar.\"\n");
    else
        log_msg(g, "\"Sin Pase de Prioridad no pasás. Podés volver por donde viniste.\"\n");
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
        else log_msg(g, "Llegaste tarde. El helicóptero ya despegó. Todavía quedan el tren y el barco.\n");
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
    if (contar_item(j, IT_MAPA) > 0)
        log_msg(g, "Tirada de peligro: d20 = %d (necesitabas más de %d).\n", d20, PELIGRO_COLOR[b->color]);
    else
        log_msg(g, "Tirada de peligro: d20 = %d.\n", d20);
    if (d20 > PELIGRO_COLOR[b->color]) {
        log_msg(g, "La zona parece tranquila... por ahora.\n");
        return;
    }
    iniciar_combate(g, destino);
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
        else entrar_bloque(g, v);
        break;

    case E_INVENTARIO:
        if (v == VOLVER) { g->estado = g->volver; break; }
        if (categoria_equipo(g->j.inv[v].id) < 0) {
            log_msg(g, "%s no se equipa: usalo desde \"Usar ítem\".\n", ITEMS[g->j.inv[v].id].nombre);
        } else if (g->j.inv[v].equipado) {
            g->j.inv[v].equipado = 0;
            log_msg(g, "Te sacaste: %s.\n", ITEMS[g->j.inv[v].id].nombre);
        } else {
            equipar(&g->j, v);
            log_msg(g, "Equipaste: %s.\n", ITEMS[g->j.inv[v].id].nombre);
        }
        break;

    case E_USAR:
        if (v == VOLVER) { g->estado = g->volver; break; }
        if (ITEMS[g->j.inv[v].id].tipo == T_MATERIAL) {
            if (!hay_armas_reparables(&g->j)) {
                log_msg(g, "No tenés armas blancas para reparar.\n");
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

static void pantalla_bloque(Juego *g, Pantalla *p)
{
    const Bloque *b = &mapa[g->j.bloque_actual];
    snprintf(p->titulo, sizeof p->titulo, "%s", b->nombre);
    agregar_texto(p, "%s", b->descripcion);
    if (b->especial == ESP_HELIPUERTO && g->j.turno > LIMITE_HELICOPTERO)
        agregar_texto(p, " La terraza está vacía.");
    if (b->especial == ESP_CHECKPOINT && b->protegido >= 0)
        agregar_texto(p, contar_item(&g->j, IT_PASE) ? " Tu Pase de Prioridad te deja seguir."
                                                      : " Sin Pase de Prioridad no te dejan seguir.");

    if (b->looteado) agregar_opcion(p, A_LOOT, 1, "Investigar el área (ya revisado)");
    else             agregar_opcion(p, A_LOOT, 0, "Investigar el área");
    agregar_opcion(p, A_MOVER, 0, "Moverse a otro bloque");
    agregar_opcion(p, A_INV, 0, "Abrir inventario");
    agregar_opcion(p, A_USAR, 0, "Usar ítem");
    agregar_opcion(p, A_ESTADO, 0, "Ver estado");
    if (DEBUG_HABILITADO) agregar_opcion(p, A_DEBUG, 0, "Debug");
}

static void pantalla_mover(Juego *g, Pantalla *p)
{
    const Bloque *b = &mapa[g->j.bloque_actual];
    int i, tiene_mapa = contar_item(&g->j, IT_MAPA) > 0;

    snprintf(p->titulo, sizeof p->titulo, "¿A dónde vas?");
    if (g->forzado) agregar_texto(p, "No te podés quedar en %s: elegí a dónde correr.", b->nombre);
    else            agregar_texto(p, "Desde %s podés ir a:", b->nombre);
    if (!tiene_mapa) agregar_texto(p, "\n\nNo sabés qué tan peligroso es cada lugar. Un Mapa Satelital te lo mostraría.");

    for (i = 0; i < b->n_con; i++) {
        int d = b->conexiones[i];
        const Bloque *db = &mapa[d];
        char info[48] = "";
        if (db->color == C_CELESTE)
            snprintf(info, sizeof info, " · zona HERO");
        else if (tiene_mapa)
            snprintf(info, sizeof info, " · %s %d%%", NOMBRE_COLOR[db->color], PELIGRO_COLOR[db->color] * 5);
        if (paso_bloqueado(g, d))
            agregar_opcion(p, d, 1, "%s (necesitás Pase)", db->nombre);
        else
            agregar_opcion(p, d, 0, "%s%s", db->nombre, info);
    }
    if (!g->forzado) agregar_opcion(p, VOLVER, 0, "Quedarse acá");
}

static void pantalla_inventario(Juego *g, Pantalla *p)
{
    Jugador *j = &g->j;
    char buf[96];
    int i;
    snprintf(p->titulo, sizeof p->titulo, "Inventario (%d/%d)", j->n_inv, MAX_INV);
    agregar_texto(p, "Arma: %s · Armadura: %d · Protección: -%d de infección por ataque.\n\n",
                  nombre_arma_equipada(j), armadura_total(j), proteccion_infeccion(j));
    agregar_texto(p, "Elegí un arma, armadura o máscara para equiparla o sacártela. [E] = equipado.");
    if (g->volver == E_COMBATE) agregar_texto(p, " Cambiar el equipo no gasta tu turno.");
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
    if (n == 0) agregar_texto(p, "No tenés ítems usables.");
    else agregar_texto(p, "Vida: %d/%d · Infección: %d%%.%s", j->vida, j->vida_max, j->infeccion,
                       g->volver == E_COMBATE ? " Usar un ítem gasta tu turno." : "");
    agregar_opcion(p, VOLVER, 0, "Volver");
}

static void pantalla_reparar(Juego *g, Pantalla *p)
{
    Jugador *j = &g->j;
    char buf[96];
    int i;
    snprintf(p->titulo, sizeof p->titulo, "Reparar con %s", ITEMS[j->inv[g->slot_material].id].nombre);
    agregar_texto(p, "¿Qué arma blanca querés reparar? (+%d usos)", ITEMS[j->inv[g->slot_material].id].valor);
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
    int s = equipado_en(j, 0);
    char buf[96] = "Puños";
    if (s >= 0) describir_slot(j, s, buf, sizeof buf);
    snprintf(p->titulo, sizeof p->titulo, "%s · %s", j->nombre, CLASES[j->clase].nombre);
    agregar_texto(p, "Vida: %d/%d\nFuerza: %d\nArmadura: %d\nInfección: %d%% (protección: -%d por ataque)\n",
                  j->vida, j->vida_max, j->fuerza, armadura_total(j), j->infeccion, proteccion_infeccion(j));
    agregar_texto(p, "Arma: %s\n", buf);
    if (j->turno <= LIMITE_HELICOPTERO)
        agregar_texto(p, "Turno: %d. El helicóptero de HERO sale en el turno %d.\n", j->turno, LIMITE_HELICOPTERO);
    else
        agregar_texto(p, "Turno: %d. El helicóptero ya se fue: quedan el tren y el barco.\n", j->turno);
    agregar_texto(p, "Pase de Prioridad: %s · Mapa Satelital: %s",
                  contar_item(j, IT_PASE) ? "sí" : "no", contar_item(j, IT_MAPA) ? "sí" : "no");
    agregar_opcion(p, VOLVER, 0, "Volver");
}

static void pantalla_combate(Juego *g, Pantalla *p)
{
    Jugador *j = &g->j;
    Combate *c = &g->c;
    Enemigo *e = &c->cola[c->actual];
    int s = equipado_en(j, 0), sin_balas = 0, necesita = necesita_escape(g);

    snprintf(p->titulo, sizeof p->titulo, "%s · enemigo %d de %d",
             c->subterraneo ? "EMBOSCADA DE VÁNDALOS" : "ATAQUE DE INFECTADOS", c->actual + 1, c->n);
    agregar_texto(p, "%s · Vida %d/%d · Armadura %d", e->nombre, maximo(0, e->vida), e->vida_max, e->armadura);
    if (e->tipo == EN_VANDALO) agregar_texto(p, " · Arma: %s", ITEMS[e->arma].nombre);
    if (e->evadiendo) agregar_texto(p, " · listo para esquivar");
    if (c->n - c->actual == 2) agregar_texto(p, "\nQueda 1 más esperando.");
    else if (c->n - c->actual > 2) agregar_texto(p, "\nQuedan %d más esperando.", c->n - c->actual - 1);

    if (s >= 0 && ITEMS[j->inv[s].id].tipo == T_FUEGO)
        sin_balas = contar_item(j, ITEMS[j->inv[s].id].municion) == 0;
    if (sin_balas) agregar_opcion(p, CA_ATACAR, 1, "Atacar con %s (sin balas)", nombre_arma_equipada(j));
    else           agregar_opcion(p, CA_ATACAR, 0, "Atacar con %s", nombre_arma_equipada(j));
    agregar_opcion(p, CA_USAR, 0, "Usar ítem");
    agregar_opcion(p, CA_INV, 0, "Abrir inventario");
    if (c->sin_escape)
        agregar_opcion(p, CA_ESCAPAR, 1, "Escapar (no hay salida)");
    else if (necesita >= 20 && d20_fijo == 0)
        agregar_opcion(p, CA_ESCAPAR, 1, "Escapar (imposible: son demasiados)");
    else
        agregar_opcion(p, CA_ESCAPAR, 0, "Escapar (d20 mayor a %d)", necesita);
    if (DEBUG_HABILITADO) agregar_opcion(p, CA_DEBUG, 0, "Debug");
}

static void pantalla_final(Juego *g, Pantalla *p)
{
    snprintf(p->imagen, sizeof p->imagen, "finales/%s", final_slug(g->final));
    p->imagen_enemigo[0] = '\0';
    snprintf(p->titulo, sizeof p->titulo, "FINAL: %s", final_titulo(g->final));
    agregar_texto(p, "%s\n\n%s sobrevivió %d turnos. Vida %d/%d · Infección %d%%",
                  final_texto(g->final), g->j.nombre, g->j.turno,
                  g->j.vida, g->j.vida_max, g->j.infeccion);
    agregar_opcion(p, 1, 0, "Jugar de nuevo");
    agregar_opcion(p, 2, 0, "Salir del juego");
}

void juego_construir_pantalla(Juego *g)
{
    Pantalla *p = &g->pant;
    int version = p->version + 1, i;

    memset(p, 0, sizeof *p);
    p->version = version;
    p->mostrar_hud = g->estado != E_NOMBRE && g->estado != E_CLASE && g->estado != E_FINAL;
    snprintf(p->imagen, sizeof p->imagen, "bloques/%s", mapa[g->j.bloque_actual].slug);
    if (g->en_combate)
        snprintf(p->imagen_enemigo, sizeof p->imagen_enemigo, "enemigos/%s", g->c.cola[g->c.actual].slug);
    if (g->msg[0]) agregar_texto(p, "%s\n", g->msg);

    switch (g->estado) {
    case E_NOMBRE:
        snprintf(p->imagen, sizeof p->imagen, "portada");
        snprintf(p->titulo, sizeof p->titulo, "ESCAPE DE LA CIUDAD");
        agregar_texto(p, "La ciudad cayó. Los infectados llenan las calles. HERO, la fuerza del gobierno, "
                         "está evacuando civiles: un helicóptero al norte, un tren al oeste y un barco al sur.\n\n"
                         "Escribí tu nombre con el teclado:");
        p->pide_nombre = 1;
        agregar_opcion(p, 1, 0, "Comenzar");
        agregar_opcion(p, 2, 0, "Nombre al azar");
        break;
    case E_CLASE:
        snprintf(p->imagen, sizeof p->imagen, "portada");
        snprintf(p->titulo, sizeof p->titulo, "¿Cuál era tu laburo, %s?", g->j.nombre);
        for (i = 0; i < NUM_CLASES; i++) {
            agregar_texto(p, "%s — Vida %d, Fuerza %d. %s\n", CLASES[i].nombre,
                          CLASES[i].vida, CLASES[i].fuerza, CLASES[i].descripcion);
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
