/*
 * MODULO ITEMS
 * Tabla de items, inventario, loot y uso de items.
 */
#include <stdio.h>
#include <string.h>
#include "juego.h"

const ItemDef ITEMS[NUM_ITEMS] = {
    /*                         nombre                       tipo        rareza      valor usos anti mun */
    [IT_NAVAJA]            = {"Navaja de Bolsillo",        T_MELEE,    R_COMUN,     10, -1,   0, -1},   /* documento: 2 */
    [IT_CUCHILLO]          = {"Cuchillo de Cocina",        T_MELEE,    R_COMUN,     14, 10,   0, -1},   /* documento: 7 */
    [IT_HACHA]             = {"Hacha de Bombero",          T_MELEE,    R_RARO,      15, 30,   0, -1},
    [IT_MACHETE]           = {"Machete",                   T_MELEE,    R_EPICO,     20, 15,   0, -1},
    [IT_CINTA]             = {"Cinta",                     T_MATERIAL, R_COMUN,      5,  0,   0, -1},
    [IT_PIEZAS]            = {"Piezas Metálicas",          T_MATERIAL, R_RARO,      10,  0,   0, -1},
    [IT_9MM]               = {"Pistola 9mm",               T_FUEGO,    R_RARO,      30,  0,   0, IT_MUN_9MM},
    [IT_ESCOPETA]          = {"Escopeta",                  T_FUEGO,    R_EPICO,     50,  0,   0, IT_MUN_ESCOPETA},
    [IT_M16]               = {"M16",                       T_FUEGO,    R_EPICO,     40,  0,   0, IT_MUN_RIFLE},
    [IT_MUN_9MM]           = {"Munición 9mm",              T_MUNICION, R_COMUN,      0,  0,   0, -1},
    [IT_MUN_ESCOPETA]      = {"Munición de Escopeta",      T_MUNICION, R_RARO,       0,  0,   0, -1},
    [IT_MUN_RIFLE]         = {"Munición de Rifle",         T_MUNICION, R_RARO,       0,  0,   0, -1},
    [IT_VENDAJE]           = {"Vendaje",                   T_CURA,     R_COMUN,     20,  0,   0, -1},
    [IT_PILDORAS]          = {"Píldoras",                  T_CURA,     R_RARO,      40,  0,   0, -1},
    [IT_BOTIQUIN]          = {"Botiquín",                  T_CURA,     R_EPICO,     80,  0,   0, -1},
    [IT_ROPA]              = {"Ropa Común",                T_ARMADURA, R_COMUN,      0,  0,   0, -1},
    [IT_UNI_PARAMEDICO]    = {"Uniforme de Paramédico",    T_ARMADURA, R_COMUN,      0,  0,   0, -1},
    [IT_HAZMAT]            = {"Hazmat Civil",              T_ARMADURA, R_COMUN,      0,  0,  20, -1},
    [IT_UNI_BOMBERO]       = {"Uniforme de Bombero",       T_ARMADURA, R_COMUN,      5,  0,  10, -1},
    [IT_TRAJE_SOBREV]      = {"Traje de Sobreviviente",    T_ARMADURA, R_COMUN,      9,  0,   0, -1},
    [IT_UNI_POLICIA]       = {"Uniforme de Policía",       T_ARMADURA, R_RARO,      10,  0,   0, -1},
    [IT_UNI_ANTIDISTURBIO] = {"Uniforme de Antidisturbio", T_ARMADURA, R_EPICO,     15,  0,  30, -1},
    [IT_UNI_HERO]          = {"Uniforme de H.E.R.O.",      T_ARMADURA, R_LEGENDARIO,20,  0, 100, -1},
    [IT_VACUNA]            = {"Vacuna de H.E.R.O.",        T_MEJORA,   R_LEGENDARIO,50,  0,   0, -1},
    [IT_SUERO]             = {"Suero de H.E.R.O.",         T_MEJORA,   R_LEGENDARIO,15,  0,   0, -1},
    [IT_MAPA]              = {"Mapa Satelital",            T_MEJORA,   R_LEGENDARIO, 0,  0,   0, -1},
    [IT_CURA_TEMPRANA]     = {"Cura Temprana",             T_MEJORA,   R_LEGENDARIO, 0,  0,   0, -1},
    [IT_PASE]              = {"Pase de Prioridad",         T_MEJORA,   R_LEGENDARIO, 0,  0,   0, -1},
    [IT_BARBIJO]           = {"Barbijo",                   T_MASCARA,  R_COMUN,      0,  0,   5, -1},
    [IT_MASCARA_GAS]       = {"Máscara de Gas",            T_MASCARA,  R_RARO,       0,  0,  10, -1},
};

const char *NOMBRE_RAREZA[4] = {"Común", "Raro", "Épico", "LEGENDARIO"};

/* ---------------- Inventario ---------------- */

