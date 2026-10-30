/*
 * MODULO BASE: MAPA
 * Todos los bloques de la ciudad y como se conectan (de ida y vuelta).
 *
 * Cada bloque usa la imagen assets/lugares/<slug>.png
 * Colores: Celeste (sin peligro), Verde 8, Naranja 13, Rojo 16, Violeta 20.
 * Los bloques subterraneos (metros, tuneles, alcantarillas) tienen Vandalos;
 * los de superficie tienen Infectados.
 */
#include <stddef.h>
#include "juego.h"

const char *NOMBRE_COLOR[5] = {"Celeste", "Verde", "Naranja", "Rojo", "Violeta"};
const int PELIGRO_COLOR[5] = {0, 8, 13, 16, 20};

/*  slug, nombre, color, subterraneo, especial, protegido, item garantizado, descripcion */
Bloque mapa[NUM_BLOQUES] = {
    [B_INICIO] = {"inicio", "Inicio", C_CELESTE, 0, ESP_INICIO, -1, IT_NAVAJA,
        "El galpón donde pasaste la noche. La radio sigue repitiendo el mismo comunicado."},

    /* ---------------- NORTE ---------------- */
    [B_CN1] = {"calle_norte_1", "Calle Norte 1", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Autos abandonados con las puertas abiertas. Una alarma suena a lo lejos."},
    [B_CN2] = {"calle_norte_2", "Calle Norte 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Las vidrieras están rotas y hay manchas oscuras en el asfalto. Al fondo, unos reflectores barren el cielo."},
    [B_CHECK_N] = {"checkpoint_norte", "H.E.R.O. Checkpoint Norte", C_CELESTE, 0, ESP_CHECKPOINT, B_CN3, -1,
        "Vallas, reflectores y soldados con trajes protectores. Más allá, el camino a la Universidad."},
    [B_CN3] = {"calle_norte_3", "Calle Norte 3", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Una calle limpia, patrullada. Demasiado limpia."},
    [B_UNIVERSIDAD] = {"universidad", "Universidad", C_CELESTE, 0, ESP_NINGUNO, -1, -1,
        "H.E.R.O. convirtió el campus en un puesto de mando. Una fila de civiles sube por las escaleras del edificio principal."},
    [B_HELIPUERTO] = {"helipuerto", "Helipuerto", C_CELESTE, 0, ESP_HELIPUERTO, -1, -1,
        "La terraza del edificio principal. El viento golpea fuerte acá arriba."},
    [B_METRO_NORTE] = {"metro_norte", "Metro Norte", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Una estación a medio terminar. Desde abajo llega aire fresco, como si hubiera otra salida."},
    [B_TUNEL_A] = {"tunel_a", "Túnel de Mantenimiento A", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Cables colgando y charcos de aceite. Alguien dejó marcas de tiza en las paredes."},
    [B_MALL] = {"mall", "Mall", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "El shopping está a oscuras. Algo se mueve entre las góndolas, despacio."},
    [B_CALLEJONES_N1] = {"callejones_norte_1", "Callejones Norte 1", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Pasillos angostos entre edificios. Cualquier cosa puede salir de una puerta."},
    [B_CALLEJONES_N2] = {"callejones_norte_2", "Callejones Norte 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Contenedores volcados. Por un hueco en la pared se ve el checkpoint."},
    [B_ESTADIO] = {"estadio", "Estadio (evacuado)", C_ROJO, 0, ESP_NINGUNO, -1, IT_PASE,
        "Evacuaron el estadio a las apuradas. Las tribunas no están vacías. No hay otra salida."},

    /* ---------------- OESTE ---------------- */
    [B_NW_METRO] = {"northwest_metro", "Northwest Metro", C_VERDE, 1, ESP_NINGUNO, -1, -1,
        "Molinetes rotos y carteles que todavía anuncian el próximo tren."},
    [B_FABRICA] = {"fabrica", "Fábrica", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Las máquinas siguen encendidas. El ruido tapa cualquier otro sonido."},
    [B_DISTRITO] = {"distrito_comercial", "Distrito Comercial", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Oficinas saqueadas y papeles volando por la avenida."},
    [B_HOSPITAL] = {"hospital", "Hospital", C_VIOLETA, 0, ESP_NINGUNO, -1, IT_BOTIQUIN,
        "Camillas en la vereda y las puertas abiertas de par en par. Adentro, algo sigue moviéndose."},
    [B_CALLEJON] = {"callejon", "Callejón", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Un pasaje oscuro detrás del hospital que termina en las vías del tren."},
    [B_CO1] = {"calle_oeste_1", "Calle Oeste 1", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Una avenida ancha. Hay huellas de neumáticos pesados, todas en la misma dirección."},
    [B_CO2] = {"calle_oeste_2", "Calle Oeste 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Un colectivo cruzado bloquea media calle. Del otro lado se ven vallas."},
    [B_CHECK_O] = {"checkpoint_oeste", "H.E.R.O. Checkpoint Oeste", C_CELESTE, 0, ESP_CHECKPOINT, B_TREN, -1,
        "Vallas, soldados y una bocina que suena cada tanto, más allá."},
    [B_TREN] = {"tren", "Tren", C_CELESTE, 0, ESP_TREN, -1, -1,
        "Una multitud desesperada rodea el último tren."},
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
        "Muelles derrumbados y grúas caídas. Formas lentas deambulan entre los escombros."},

    /* ---------------- SUR ---------------- */
    [B_CS1] = {"calle_sur_1", "Calle Sur 1", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Persianas bajas y un semáforo que titila en amarillo."},
    [B_CS2] = {"calle_sur_2", "Calle Sur 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Una barricada civil abandonada. Quien la armó no tuvo suerte."},
    [B_CS3] = {"calle_sur_3", "Calle Sur 3", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "Se escucha algo, como una multitud que no habla. Huele a mar."},
    [B_CHECK_S] = {"checkpoint_sur", "H.E.R.O. Checkpoint Sur", C_CELESTE, 0, ESP_CHECKPOINT, B_PUERTO, -1,
        "Contenedores apilados como muralla. Detrás se escucha el motor de un barco."},
    [B_PUERTO] = {"puerto", "Puerto", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Un barco todavía está amarrado. La pasarela sigue baja."},
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
        "Bicicletas tiradas y un puesto de diarios dado vuelta. En una pared, alguien pintó una flecha "
        "hacia el este: «AEROPUERTO - VUELOS»."},
    [B_CE2] = {"calle_este_2", "Calle Este 2", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Patrulleros abandonados con las sirenas apagadas. Uno tiene la radio prendida: «...desvíen a los "
        "civiles al aeropuerto...» y después estática."},
    [B_CHECK_E] = {"checkpoint_este", "H.E.R.O. Checkpoint (cerrado)", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "Las vallas están soldadas y no hay nadie. Quien estuviera acá se fue hace rato."},
    [B_TURISTA1] = {"calle_turista_1", "Calle Turista 1", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Restaurantes con las mesas todavía servidas."},
    [B_TURISTA2] = {"calle_turista_2", "Calle Turista 2", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "Un micro de turistas volcado. Sus pasajeros siguen cerca."},
    [B_HOTEL] = {"hotel", "Hotel", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "El lobby es un desastre. Se escuchan golpes en los pisos de arriba. En la recepción hay un cartel "
        "escrito a mano: «Huéspedes: traslado al aeropuerto cada hora»."},
    [B_ESTACIONAMIENTO] = {"estacionamiento", "Estacionamiento", C_VERDE, 0, ESP_NINGUNO, -1, -1,
        "Un subsuelo de autos. Algunos todavía tienen las llaves puestas. Faltan muchos: las marcas de "
        "neumáticos salen todas hacia la autopista del aeropuerto."},
    [B_CE3] = {"calle_este_3", "Calle Este 3", C_ROJO, 0, ESP_NINGUNO, -1, -1,
        "La autopista hacia el aeropuerto, cubierta de autos chocados, todos apuntando al mismo lado. "
        "Un cartel luminoso parpadea: «EVACUACIÓN - TERMINAL A»."},
    [B_AEROPUERTO] = {"aeropuerto", "Aeropuerto", C_ROJO, 0, ESP_SIN_ESCAPE, -1, -1,
        "Los carteles de «EVACUACIÓN» siguen encendidos, pero no hay aviones. Solo una terminal a oscuras, "
        "llena de valijas, y un olor dulce, espeso."},
    [B_NE_METRO] = {"northeast_metro", "Northeast Metro", C_VERDE, 1, ESP_NINGUNO, -1, -1,
        "Un andén silencioso. Las luces de emergencia parpadean."},
    [B_TUNEL_B] = {"tunel_b", "Túnel de Mantenimiento B", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Un pasillo de servicio con puertas de metal a los costados."},
    [B_BLOQUEO] = {"bloqueo", "Bloqueo", C_NARANJA, 1, ESP_NINGUNO, -1, -1,
        "Un derrumbe tapa el túnel por completo. No se puede seguir."},
    [B_CALLEJONES_E1] = {"callejones_este_1", "Callejones Este 1", C_NARANJA, 0, ESP_NINGUNO, -1, -1,
        "Escaleras de incendio y ropa colgada. Una salida del túnel da acá."},
};

/* Como se ve cada bloque desde uno vecino. Se usa para describir las salidas. */
const char *VISTO[NUM_BLOQUES] = {
    [B_INICIO]          = "el galpón donde pasaste la noche",

    [B_CN1]             = "una calle con autos abandonados que va hacia el norte",
    [B_CN2]             = "una avenida más ancha; al fondo, reflectores",
    [B_CHECK_N]         = "las vallas y los reflectores de un puesto de H.E.R.O.",
    [B_CN3]             = "una calle limpia y patrullada, del otro lado de las vallas",
    [B_UNIVERSIDAD]     = "el campus de la Universidad, lleno de soldados",
    [B_HELIPUERTO]      = "las escaleras que suben a la terraza",
    [B_METRO_NORTE]     = "la boca de una estación de subte sin terminar",
    [B_TUNEL_A]         = "un túnel de mantenimiento",
    [B_MALL]            = "la entrada del shopping, a oscuras",
    [B_CALLEJONES_N1]   = "un pasillo angosto entre edificios",
    [B_CALLEJONES_N2]   = "más pasillos, que se meten entre los edificios",
    [B_ESTADIO]         = "los portones del estadio",

    [B_NW_METRO]        = "las escaleras del subte, hacia el noroeste",
    [B_FABRICA]         = "el portón de una fábrica",
    [B_DISTRITO]        = "las oficinas del distrito comercial",
    [B_HOSPITAL]        = "la entrada del hospital",
    [B_CALLEJON]        = "un callejón detrás del hospital, hacia las vías",
    [B_CO1]             = "una avenida ancha hacia el oeste",
    [B_CO2]             = "una calle cortada por un colectivo",
    [B_CHECK_O]         = "un puesto de H.E.R.O. junto a las vías",
    [B_TREN]            = "la estación de tren",
    [B_SW_METRO]        = "las escaleras del subte, hacia el suroeste",
    [B_TUNEL_D]         = "un túnel lleno de grafitis",
    [B_HIDEOUT]         = "un pasillo con luz de fogatas y voces",
    [B_TUNEL_C]         = "un túnel donde el agua empieza a subir",
    [B_INUNDACION]      = "un tramo inundado",
    [B_ALC1]            = "una boca de alcantarilla",
    [B_ALC2]            = "las alcantarillas, más adentro",
    [B_ALC3]            = "un canal que huele a mar",
    [B_PUERTO_DESTRUIDO]= "una salida a un muelle derrumbado",

    [B_CS1]             = "una calle hacia el sur",
    [B_CS2]             = "una barricada abandonada",
    [B_CS3]             = "la última calle antes del puerto",
    [B_CHECK_S]         = "una muralla de contenedores con soldados",
    [B_PUERTO]          = "el puerto, donde todavía hay un barco",
    [B_BARCO]           = "la pasarela del barco",
    [B_INVERNADERO]     = "un invernadero de vidrio",
    [B_ALMACEN]         = "un depósito lleno de cajas",
    [B_GUARDA_BARCOS]   = "un galpón de lanchas, junto al agua",

    [B_CE1]             = "una calle hacia el este",
    [B_CE2]             = "una calle con patrulleros abandonados",
    [B_CHECK_E]         = "un puesto de control, aparentemente vacío",
    [B_TURISTA1]        = "una calle de restaurantes",
    [B_TURISTA2]        = "una calle con un micro volcado",
    [B_HOTEL]           = "la entrada de un hotel",
    [B_ESTACIONAMIENTO] = "la rampa de un estacionamiento",
    [B_CE3]             = "la autopista hacia el aeropuerto; hay carteles de evacuación",
    [B_AEROPUERTO]      = "la terminal del aeropuerto, con carteles de «EVACUACIÓN» encendidos",
    [B_NE_METRO]        = "las escaleras del subte, hacia el noreste",
    [B_TUNEL_B]         = "un túnel de servicio",
    [B_BLOQUEO]         = "un túnel que parece tapado más adelante",
    [B_CALLEJONES_E1]   = "un callejón con escaleras de incendio",
};

/* Bioma de cada bloque: si no existe assets/lugares/<slug>.png, se usa
 * assets/biomas/<bioma>.png. Los bloques sin bioma necesitan su propia imagen. */
const char *BIOMA[NUM_BLOQUES] = {
    [B_CN1] = "calle", [B_CN2] = "calle", [B_CN3] = "calle",
    [B_CO1] = "calle", [B_CO2] = "calle",
    [B_CS1] = "calle", [B_CS2] = "calle", [B_CS3] = "calle",
    [B_CE1] = "calle", [B_CE2] = "calle", [B_CE3] = "calle",
    [B_TURISTA1] = "calle", [B_TURISTA2] = "calle", [B_DISTRITO] = "calle",
    [B_CALLEJONES_N1] = "callejon", [B_CALLEJONES_N2] = "callejon", [B_CALLEJONES_E1] = "callejon",
    [B_CALLEJON] = "callejon",

    [B_CHECK_N] = "checkpoint", [B_CHECK_O] = "checkpoint", [B_CHECK_S] = "checkpoint",
    [B_CHECK_E] = "checkpoint",

    [B_NW_METRO] = "subte", [B_NE_METRO] = "subte", [B_SW_METRO] = "subte", [B_METRO_NORTE] = "subte",

    [B_TUNEL_A] = "tunel", [B_TUNEL_B] = "tunel", [B_TUNEL_C] = "tunel", [B_TUNEL_D] = "tunel",
    [B_BLOQUEO] = "tunel", [B_HIDEOUT] = "tunel",

    [B_INUNDACION] = "alcantarilla", [B_ALC1] = "alcantarilla", [B_ALC2] = "alcantarilla",
    [B_ALC3] = "alcantarilla",

    [B_PUERTO] = "puerto", [B_PUERTO_DESTRUIDO] = "puerto", [B_GUARDA_BARCOS] = "puerto",
    [B_BARCO] = "puerto",

    [B_INICIO] = "galpon", [B_ALMACEN] = "galpon",
    [B_TREN] = "calle",
};

/* Atajos que esquivan un checkpoint. Para pasar hace falta FUERZA:
 * Bombero 20, Policia 15, Paramedico 5, Vago 0 (+15 con el Suero de H.E.R.O.) */
Paso pasos[NUM_PASOS] = {
    {B_METRO_NORTE, B_CN3, 20, "la puerta no cede",
        "Una puerta de servicio oxidada corta la escalera. Empujás con todo el cuerpo, pero no cede.",
        "Una puerta de servicio oxidada corta la escalera. Empujás con todo el cuerpo y, con un chirrido, cede.", 0, 0},
    {B_CALLEJON, B_TREN, 20, "la reja no cede",
        "Una reja con cadena corta el paso hacia las vías. Tirás hasta que te sangran las manos. No se mueve.",
        "Una reja con cadena corta el paso hacia las vías. Tirás con fuerza hasta que la cadena salta.", 0, 0},
    {B_GUARDA_BARCOS, B_PUERTO, 15, "el portón no cede",
        "Un portón de chapa trabado da a la rampa. Lo empujás, pero apenas se mueve.",
        "Un portón de chapa trabado da a la rampa. Lo empujás hasta abrir un hueco por el que pasar.", 0, 0},
    {B_PUERTO_DESTRUIDO, B_PUERTO, 15, "los escombros no se mueven",
        "Una grúa caída tapa el camino al puerto. Intentás mover los escombros, pero son demasiado pesados.",
        "Una grúa caída tapa el camino al puerto. Movés los escombros hasta abrir un paso.", 0, 0},
};

Paso *buscar_paso(int a, int b)
{
    int i;
    for (i = 0; i < NUM_PASOS; i++)
        if ((pasos[i].a == a && pasos[i].b == b) || (pasos[i].a == b && pasos[i].b == a)) return &pasos[i];
    return NULL;
}

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
    for (i = 0; i < NUM_PASOS; i++) {
        pasos[i].abierto = 0;
        pasos[i].intentado = 0;
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
