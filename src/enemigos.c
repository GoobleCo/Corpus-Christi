/*
 * MODULO ENEMIGOS
 * Definicion de enemigos, eventos de combate y lo que hace cada uno en su turno.
 *
 * El jugador nunca ve el nombre real del enemigo: el juego los describe
 * como lo que se percibe ("el manifestante", "lo que acecha"...). Los numeros
 * solo aparecen con el modo debug "Mostrar números".
 */
#include <string.h>
#include "juego.h"

/* Devuelve el texto con la primera letra en mayuscula ("el enfermo" -> "El enfermo") */
static const char *mayus(const char *s)
{
    static char buf[64];
    strncpy(buf, s, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    if (buf[0] >= 'a' && buf[0] <= 'z') buf[0] = (char)(buf[0] - 'a' + 'A');
    return buf;
}

Enemigo crear_enemigo(TipoEnemigo t)
{
    Enemigo e;
    memset(&e, 0, sizeof e);
    e.tipo = t;
    e.arma = -1;
    switch (t) {
    case EN_COMUN:
        e.nombre = "Infectado Común";
        if (azar(0, 1)) { e.slug = "infectado_comun";   e.percibido = "el manifestante"; }
        else            { e.slug = "infectado_comun_2"; e.percibido = "el enfermo"; }
        e.vida = VIDA_COMUN; e.fuerza = 10; e.armadura = 0;
        break;
    case EN_POLICIA:
        e.nombre = "Policía Infectado";
        if (azar(0, 1)) { e.slug = "policia_infectado";   e.percibido = "el policía"; }
        else            { e.slug = "policia_infectado_2"; e.percibido = "el antidisturbios"; }
        e.vida = VIDA_POLICIA; e.fuerza = 15; e.armadura = 5;
        break;
    case EN_STALKER:
        e.nombre = "Stalker";           e.percibido = "lo que acecha";
        e.slug = "stalker";
        e.vida = 20; e.fuerza = 5;  e.armadura = 0;
        break;
    case EN_VANDALO: {
        static const int fuego[] = {IT_9MM, IT_ESCOPETA, IT_M16};
        static const int blanca[] = {IT_NAVAJA, IT_CUCHILLO, IT_HACHA, IT_MACHETE};
        e.nombre = "Vándalo";           e.percibido = "la persona armada";
        e.vida = VIDA_VANDALO; e.fuerza = 10; e.armadura = ARMADURA_VANDALO;
        e.arma = azar(0, 1) ? fuego[azar(0, 2)] : blanca[azar(0, 3)];
        /* la imagen muestra el arma que tiene */
        if (e.arma == IT_M16)           e.slug = "vandalo_m16";
        else if (e.arma == IT_ESCOPETA) e.slug = "vandalo_escopeta";
        else if (e.arma == IT_9MM)      e.slug = "vandalo";
        else                            e.slug = "vandalo_escopeta";   /* lleva un cuchillo colgado */
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

/* Cantidad de enemigos por evento: {min, max}
 * (documento: Verde 1-3, Naranja 4-5, Rojo 6-8) */
static const int CANT_INFECTADOS[5][2] = { {0,0}, {1,2}, {2,3}, {3,4}, {4,6} };
static const int CANT_VANDALOS[5][2]   = { {0,0}, {1,1}, {1,2}, {2,2}, {2,3} };

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

/* Describe el daño sin numeros, segun que parte de la vida se perdio */
void aplicar_danio(Juego *g, int d)
{
    Jugador *j = &g->j;
    int pct = j->vida_max > 0 ? d * 100 / j->vida_max : 0;
    j->vida = maximo(0, j->vida - d);
    if (d <= 0)        log_msg(g, "  No te alcanza.\n");
    else if (pct < 8)  log_msg(g, "  Te lastima: apenas un rasguño.\n");
    else if (pct < 16) log_msg(g, "  Te lastima. Duele.\n");
    else if (pct < 28) log_msg(g, "  Te lastima mucho. El golpe te deja sin aire.\n");
    else               log_msg(g, "  Te lastima gravemente. Sentís que algo se rompe.\n");
    log_num(g, "  [-%d vida → %d/%d]\n", d, j->vida, j->vida_max);
    if (j->vida > 0 && j->vida * 100 < j->vida_max * 30)
        log_msg(g, "  Te cuesta mantenerte en pie.\n");
}

void aplicar_infeccion(Juego *g, int ixi)
{
    int prot = proteccion_infeccion(&g->j);
    int gana = maximo(0, ixi - prot);
    if (gana == 0) {
        log_msg(g, "  Tu protección evita que la herida se infecte.\n");
        return;
    }
    g->j.infeccion = minimo(100, g->j.infeccion + gana);
    if (g->j.infeccion >= 60)      log_msg(g, "  La herida se infecta. Tenés fiebre alta y la frente empapada.\n");
    else if (g->j.infeccion >= 30) log_msg(g, "  La herida se infecta. Empezás a tener fiebre.\n");
    else                           log_msg(g, "  La herida arde. Puede que se haya infectado.\n");
    log_num(g, "  [+%d infección (bloqueaste %d) → %d%%]\n", gana, ixi - gana, g->j.infeccion);
}

/* Ataque cuerpo a cuerpo de un enemigo.
 * contagio = % de chance de que transmita la infeccion */
static void golpe_enemigo(Juego *g, int danio, int ixi, int contagio)
{
    /* la armadura resta daño, pero siempre pasa al menos ARMADURA_MIN_PCT % */
    int minimo_pasa = maximo(1, danio * ARMADURA_MIN_PCT / 100);
    aplicar_danio(g, maximo(minimo_pasa, danio - armadura_total(&g->j)));
    if (ixi > 0 && azar(1, 100) <= contagio) aplicar_infeccion(g, ixi);
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
            log_msg(g, "Caés de rodillas y %s se te tira encima. Sentís los dientes.\n", e->percibido);
            golpe_enemigo(g, MORDIDA_DANIO, MORDIDA_INFEC, 100);
        } else {
            log_msg(g, "%s se abalanza sobre vos.\n", mayus(e->percibido));
            golpe_enemigo(g, GOLPE_DANIO, GOLPE_INFEC, GOLPE_CONTAGIO);
        }
        return 0;

    case EN_STALKER:
        if (e->cd_salto > 0) e->cd_salto--;
        if (e->cd_evasion > 0) e->cd_evasion--;
        if (e->vida < 15) {
            log_msg(g, "Escuchás un chillido. Lo que acechaba se pierde entre las sombras.\n");
            return 1;
        }
        if (e->cd_salto == 0) {            /* Salto: cada 3 turnos */
            log_msg(g, "Algo salta desde la oscuridad y cae sobre vos.\n");
            golpe_enemigo(g, SALTO_DANIO, SALTO_INFEC, SALTO_CONTAGIO);
            e->cd_salto = 3;
        } else if (e->cd_evasion == 0 && !e->evadiendo) {   /* Evasión: cada 2 turnos */
            log_msg(g, "Se queda quieto, agazapado. Te está mirando.\n");
            e->evadiendo = 1;
            e->cd_evasion = 2;
        } else {
            log_msg(g, "Lo escuchás moverse a tu alrededor.\n");
        }
        return 0;

    case EN_VANDALO: {
        const ItemDef *a = &ITEMS[e->arma];
        if (a->tipo == T_FUEGO) {
            /* Arma de fuego: daño base + (d20 - armadura) */
            int d20 = tirar_d20();
            log_msg(g, "Un fogonazo. %s te dispara.\n", mayus(e->percibido));
            log_num(g, "  [d20 = %d]\n", d20);
            aplicar_danio(g, maximo(0, a->valor * VANDALO_FUEGO_PCT / 100 + (d20 - armadura_total(j)) / 2));
        } else {
            log_msg(g, "%s te ataca con algo filoso.\n", mayus(e->percibido));
            golpe_enemigo(g, a->valor + e->fuerza, 0, 0);
        }
        return 0;
    }
    }
    return 0;
}

/* Horda: los infectados que esperan detras a veces tambien te alcanzan */
void ataque_horda(Juego *g)
{
    Combate *c = &g->c;
    int i, atacan = 0;
    for (i = c->actual + 1; i < c->n && atacan < HORDA_MAX; i++) {
        Enemigo *e = &c->cola[i];
        if (e->tipo == EN_VANDALO || e->tipo == EN_STALKER) continue;
        atacan++;
        if (azar(1, 100) > HORDA_CHANCE) continue;
        log_msg(g, "%s te alcanza por un costado.\n", mayus(e->percibido));
        golpe_enemigo(g, GOLPE_DANIO, GOLPE_INFEC, GOLPE_CONTAGIO);
        if (g->j.vida <= 0) return;
    }
}