/* Categorias de equipamiento: 0 = arma, 1 = armadura, 2 = mascara */
int categoria_equipo(int id)
{
    switch (ITEMS[id].tipo) {
    case T_MELEE: case T_FUEGO: return 0;
    case T_ARMADURA:            return 1;
    case T_MASCARA:             return 2;
    default:                    return -1;
    }
}

static int es_apilable(int id)
{
    TipoItem t = ITEMS[id].tipo;
    return t == T_MUNICION || t == T_CURA || t == T_MATERIAL || t == T_MEJORA;
}

int es_usable(int id)
{
    TipoItem t = ITEMS[id].tipo;
    return t == T_CURA || t == T_MATERIAL || t == T_MEJORA;
}

int buscar_item(const Jugador *j, int id)
{
    int i;
    for (i = 0; i < j->n_inv; i++)
        if (j->inv[i].id == id) return i;
    return -1;
}

int contar_item(const Jugador *j, int id)
{
    int i, total = 0;
    for (i = 0; i < j->n_inv; i++)
        if (j->inv[i].id == id) total += j->inv[i].cantidad;
    return total;
}

int equipado_en(const Jugador *j, int categoria)
{
    int i;
    for (i = 0; i < j->n_inv; i++)
        if (j->inv[i].equipado && categoria_equipo(j->inv[i].id) == categoria) return i;
    return -1;
}

void equipar(Jugador *j, int slot)
{
    int i, cat = categoria_equipo(j->inv[slot].id);
    if (cat < 0) return;
    for (i = 0; i < j->n_inv; i++)
        if (categoria_equipo(j->inv[i].id) == cat) j->inv[i].equipado = 0;
    j->inv[slot].equipado = 1;
}

void quitar_slot(Jugador *j, int slot, int cant)
{
    int i;
    j->inv[slot].cantidad -= cant;
    if (j->inv[slot].cantidad > 0) return;
    for (i = slot; i < j->n_inv - 1; i++) j->inv[i] = j->inv[i + 1];
    j->n_inv--;
}

/* Devuelve el slot donde quedo el item, o -1 si el inventario esta lleno */
int agregar_item(Jugador *j, int id, int cant)
{
    int s = -1, k;
    if (es_apilable(id)) {
        s = buscar_item(j, id);
        if (s >= 0) { j->inv[s].cantidad += cant; return s; }
        if (j->n_inv >= MAX_INV) return -1;
        s = j->n_inv++;
        j->inv[s].id = id; j->inv[s].cantidad = cant;
        j->inv[s].usos = 0; j->inv[s].equipado = 0;
        return s;
    }
    for (k = 0; k < cant; k++) {
        if (j->n_inv >= MAX_INV) return s;
        s = j->n_inv++;
        j->inv[s].id = id; j->inv[s].cantidad = 1;
        j->inv[s].usos = ITEMS[id].usos; j->inv[s].equipado = 0;
        /* si no tenia nada equipado en esa categoria, se equipa solo */
        if (categoria_equipo(id) >= 0 && equipado_en(j, categoria_equipo(id)) < 0)
            equipar(j, s);
    }
    return s;
}

int armadura_total(const Jugador *j)
{
    int s = equipado_en(j, 1);
    return s >= 0 ? ITEMS[j->inv[s].id].valor : 0;
}

int proteccion_infeccion(const Jugador *j)
{
    int total = 0, s;
    if ((s = equipado_en(j, 1)) >= 0) total += ITEMS[j->inv[s].id].anti_infec;
    if ((s = equipado_en(j, 2)) >= 0) total += ITEMS[j->inv[s].id].anti_infec;
    return total;
}

void describir_slot(const Jugador *j, int s, char *buf, int tam)
{
    const Slot *sl = &j->inv[s];
    const ItemDef *d = &ITEMS[sl->id];
    const char *eq = sl->equipado ? "[E] " : "";
    switch (d->tipo) {
    case T_MELEE:
        if (sl->usos < 0) snprintf(buf, tam, "%s%s (daño %d, ∞ usos)", eq, d->nombre, d->valor);
        else snprintf(buf, tam, "%s%s (daño %d, %d/%d usos)", eq, d->nombre, d->valor, sl->usos, d->usos);
        break;
    case T_FUEGO:
        snprintf(buf, tam, "%s%s (daño %d, %d balas)", eq, d->nombre, d->valor, contar_item(j, d->municion));
        break;
    case T_ARMADURA:
        if (d->anti_infec > 0) snprintf(buf, tam, "%s%s (arm %d, -%d inf)", eq, d->nombre, d->valor, d->anti_infec);
        else                   snprintf(buf, tam, "%s%s (arm %d)", eq, d->nombre, d->valor);
        break;
    case T_MASCARA:
        snprintf(buf, tam, "%s%s (-%d inf)", eq, d->nombre, d->anti_infec);
        break;
    case T_CURA:
        snprintf(buf, tam, "%s (+%d vida) x%d", d->nombre, d->valor, sl->cantidad);
        break;
    case T_MATERIAL:
        snprintf(buf, tam, "%s (+%d usos) x%d", d->nombre, d->valor, sl->cantidad);
        break;
    case T_MUNICION:
        snprintf(buf, tam, "%s x%d", d->nombre, sl->cantidad);
        break;
    case T_MEJORA:
        if (sl->id == IT_VACUNA)              snprintf(buf, tam, "%s (+50 vida máx) x%d", d->nombre, sl->cantidad);
        else if (sl->id == IT_SUERO)          snprintf(buf, tam, "%s (+15 fuerza) x%d", d->nombre, sl->cantidad);
        else if (sl->id == IT_CURA_TEMPRANA)  snprintf(buf, tam, "%s (infección a 0) x%d", d->nombre, sl->cantidad);
        else                                  snprintf(buf, tam, "%s (pasivo)", d->nombre);
        break;
    }
}

