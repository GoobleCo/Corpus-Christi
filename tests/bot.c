/* Bot que juega "bien" para medir el balance: va por el camino mas corto
 * a una salida, revisa cada bloque, se cura cuando esta herido y usa la
 * mejor arma. Uso: ./bot [partidas] [destino: heli|tren|barco] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/juego.h"

static Juego g;

static int idx_valor(int v)
{
    int i;
    for (i = 0; i < g.pant.n_op; i++)
        if (g.pant.op[i].valor == v && !g.pant.op[i].deshabilitada) return i;
    return -1;
}
/* BFS desde el bloque actual: devuelve el primer paso hacia 'meta' */
static int primer_paso(int desde, int meta, int con_pase)
{
    int padre[NUM_BLOQUES], cola[NUM_BLOQUES], ini = 0, fin = 0, i, b;
    for (i = 0; i < NUM_BLOQUES; i++) padre[i] = -2;
    padre[desde] = -1; cola[fin++] = desde;
    while (ini < fin) {
        b = cola[ini++];
        for (i = 0; i < mapa[b].n_con; i++) {
            int d = mapa[b].conexiones[i];
            if (padre[d] != -2) continue;
            if (mapa[b].especial == ESP_CHECKPOINT && mapa[b].protegido == d && !con_pase) continue;
            if (mapa[d].especial == ESP_SIN_ESCAPE) continue;
            if (buscar_paso(b, d) && !buscar_paso(b, d)->abierto && buscar_paso(b, d)->fuerza > g.j.fuerza) continue;
            padre[d] = b; cola[fin++] = d;
        }
    }
    if (padre[meta] == -2) return -1;
    b = meta;
    while (padre[b] != desde) b = padre[b];
    return b;
}

static void equipar_mejor(void)
{
    int i, mejor = -1, mejor_d = -1, arm = -1, mejor_a = -1;
    for (i = 0; i < g.j.n_inv; i++) {
        const ItemDef *d = &ITEMS[g.j.inv[i].id];
        int dmg = -1;
        if (d->tipo == T_MELEE) dmg = d->valor + g.j.fuerza;
        if (d->tipo == T_FUEGO && contar_item(&g.j, d->municion) > 0) dmg = d->valor + 10;
        if (dmg > mejor_d) { mejor_d = dmg; mejor = i; }
        if (d->tipo == T_ARMADURA && d->valor * 2 + d->anti_infec > mejor_a) { mejor_a = d->valor * 2 + d->anti_infec; arm = i; }
    }
    if (mejor >= 0 && !g.j.inv[mejor].equipado) equipar(&g.j, mejor);
    if (arm >= 0 && !g.j.inv[arm].equipado) equipar(&g.j, arm);
    i = buscar_item(&g.j, IT_MASCARA_GAS);
    if (i < 0) i = buscar_item(&g.j, IT_BARBIJO);
    if (i >= 0 && !g.j.inv[i].equipado) equipar(&g.j, i);
}

static int cura_disponible(void)
{
    static const int curas[] = {IT_VENDAJE, IT_PILDORAS, IT_BOTIQUIN};
    int k;
    for (k = 0; k < 3; k++) if (buscar_item(&g.j, curas[k]) >= 0) return curas[k];
    return -1;
}

