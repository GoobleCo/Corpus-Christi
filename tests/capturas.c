/* Saca capturas del juego real siguiendo un recorrido fijo (solo para probar). */
#define main main_juego
#include "../src/main.c"
#undef main

static void capturar(const char *nombre)
{
    int k;
    for (k = 0; k < 3; k++) cuadro();
    Image img = LoadImageFromTexture(lienzo.texture);
    ImageFlipVertical(&img);
    ExportImage(img, nombre);
    UnloadImage(img);
}

static void elegir_texto(const char *t)
{
    int i;
    for (i = 0; i < juego.pant.n_op; i++)
        if (strncmp(juego.pant.op[i].texto, t, strlen(t)) == 0) { juego_elegir(&juego, i); return; }
    printf("NO ENCONTRE OPCION: %s (pantalla %s)\n", t, juego.pant.titulo);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    char ruta[256];
    srand(4);
    InitWindow(ANCHO, ALTO, "capturas");
    fuente = cargar_fuente("assets/fuente.ttf");
    fuente_negrita = cargar_fuente("assets/fuente_negrita.ttf");
    lienzo = LoadRenderTexture(ANCHO, ALTO);
    juego_nuevo(&juego);
    strcpy(juego.j.nombre, "José");
    #define CAP(n) snprintf(ruta, sizeof ruta, "%s/%s.png", dir, n); capturar(ruta)
    CAP("01_nombre");
    elegir_texto("Comenzar");          CAP("02_clase");
    elegir_texto("Bombero");           CAP("03_inicio");
    elegir_texto("Investigar");        CAP("04_loot");
    elegir_texto("Moverse");           CAP("05_mover");
    d20_fijo = 1;                       /* forzar combate */
    elegir_texto("Calle Norte 1");     CAP("06_combate");
    elegir_texto("Atacar");            CAP("07_ataque");
    elegir_texto("Abrir inventario");  CAP("08_inventario");
    elegir_texto("Volver");
    d20_fijo = 0;
    while (juego.estado == E_COMBATE) elegir_texto("Atacar");
    CAP("09_despues");
    juego.estado = E_DEBUG; juego.volver = E_BLOQUE; juego_construir_pantalla(&juego);
    elegir_texto("Teletransportarse"); CAP("10_tp_paginas");
    activar_boton(7);                  CAP("11_tp_pagina2");
    juego.j.infeccion = 0; juego.j.turno = 9;
    juego.j.bloque_actual = B_UNIVERSIDAD; juego.estado = E_MOVER; juego_construir_pantalla(&juego);
    elegir_texto("Helipuerto");        CAP("12_final");
    CloseWindow();
    return 0;
}