const char *nombre_arma_equipada(const Jugador *j)
{
    int s = equipado_en(j, 0);
    return s >= 0 ? ITEMS[j->inv[s].id].nombre : "Puños";
}

/* ---------------- Loot ---------------- */

/* Probabilidad (%) de cada rareza segun el color del bloque:
 *                         comun raro epico legendario */
static const int PESO_RAREZA[5][4] = {
    {70, 25,  5,  0},   /* Celeste */
    {75, 20,  5,  0},   /* Verde   */
    {55, 30, 13,  2},   /* Naranja */
    {40, 33, 22,  5},   /* Rojo    */
    {25, 30, 30, 15},   /* Violeta */
};

int item_aleatorio(int color)
{
    int r = azar(1, 100), rareza = 0, acum = 0, i, n = 0;
    int pool[NUM_ITEMS];
    for (i = 0; i < 4; i++) {
        acum += PESO_RAREZA[color][i];
        if (r <= acum) { rareza = i; break; }
    }
    for (i = 0; i < NUM_ITEMS; i++)
        if ((int)ITEMS[i].rareza == rareza) pool[n++] = i;
    return pool[azar(0, n - 1)];
}

int cantidad_loot(int id)
{
    switch (id) {
    case IT_MUN_9MM:       return azar(4, 10);
    case IT_MUN_ESCOPETA:  return azar(2, 5);
    case IT_MUN_RIFLE:     return azar(5, 15);
    default:               return 1;
    }
}

void recibir_loot(Juego *g, int id, int cant)
{
    if (cant > 1) log_msg(g, "  · %s x%d\n", ITEMS[id].nombre, cant);
    else          log_msg(g, "  · %s\n", ITEMS[id].nombre);
    log_num(g, "    [%s]\n", NOMBRE_RAREZA[ITEMS[id].rareza]);
    if (agregar_item(&g->j, id, cant) < 0)
        log_msg(g, "    (No te entra nada más. Lo dejás ahí.)\n");
}

/* ---------------- Uso de items ---------------- */

/* Devuelve 1 si el item se uso (en combate gasta el turno).
 * Los materiales no se usan aca: llevan a la pantalla de reparar. */
int usar_item(Juego *g, int s)
{
    Jugador *j = &g->j;
    int id = j->inv[s].id;

    switch (ITEMS[id].tipo) {
    case T_CURA:
        if (j->vida >= j->vida_max) { log_msg(g, "No estás herido.\n"); return 0; }
        j->vida = minimo(j->vida_max, j->vida + ITEMS[id].valor);
        log_msg(g, "Usás %s. Te sentís un poco mejor.\n", ITEMS[id].nombre);
        log_num(g, "  [vida %d/%d]\n", j->vida, j->vida_max);
        quitar_slot(j, s, 1);
        return 1;
    case T_MEJORA:
        if (id == IT_VACUNA) {
            j->vida_max += 50; j->vida += 50;
            log_msg(g, "Te inyectás la vacuna. Un calor te recorre el cuerpo entero.\n");
            log_num(g, "  [vida %d/%d]\n", j->vida, j->vida_max);
        } else if (id == IT_SUERO) {
            j->fuerza += 15;
            log_msg(g, "El suero quema al entrar. Tus manos dejan de temblar.\n");
            log_num(g, "  [fuerza %d]\n", j->fuerza);
        } else if (id == IT_CURA_TEMPRANA) {
            j->infeccion = 0;
            log_msg(g, "Te aplicás la Cura Temprana. La fiebre baja.\n");
        } else {
            log_msg(g, "%s no se usa: sirve con tenerlo encima.\n", ITEMS[id].nombre);
            return 0;
        }
        quitar_slot(j, s, 1);
        return 1;
    default:
        return 0;
    }
}

/* Repara el arma del slot indicado con el material de g->slot_material */
int reparar_arma(Juego *g, int s)
{
    Jugador *j = &g->j;
    int m = g->slot_material;
    const ItemDef *arma = &ITEMS[j->inv[s].id];
    j->inv[s].usos = minimo(arma->usos, j->inv[s].usos + ITEMS[j->inv[m].id].valor);
    log_msg(g, "Reparás tu %s con %s.\n", arma->nombre, ITEMS[j->inv[m].id].nombre);
    quitar_slot(j, m, 1);
    return 1;
}