int main(int argc, char **argv)
{
    int partidas = argc > 1 ? atoi(argv[1]) : 2000;
    const char *dest = argc > 2 ? argv[2] : "heli";
    int fin[NUM_FINALES][4] = {{0}}, p, c, f, paso;
    long turnos[4] = {0}, gan[4] = {0};
    long inf_check_sum[4] = {0}, inf_check_n[4] = {0};
    for (c = 0; c < 4; c++) for (p = 0; p < partidas; p++) {
        srand(p * 104729 + c * 7 + 3);
        juego_nuevo(&g);
        juego_elegir(&g, 0);            /* nombre */
        juego_elegir(&g, idx_valor(c)); /* clase */
        for (paso = 0; paso < 5000 && g.estado != E_FINAL; paso++) {
            int meta, i;
            g.estado = g.estado; /* */
            if (g.estado == E_BLOQUE) {
                equipar_mejor();
                if (!mapa[g.j.bloque_actual].looteado) { juego_elegir(&g, idx_valor(1)); continue; }
                if (g.j.vida * 100 < g.j.vida_max * 55 && (i = cura_disponible()) >= 0) {
                    usar_item(&g, buscar_item(&g.j, i)); continue;
                }
                if (buscar_item(&g.j, IT_CURA_TEMPRANA) >= 0 && g.j.infeccion > 15) { usar_item(&g, buscar_item(&g.j, IT_CURA_TEMPRANA)); continue; }
                if (buscar_item(&g.j, IT_VACUNA) >= 0) { usar_item(&g, buscar_item(&g.j, IT_VACUNA)); continue; }
                if (buscar_item(&g.j, IT_SUERO) >= 0) { usar_item(&g, buscar_item(&g.j, IT_SUERO)); continue; }
                juego_elegir(&g, idx_valor(2));   /* mover */
                continue;
            }
            if (g.estado == E_MOVER) {
                int pase = contar_item(&g.j, IT_PASE) > 0;
                if (!strcmp(dest, "heli")) meta = g.j.turno < 20 ? B_HELIPUERTO : B_BARCO;
                else if (!strcmp(dest, "tren")) meta = B_TREN;
                else meta = B_BARCO;
                i = primer_paso(g.j.bloque_actual, meta, pase);
                /* si no hay camino (falta el pase o la fuerza), ir a buscar el pase al Estadio */
                if (i < 0 && !pase && g.j.bloque_actual != B_ESTADIO)
                    i = primer_paso(g.j.bloque_actual, B_ESTADIO, pase);
                if (i < 0 || idx_valor(i) < 0) i = g.pant.op[0].valor;
                if (mapa[g.j.bloque_actual].especial == ESP_CHECKPOINT && i != -1) {
                    inf_check_sum[c] += g.j.infeccion; inf_check_n[c]++;
                }
                juego_elegir(&g, idx_valor(i) >= 0 ? idx_valor(i) : 0);
                continue;
            }
            if (g.estado == E_COMBATE) {
                if (g.j.vida * 100 < g.j.vida_max * 35 && (i = cura_disponible()) >= 0) {
                    g.volver = E_COMBATE; g.estado = E_USAR; juego_construir_pantalla(&g);
                    juego_elegir(&g, idx_valor(buscar_item(&g.j, i)));
                    continue;
                }
                equipar_mejor(); juego_construir_pantalla(&g);
                i = idx_valor(1);
                if (i < 0) { equipar(&g.j, 0); juego_construir_pantalla(&g); i = 0; }
                juego_elegir(&g, i);
                continue;
            }
            /* cualquier otra pantalla: volver */
            i = idx_valor(-1);
            juego_elegir(&g, i >= 0 ? i : 0);
        }
        f = g.final;
        fin[f][c]++;
        turnos[c] += g.j.turno;
        if (f == F_HELICOPTERO || f == F_BOTE || f == F_TREN || f == F_INFECCION_TARDIA) gan[c]++;
    }
    printf("destino=%s  partidas por clase=%d\n%-18s %7s %7s %7s %7s\n", dest, partidas, "", "Vago", "Bombero", "Param", "Policia");
    for (f = 0; f < NUM_FINALES; f++) {
        printf("%-18s", final_titulo(f));
        for (c = 0; c < 4; c++) printf(" %6.1f%%", 100.0 * fin[f][c] / partidas);
        printf("\n");
    }
    printf("%-18s", "escapa (cualquier)"); for (c = 0; c < 4; c++) printf(" %6.1f%%", 100.0 * gan[c] / partidas); printf("\n");
    printf("%-18s", "inf. en checkpoint"); for (c = 0; c < 4; c++) printf(" %7.1f", inf_check_n[c] ? (double)inf_check_sum[c] / inf_check_n[c] : 0); printf("\n");
    printf("%-18s", "turnos prom"); for (c = 0; c < 4; c++) printf(" %7.1f", (double)turnos[c] / partidas); printf("\n");
    return 0;
}
