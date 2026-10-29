/* Prueba automatica: juega miles de partidas eligiendo opciones al azar. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/juego.h"

int main(int argc, char **argv)
{
    static Juego g;
    int partidas = argc > 1 ? atoi(argv[1]) : 2000, p, paso, finales[NUM_FINALES] = {0}, i;
    long pasos_total = 0;
    for (p = 0; p < partidas; p++) {
        srand(p * 7919 + 1);
        juego_nuevo(&g);
        for (paso = 0; paso < 3000; paso++) {
            int hab = 0, k, elegido;
            for (k = 0; k < g.pant.n_op; k++) if (!g.pant.op[k].deshabilitada) hab++;
            if (hab == 0) { printf("SIN OPCIONES p=%d estado=%d titulo=%s\n", p, g.estado, g.pant.titulo); return 1; }
            if (g.estado == E_FINAL) { finales[g.final]++; break; }
            /* preferir no ir a debug y elegir al azar */
            do { elegido = rand() % g.pant.n_op; } while (g.pant.op[elegido].deshabilitada ||
                    (strcmp(g.pant.op[elegido].texto, "Debug") == 0 && rand() % 10));
            juego_elegir(&g, elegido);
            if (g.j.vida < 0 || g.j.infeccion > 100 || g.j.n_inv > MAX_INV) { printf("INVARIANTE p=%d\n", p); return 1; }
        }
        pasos_total += paso;
    }
    for (i = 0; i < NUM_FINALES; i++) printf("%-18s %d\n", final_titulo(i), finales[i]);
    printf("pasos promedio %ld\n", pasos_total / partidas);
    return 0;
}
