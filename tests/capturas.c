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
    juego_construir_pantalla(&juego);
    #define CAP(n) snprintf(ruta, sizeof ruta, "%s/%s.png", dir, n); capturar(ruta)
    CAP("01_nombre");
    elegir_texto("Comenzar");          CAP("02_clase");
    elegir_texto("Bombero");           CAP("02b_radio");
    elegir_texto("Salir del refugio"); CAP("03_inicio");
    elegir_texto("Investigar");        CAP("04_loot");
    elegir_texto("Moverse");           CAP("05_mover");
    d20_fijo = 1;                       /* forzar combate */
    elegir_texto("Calle Norte 1");     CAP("06_combate");
    elegir_texto("Atacar");            CAP("07_ataque");
    elegir_texto("Inventario");        CAP("08_inventario");
    elegir_texto("Volver");
    d20_fijo = 0;
    while (juego.estado == E_COMBATE) elegir_texto("Atacar");
    CAP("09_despues");
    elegir_texto("Ver estado");        CAP("09b_estado");
    elegir_texto("Volver");
    juego.estado = E_DEBUG; juego.volver = E_BLOQUE; juego_construir_pantalla(&juego);
    elegir_texto("Teletransportarse"); CAP("10_tp_paginas");
    activar_boton(7);                  CAP("11_tp_pagina2");
    juego.j.infeccion = 0; juego.j.turno = 9;
    juego.j.bloque_actual = B_UNIVERSIDAD; juego.estado = E_MOVER; juego_construir_pantalla(&juego);
    elegir_texto("Helipuerto");        CAP("12_final");
    /* VARIANTES: una captura por imagen de enemigo */
    {
        static const char *slugs[] = {"infectado_comun","infectado_comun_2","policia_infectado","policia_infectado_2","stalker","vandalo","vandalo_escopeta","vandalo_m16"};
        int k;
        juego.en_combate = 1; juego.estado = E_COMBATE; juego.c.n = 1; juego.c.actual = 0;
        juego.j.bloque_actual = B_CN1;
        for (k = 0; k < 8; k++) {
            juego.c.cola[0] = crear_enemigo(k < 2 ? EN_COMUN : k < 4 ? EN_POLICIA : k == 4 ? EN_STALKER : EN_VANDALO);
            juego.c.cola[0].slug = slugs[k];
            juego.msg[0] = 0; juego_construir_pantalla(&juego);
            snprintf(ruta, sizeof ruta, "%s/e_%s.png", dir, slugs[k]); capturar(ruta);
        }
    }
    juego.en_combate = 0; juego.estado = E_NOMBRE; juego_construir_pantalla(&juego);
    CAP("00_portada");
    /* NUEVAS: checkpoint con los dos NPC, charla, heridas, patrulla */
    juego.j.vida = juego.j.vida_max; juego.j.bloque_actual = B_CHECK_N; juego.estado = E_BLOQUE;
    juego.msg[0] = 0; juego_construir_pantalla(&juego); CAP("20_checkpoint");
    elegir_texto("Hablar con el soldado"); CAP("21_charla");
    juego.j.bloque_actual = B_CN1; juego.estado = E_BLOQUE; juego.j.turno = 9;
    juego.j.vida = juego.j.vida_max / 2; juego.msg[0] = 0; juego_construir_pantalla(&juego); CAP("22_herido");
    juego.j.vida = juego.j.vida_max / 6; juego_construir_pantalla(&juego); CAP("23_muy_herido");
    juego.j.vida = juego.j.vida_max; juego.patrulla = 1; juego.msg[0] = 0;
    log_msg(&juego, "Una patrulla de H.E.R.O. cruza la calle a paso rápido. Uno de los soldados se frena, te mira un segundo y te tira algo antes de seguir.\n  · Botiquín\n«Suerte.»\n");
    juego_construir_pantalla(&juego); CAP("24_patrulla");
    juego.patrulla = 0;
    /* COLA: pelea con varios esperando detras */
    {
        static const TipoEnemigo t[] = {EN_COMUN, EN_POLICIA, EN_COMUN, EN_STALKER, EN_POLICIA};
        int k;
        juego.j.bloque_actual = B_CS3; juego.en_combate = 1; juego.estado = E_COMBATE;
        juego.c.n = 5; juego.c.actual = 0;
        for (k = 0; k < 5; k++) juego.c.cola[k] = crear_enemigo(t[k]);
        juego.msg[0] = 0; juego_construir_pantalla(&juego); CAP("13_cola5");
        juego.c.actual = 3; juego_construir_pantalla(&juego); CAP("14_cola2");
        juego.en_combate = 0;
    }
    /* BIOMAS */
    {
        static const int bl[] = {B_CN2, B_CHECK_N, B_NW_METRO, B_TUNEL_A, B_INICIO, B_CALLEJONES_N1, B_ALC1, B_PUERTO, B_ESTADIO, B_UNIVERSIDAD, B_ALMACEN, B_MALL};
        int k;
        for (k = 0; k < 12; k++) {
            juego.j.bloque_actual = bl[k]; juego.estado = E_BLOQUE; juego.msg[0] = 0;
            juego_construir_pantalla(&juego);
            snprintf(ruta, sizeof ruta, "%s/b_%s.png", dir, mapa[bl[k]].slug); capturar(ruta);
        }
    }
    CloseWindow();
    return 0;
}
