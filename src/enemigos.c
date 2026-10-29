/*
 * MODULO ENEMIGOS
 * Definicion de enemigos, eventos de combate y lo que hace cada uno en su turno.
 */
#include <string.h>
#include "juego.h"

Enemigo crear_enemigo(TipoEnemigo t)
{
    Enemigo e;
    memset(&e, 0, sizeof e);
    e.tipo = t;
    e.arma = -1;
    switch (t) {
    case EN_COMUN:
        e.nombre = "Infectado Común";   e.slug = "infectado_comun";
        e.vida = 40; e.fuerza = 10; e.armadura = 0;
        break;
    case EN_POLICIA:
        e.nombre = "Policía Infectado"; e.slug = "policia_infectado";
        e.vida = 50; e.fuerza = 15; e.armadura = 5;
        break;
    case EN_STALKER:
        e.nombre = "Stalker";           e.slug = "stalker";
        e.vida = 20; e.fuerza = 5;  e.armadura = 0;
        break;
    case EN_VANDALO: {
        static const int fuego[] = {IT_9MM, IT_ESCOPETA, IT_M16};
        static const int blanca[] = {IT_NAVAJA, IT_CUCHILLO, IT_HACHA, IT_MACHETE};
        e.nombre = "Vándalo";           e.slug = "vandalo";
        e.vida = 45; e.fuerza = 10; e.armadura = 9;
        e.arma = azar(0, 1) ? fuego[azar(0, 2)] : blanca[azar(0, 3)];
        break;
    }
    }
    e.vida_max = e.vida;
    return e;
}

/* Probabilidad (%) de cada infectado segun el color:  comun policia stalker
 * (los enemigos raros se vuelven mas comunes en bloques mas dificiles) */
static const int PESO_INFECTADO[5][3] = {
    {100,  0,  0},  /* Celeste (no se usa) */
    { 80, 17,  3},  /* Verde   */
    { 65, 25, 10},  /* Naranja */
    { 50, 30, 20},  /* Rojo    */
    { 35, 35, 30},  /* Violeta */
};

/* Cantidad de enemigos por evento: {min, max} */
static const int CANT_INFECTADOS[5][2] = { {0,0}, {1,3}, {4,5}, {6,8}, {8,10} };
static const int CANT_VANDALOS[5][2]   = { {0,0}, {1,1}, {1,2}, {2,3}, {3,4} };

int cantidad_enemigos(int color, int subterraneo)
{
    const int *r = subterraneo ? CANT_VANDALOS[color] : CANT_INFECTADOS[color];
    return azar(r[0], r[1]);
}

Enemigo enemigo_aleatorio(int color, int subterraneo)
{
    int r, i, acum = 0;
    if (subterraneo) return crear_enemigo(EN_VANDALO);
    r = azar(1, 100);
    for (i = 0; i < 3; i++) {
        acum += PESO_INFECTADO[color][i];
        if (r <= acum) return crear_enemigo((TipoEnemigo)i);
    }
    return crear_enemigo(EN_COMUN);
}

void aplicar_danio(Juego *g, int d)
{
    g->j.vida = maximo(0, g->j.vida - d);
    log_msg(g, "  Recibís %d de daño. (Vida: %d/%d)\n", d, g->j.vida, g->j.vida_max);
}

void aplicar_infeccion(Juego *g, int ixi)
{
    int prot = proteccion_infeccion(&g->j);
    int gana = maximo(0, ixi - prot);
    if (gana == 0) { log_msg(g, "  Tu protección bloqueó la infección.\n"); return; }
    g->j.infeccion = minimo(100, g->j.infeccion + gana);
    log_msg(g, "  Infección +%d (bloqueaste %d). Infección: %d%%\n", gana, ixi - gana, g->j.infeccion);
}

/* Ataque cuerpo a cuerpo de un enemigo: la armadura resta daño (mínimo 1) */
static void golpe_enemigo(Juego *g, int danio, int ixi)
{
    aplicar_danio(g, maximo(1, danio - armadura_total(&g->j)));
    if (ixi > 0) aplicar_infeccion(g, ixi);
}

/* Turno del enemigo. Devuelve 1 si el enemigo abandona el combate. */
int turno_enemigo(Juego *g, Enemigo *e)
{
    Jugador *j = &g->j;
    switch (e->tipo) {
    case EN_COMUN:
    case EN_POLICIA:
        /* Mordida: solo si el jugador tiene menos del 30% de su vida */
        if (j->vida * 100 < j->vida_max * 30) {
            log_msg(g, "▶ ¡%s te MUERDE!\n", e->nombre);
            golpe_enemigo(g, 20, 20);
        } else {
            log_msg(g, "▶ %s te golpea.\n", e->nombre);
            golpe_enemigo(g, 15, 10);
        }
        return 0;

    case EN_STALKER:
        if (e->cd_salto > 0) e->cd_salto--;
        if (e->cd_evasion > 0) e->cd_evasion--;
        if (e->vida < 15) {
            log_msg(g, "▶ El Stalker, herido, se escabulle entre las sombras y desaparece.\n");
            return 1;
        }
        if (e->cd_salto == 0) {            /* Salto: cada 3 turnos */
            log_msg(g, "▶ ¡El Stalker SALTA sobre vos!\n");
            golpe_enemigo(g, 25, 10);
            e->cd_salto = 3;
        } else if (e->cd_evasion == 0 && !e->evadiendo) {   /* Evasión: cada 2 turnos */
            log_msg(g, "▶ El Stalker se agacha, listo para esquivar tu próximo ataque.\n");
            e->evadiendo = 1;
            e->cd_evasion = 2;
        } else {
            log_msg(g, "▶ El Stalker te rodea, acechando...\n");
        }
        return 0;

    case EN_VANDALO: {
        const ItemDef *a = &ITEMS[e->arma];
        if (a->tipo == T_FUEGO) {
            /* Arma de fuego: daño base + (d20 - armadura) */
            int d20 = tirar_d20();
            log_msg(g, "▶ El Vándalo te dispara con su %s (d20: %d).\n", a->nombre, d20);
            aplicar_danio(g, maximo(0, a->valor + (d20 - armadura_total(j))));
        } else {
            log_msg(g, "▶ El Vándalo te ataca con su %s.\n", a->nombre);
            golpe_enemigo(g, a->valor + e->fuerza, 0);
        }
        return 0;
    }
    }
    return 0;
}
