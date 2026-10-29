/*
 * MODULO DEBUG
 * "Cheats" para probar el juego: agregar items, cambiar stats, d20 fijo
 * y teletransporte. Se esconde poniendo DEBUG_HABILITADO en 0 (juego.h).
 */
#include <stdio.h>
#include "juego.h"

enum { VOLVER = -1 };
enum { D_ITEMS = 1, D_STATS, D_D20, D_TP };
enum {
    S_VIDA_MAS = 1, S_VIDA_MENOS, S_VIDA_LLENA, S_VMAX_MAS, S_FUERZA_MAS, S_FUERZA_MENOS,
    S_INF_MAS, S_INF_MENOS, S_TURNO_MAS, S_TURNO_MENOS
};

void debug_pantalla(Juego *g)
{
    Pantalla *p = &g->pant;
    Jugador *j = &g->j;
    int i;

    switch (g->estado) {
    case E_DEBUG:
        snprintf(p->titulo, sizeof p->titulo, "DEBUG");
        agregar_texto(p, "Herramientas para probar el juego.");
        agregar_opcion(p, D_ITEMS, 0, "Agregar ítems");
        agregar_opcion(p, D_STATS, 0, "Cambiar stats");
        if (d20_fijo) agregar_opcion(p, D_D20, 0, "Always d20 (ahora: %d)", d20_fijo);
        else          agregar_opcion(p, D_D20, 0, "Always d20 (ahora: apagado)");
        agregar_opcion(p, D_TP, g->en_combate, g->en_combate ? "Teletransportarse (no en combate)"
                                                             : "Teletransportarse");
        agregar_opcion(p, VOLVER, 0, "Volver");
        break;

    case E_DEBUG_ITEMS:
        snprintf(p->titulo, sizeof p->titulo, "DEBUG · Agregar ítems");
        agregar_texto(p, "Elegí un ítem.");
        for (i = 0; i < NUM_ITEMS; i++) agregar_opcion(p, i, 0, "%s", ITEMS[i].nombre);
        agregar_opcion(p, VOLVER, 0, "Volver");
        break;

    case E_DEBUG_CANT:
        snprintf(p->titulo, sizeof p->titulo, "DEBUG · %s", ITEMS[g->debug_item].nombre);
        agregar_texto(p, "¿Cuántos?");
        agregar_opcion(p, 1, 0, "x1");
        agregar_opcion(p, 5, 0, "x5");
        agregar_opcion(p, 10, 0, "x10");
        agregar_opcion(p, VOLVER, 0, "Volver");
        break;

    case E_DEBUG_STATS:
        snprintf(p->titulo, sizeof p->titulo, "DEBUG · Stats");
        agregar_texto(p, "Vida %d/%d · Fuerza %d · Infección %d%% · Turno %d",
                      j->vida, j->vida_max, j->fuerza, j->infeccion, j->turno);
        agregar_opcion(p, S_VIDA_MAS, 0, "Vida +10");
        agregar_opcion(p, S_VIDA_MENOS, 0, "Vida -10");
        agregar_opcion(p, S_VIDA_LLENA, 0, "Vida al máximo");
        agregar_opcion(p, S_VMAX_MAS, 0, "Vida máxima +10");
        agregar_opcion(p, S_FUERZA_MAS, 0, "Fuerza +5");
        agregar_opcion(p, S_FUERZA_MENOS, 0, "Fuerza -5");
        agregar_opcion(p, S_INF_MAS, 0, "Infección +10");
        agregar_opcion(p, S_INF_MENOS, 0, "Infección -10");
        agregar_opcion(p, S_TURNO_MAS, 0, "Turno +1");
        agregar_opcion(p, S_TURNO_MENOS, 0, "Turno -1");
        agregar_opcion(p, VOLVER, 0, "Volver");
        break;

    case E_DEBUG_D20:
        snprintf(p->titulo, sizeof p->titulo, "DEBUG · Always d20");
        agregar_texto(p, "Todas las tiradas de d20 van a dar este número.");
        agregar_opcion(p, 0, 0, "Apagado (dado normal)");
        for (i = 1; i <= 20; i++) agregar_opcion(p, i, 0, "Siempre %d", i);
        agregar_opcion(p, VOLVER, 0, "Volver");
        break;

    case E_DEBUG_TP:
        snprintf(p->titulo, sizeof p->titulo, "DEBUG · Teletransporte");
        agregar_texto(p, "Te movés sin gastar turnos ni tirar peligro.");
        for (i = 0; i < NUM_BLOQUES; i++) agregar_opcion(p, i, 0, "%s", mapa[i].nombre);
        agregar_opcion(p, VOLVER, 0, "Volver");
        break;

    default:
        break;
    }
}

void debug_elegir(Juego *g, int v)
{
    Jugador *j = &g->j;

    switch (g->estado) {
    case E_DEBUG:
        if (v == VOLVER)     g->estado = g->volver;
        else if (v == D_ITEMS) g->estado = E_DEBUG_ITEMS;
        else if (v == D_STATS) g->estado = E_DEBUG_STATS;
        else if (v == D_D20)   g->estado = E_DEBUG_D20;
        else if (v == D_TP)    g->estado = E_DEBUG_TP;
        break;

    case E_DEBUG_ITEMS:
        if (v == VOLVER) g->estado = E_DEBUG;
        else { g->debug_item = v; g->estado = E_DEBUG_CANT; }
        break;

    case E_DEBUG_CANT:
        if (v != VOLVER) recibir_loot(g, g->debug_item, v);
        g->estado = E_DEBUG_ITEMS;
        break;

    case E_DEBUG_STATS:
        switch (v) {
        case S_VIDA_MAS:     j->vida = minimo(j->vida_max, j->vida + 10); break;
        case S_VIDA_MENOS:   j->vida = maximo(1, j->vida - 10); break;
        case S_VIDA_LLENA:   j->vida = j->vida_max; break;
        case S_VMAX_MAS:     j->vida_max += 10; break;
        case S_FUERZA_MAS:   j->fuerza += 5; break;
        case S_FUERZA_MENOS: j->fuerza = maximo(0, j->fuerza - 5); break;
        case S_INF_MAS:      j->infeccion = minimo(99, j->infeccion + 10); break;
        case S_INF_MENOS:    j->infeccion = maximo(0, j->infeccion - 10); break;
        case S_TURNO_MAS:    j->turno++; break;
        case S_TURNO_MENOS:  j->turno = maximo(0, j->turno - 1); break;
        default:             g->estado = E_DEBUG; break;
        }
        break;

    case E_DEBUG_D20:
        if (v != VOLVER) d20_fijo = v;
        g->estado = E_DEBUG;
        break;

    case E_DEBUG_TP:
        if (v == VOLVER) { g->estado = E_DEBUG; break; }
        j->bloque_actual = v;
        g->estado = E_BLOQUE;
        log_msg(g, "Te teletransportaste a %s.\n", mapa[v].nombre);
        break;

    default:
        break;
    }
}
