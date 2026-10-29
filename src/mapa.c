/*
 * MODULO BASE: MAPA
 * Todos los bloques de la ciudad y como se conectan (de ida y vuelta).
 *
 * Cada bloque usa la imagen assets/bloques/<slug>.png
 * Colores: Celeste (sin peligro), Verde 8, Naranja 13, Rojo 16, Violeta 20.
 * Los bloques subterraneos (metros, tuneles, alcantarillas) tienen Vandalos;
 * los de superficie tienen Infectados.
 */
#include "juego.h"

const char *NOMBRE_COLOR[5] = {"Celeste", "Verde", "Naranja", "Rojo", "Violeta"};
const int PELIGRO_COLOR[5] = {0, 8, 13, 16, 20};

/*  slug, nombre, color, subterraneo, especial, protegido, item garantizado, descripcion */
Bloque mapa[NUM_BLOQUES] = {
    [B_INICIO] = {"inicio", "Inicio", C_CELESTE, 0, ESP_INICIO, -1, IT_NAVAJA,
        "El refugio improvisado donde pasaste la noche. Por la radio, HERO anuncia la "
        "evacuación: un helicóptero al norte, un tren al oeste y un barco al sur."},

    /* ---------------- NORTE ---------------- */
    [B_CN1] = {"calle_norte_1", "Calle Norte 1", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Autos abandonados con las puertas abiertas. Una alarma suena a lo lejos."},
    [B_CN2] = {"calle_norte_2", "Calle Norte 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Las vidrieras están rotas y hay manchas de sangre en el asfalto. Al fondo se ven los reflectores de HERO."},
    [B_CHECK_N] = {"checkpoint_norte", "H.E.R.O. Checkpoint Norte", C_CELESTE, 0, ESP_CHECKPOINT, B_CN3, -1,
        "Vallas, reflectores y soldados con trajes protectores. Más allá, el camino a la Universidad."},
    [B_CN3] = {"calle_norte_3", "Calle Norte 3", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Una calle limpia, patrullada. Se escuchan las aspas de un helicóptero."},
    [B_UNIVERSIDAD] = {"universidad", "Universidad", C_CELESTE, 0, ESP_NINGUNO, -1, -1,
        "HERO convirtió el campus en un puesto de mando. Civiles hacen fila hacia el helipuerto."},
    [B_HELIPUERTO] = {"helipuerto", "Helipuerto", C_CELESTE, 0, ESP_HELIPUERTO, -1, -1,
        "La terraza del edificio principal. Es tu última oportunidad."},
    [B_METRO_NORTE] = {"metro_norte", "Metro Norte", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Una estación a medio terminar. Sus escaleras salen del otro lado del checkpoint."},
    [B_TUNEL_A] = {"tunel_a", "Túnel de Mantenimiento A", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Cables colgando y charcos de aceite. Alguien dejó marcas de tiza en las paredes."},
    [B_MALL] = {"mall", "Mall", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "El shopping está a oscuras. Entre las góndolas se mueven siluetas."},
    [B_CALLEJONES_N1] = {"callejones_norte_1", "Callejones Norte 1", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Pasillos angostos entre edificios. Cualquier cosa puede salir de una puerta."},
    [B_CALLEJONES_N2] = {"callejones_norte_2", "Callejones Norte 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Contenedores volcados. Por un hueco en la pared se ve el checkpoint."},
    [B_ESTADIO] = {"estadio", "Estadio (evacuado)", C_ROJO, 0, ESP_NINGUNO, -1, IT_PASE,
        "HERO evacuó el estadio a las apuradas. Las tribunas no están vacías. No hay otra salida."},

    /* ---------------- OESTE ---------------- */
    [B_NW_METRO] = {"northwest_metro", "Northwest Metro", C_VERDE, 1, ESP_NINGUNO, -1, -1,
        "Molinetes rotos y carteles que todavía anuncian el próximo tren."},
    [B_FABRICA] = {"fabrica", "Fábrica", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Las máquinas siguen encendidas. El ruido tapa cualquier paso."},
    [B_DISTRITO] = {"distrito_comercial", "Distrito Comercial", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Oficinas saqueadas y papeles volando por la avenida."},
    [B_HOSPITAL] = {"hospital", "Hospital", C_VIOLETA, 0, ESP_NINGUNO, -1, IT_BOTIQUIN,
        "Camillas en la vereda. Acá empezó todo, y acá es donde más infectados quedan."},
    [B_CALLEJON] = {"callejon", "Callejón", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Un pasaje oscuro detrás del hospital que termina en las vías del tren."},
    [B_CO1] = {"calle_oeste_1", "Calle Oeste 1", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Una avenida ancha. Hay huellas de neumáticos de los camiones de HERO."},
    [B_CO2] = {"calle_oeste_2", "Calle Oeste 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Un colectivo cruzado bloquea media calle. Del otro lado, el checkpoint oeste."},
    [B_CHECK_O] = {"checkpoint_oeste", "H.E.R.O. Checkpoint Oeste", C_CELESTE, 0, ESP_CHECKPOINT, B_TREN, -1,
        "Soldados controlan el acceso a la estación de tren."},
    [B_TREN] = {"tren", "Tren", C_CELESTE, 0, ESP_TREN, -1, -1,
        "Una multitud desesperada rodea el último tren de HERO."},
    [B_SW_METRO] = {"southwest_metro", "Southwest Metro", C_VERDE, 1, ESP_NINGUNO, -1, -1,
        "Una estación de subte con fogatas apagadas. Alguien vive acá abajo."},
    [B_TUNEL_D] = {"tunel_d", "Túnel de Mantenimiento D", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Grafitis frescos marcan territorio de una banda."},
    [B_HIDEOUT] = {"vandal_hideout", "Vandal Hideout", C_VIOLETA, 1, ESP_NINGUNO, -1, -1,
        "La guarida de los vándalos. Cajas de botín apiladas y armas por todos lados."},
    [B_TUNEL_C] = {"tunel_c", "Túnel de Mantenimiento C", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "El agua llega hasta los tobillos y sigue subiendo hacia el sur."},
    [B_INUNDACION] = {"inundacion", "Inundación", C_ROJO, 1, ESP_NINGUNO, -1, -1,
        "Un tramo inundado. Hay que avanzar con el agua hasta la cintura."},
    [B_ALC1] = {"alcantarillas_1", "Alcantarillas 1", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "El olor es insoportable. Las ratas huyen de algo."},
    [B_ALC2] = {"alcantarillas_2", "Alcantarillas 2", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Pasarelas oxidadas sobre un canal de agua negra."},
    [B_ALC3] = {"alcantarillas_3", "Alcantarillas 3", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Se siente olor a sal. La salida al puerto está cerca."},
    [B_PUERTO_DESTRUIDO] = {"puerto_destruido", "Puerto Destruido", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "Muelles derrumbados y grúas caídas. Los infectados deambulan entre los escombros."},

    /* ---------------- SUR ---------------- */
    [B_CS1] = {"calle_sur_1", "Calle Sur 1", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Persianas bajas y un semáforo que titila en amarillo."},
    [B_CS2] = {"calle_sur_2", "Calle Sur 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Una barricada civil abandonada. Quien la armó no tuvo suerte."},
    [B_CS3] = {"calle_sur_3", "Calle Sur 3", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "La última calle antes del puerto. La horda se junta acá."},
    [B_CHECK_S] = {"checkpoint_sur", "H.E.R.O. Checkpoint Sur", C_CELESTE, 0, ESP_CHECKPOINT, B_PUERTO, -1,
        "Contenedores apilados como muralla. Detrás, el puerto."},
    [B_PUERTO] = {"puerto", "Puerto", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "El barco de HERO todavía está amarrado. La pasarela sigue baja."},
    [B_BARCO] = {"barco", "Barco", C_CELESTE, 0, ESP_BOTE, -1, -1,
        "Subís por la pasarela antes de que la levanten."},
    [B_INVERNADERO] = {"invernadero", "Invernadero", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Plantas por todos lados y vidrios empañados. Extrañamente tranquilo."},
    [B_ALMACEN] = {"almacen", "Almacén", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Un depósito lleno de cajas. Algunas fueron abiertas a la fuerza."},
    [B_GUARDA_BARCOS] = {"guarda_barcos", "Guarda Barcos", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Lanchas colgadas del techo. Una rampa lleva directo al puerto."},

    /* ---------------- ESTE ---------------- */
    [B_CE1] = {"calle_este_1", "Calle Este 1", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Bicicletas tiradas y un puesto de diarios dado vuelta."},
    [B_CE2] = {"calle_este_2", "Calle Este 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Patrulleros abandonados con las sirenas apagadas."},
    [B_CHECK_E] = {"checkpoint_este", "H.E.R.O. Checkpoint (cerrado)", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "El checkpoint fue abandonado. Las vallas están soldadas y no hay forma de pasar."},
    [B_TURISTA1] = {"calle_turista_1", "Calle Turista 1", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Restaurantes con las mesas todavía servidas."},
    [B_TURISTA2] = {"calle_turista_2", "Calle Turista 2", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "Un micro de turistas volcado. Sus pasajeros siguen cerca."},
    [B_HOTEL] = {"hotel", "Hotel", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "El lobby es un desastre. Se escuchan golpes en los pisos de arriba."},
    [B_ESTACIONAMIENTO] = {"estacionamiento", "Estacionamiento", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Un subsuelo de autos. Algunos todavía tienen las llaves puestas."},
    [B_CE3] = {"calle_este_3", "Calle Este 3", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "La autopista hacia el aeropuerto, cubierta de autos chocados."},
    [B_AEROPUERTO] = {"aeropuerto", "Aeropuerto", C_ROJO, 0, ESP_SIN_ESCAPE, -1, -1,
        "No hay aviones. Solo una terminal llena de infectados. Si te encuentran acá, no hay escape."},
    [B_NE_METRO] = {"northeast_metro", "Northeast Metro", C_VERDE, 1, ESP_NINGUNO, -1, -1,
        "Un andén silencioso. Las luces de emergencia parpadean."},
    [B_TUNEL_B] = {"tunel_b", "Túnel de Mantenimiento B", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Un pasillo de servicio con puertas de metal a los costados."},
    [B_BLOQUEO] = {"bloqueo", "Bloqueo", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Un derrumbe tapa el túnel por completo. No se puede seguir."},
    [B_CALLEJONES_E1] = {"callejones_este_1", "Callejones Este 1", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Escaleras de incendio y ropa colgada. Una salida del túnel da acá."},
};

static void conectar(int a, int b)
{
    mapa[a].conexiones[mapa[a].n_con++] = b;
    mapa[b].conexiones[mapa[b].n_con++] = a;
}

void iniciar_mapa(void)
{
    int i;
    for (i = 0; i < NUM_BLOQUES; i++) {
        mapa[i].n_con = 0;
        mapa[i].looteado = 0;
    }

    /* desde el inicio */
    conectar(B_INICIO, B_CN1);
    conectar(B_INICIO, B_CE1);
    conectar(B_INICIO, B_CS1);
    conectar(B_INICIO, B_CO1);
    conectar(B_INICIO, B_NW_METRO);
    conectar(B_INICIO, B_NE_METRO);
    conectar(B_INICIO, B_SW_METRO);

    /* norte */
    conectar(B_CN1, B_CN2);
    conectar(B_CN1, B_MALL);
    conectar(B_MALL, B_CALLEJONES_N1);
    conectar(B_CALLEJONES_N1, B_CALLEJONES_N2);
    conectar(B_CALLEJONES_N1, B_CALLEJONES_E1);
    conectar(B_CALLEJONES_N2, B_ESTADIO);
    conectar(B_CALLEJONES_N2, B_CHECK_N);
    conectar(B_CN2, B_CHECK_N);
    conectar(B_CN2, B_TUNEL_A);
    conectar(B_TUNEL_A, B_METRO_NORTE);
    conectar(B_METRO_NORTE, B_CN3);
    conectar(B_CHECK_N, B_CN3);
    conectar(B_CN3, B_UNIVERSIDAD);
    conectar(B_UNIVERSIDAD, B_HELIPUERTO);

    /* oeste */
    conectar(B_NW_METRO, B_TUNEL_A);
    conectar(B_NW_METRO, B_FABRICA);
    conectar(B_FABRICA, B_DISTRITO);
    conectar(B_DISTRITO, B_HOSPITAL);
    conectar(B_HOSPITAL, B_CALLEJON);
    conectar(B_HOSPITAL, B_CO2);
    conectar(B_CALLEJON, B_TREN);
    conectar(B_CO1, B_CO2);
    conectar(B_CO2, B_CHECK_O);
    conectar(B_CHECK_O, B_TREN);
    conectar(B_CO2, B_TUNEL_D);
    conectar(B_SW_METRO, B_TUNEL_D);
    conectar(B_SW_METRO, B_TUNEL_C);
    conectar(B_TUNEL_D, B_HIDEOUT);
    conectar(B_TUNEL_C, B_TUNEL_D);
    conectar(B_TUNEL_C, B_INUNDACION);
    conectar(B_INUNDACION, B_ALC1);
    conectar(B_ALC1, B_ALC2);
    conectar(B_ALC2, B_ALC3);
    conectar(B_ALC3, B_PUERTO_DESTRUIDO);
    conectar(B_PUERTO_DESTRUIDO, B_PUERTO);

    /* sur */
    conectar(B_CS1, B_SW_METRO);
    conectar(B_CS1, B_CS2);
    conectar(B_CS1, B_TURISTA1);
    conectar(B_CS2, B_CS3);
    conectar(B_CS2, B_INVERNADERO);
    conectar(B_CS3, B_ALMACEN);
    conectar(B_CS3, B_CHECK_S);
    conectar(B_CHECK_S, B_PUERTO);
    conectar(B_PUERTO, B_BARCO);
    conectar(B_INVERNADERO, B_ALMACEN);
    conectar(B_ALMACEN, B_GUARDA_BARCOS);
    conectar(B_GUARDA_BARCOS, B_PUERTO);

    /* este */
    conectar(B_CE1, B_CE2);
    conectar(B_CE1, B_TURISTA1);
    conectar(B_CE2, B_CHECK_E);
    conectar(B_CE2, B_HOTEL);
    conectar(B_TURISTA1, B_INVERNADERO);
    conectar(B_TURISTA1, B_TURISTA2);
    conectar(B_TURISTA2, B_HOTEL);
    conectar(B_HOTEL, B_ESTACIONAMIENTO);
    conectar(B_ESTACIONAMIENTO, B_CE3);
    conectar(B_CE3, B_AEROPUERTO);
    conectar(B_NE_METRO, B_TUNEL_B);
    conectar(B_TUNEL_B, B_CALLEJONES_E1);
    conectar(B_TUNEL_B, B_BLOQUEO);
}
