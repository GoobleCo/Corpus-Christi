/*
 * ESCAPE DE LA CIUDAD - tipos y funciones compartidas por todos los modulos.
 *
 * La logica del juego (items.c, enemigos.c, mapa.c, finales.c, logica.c,
 * debug.c) NO usa raylib: solo arma una "Pantalla" (imagen + texto +
 * opciones). main.c es el unico archivo que dibuja con raylib.
 */
#ifndef JUEGO_H
#define JUEGO_H

/* Poner en 0 para esconder el menu de debug en la version final */
#define DEBUG_HABILITADO 1

#define MAX_INV            30
#define MAX_NOMBRE         24
#define MAX_ENEMIGOS       14
#define MAX_CONEXIONES     8
#define LIMITE_HELICOPTERO 20
#define LIMITE_CUARENTENA  30
#define DANIO_PUNOS        5
#define CHANCE_RUIDO       25    /* % de que un disparo atraiga otro enemigo */
#define MAX_OPCIONES       48
#define TEXTO_MAX          6000
#define MSG_MAX            4000

/* ============================ ITEMS ============================ */

typedef enum {
    T_MELEE, T_FUEGO, T_MUNICION, T_CURA,
    T_ARMADURA, T_MASCARA, T_MEJORA, T_MATERIAL
} TipoItem;

typedef enum { R_COMUN, R_RARO, R_EPICO, R_LEGENDARIO } Rareza;

typedef enum {
    IT_NAVAJA, IT_CUCHILLO, IT_HACHA, IT_MACHETE,
    IT_CINTA, IT_PIEZAS,
    IT_9MM, IT_ESCOPETA, IT_M16,
    IT_MUN_9MM, IT_MUN_ESCOPETA, IT_MUN_RIFLE,
    IT_VENDAJE, IT_PILDORAS, IT_BOTIQUIN,
    IT_ROPA, IT_UNI_PARAMEDICO, IT_HAZMAT, IT_UNI_BOMBERO,
    IT_TRAJE_SOBREV, IT_UNI_POLICIA, IT_UNI_ANTIDISTURBIO, IT_UNI_HERO,
    IT_VACUNA, IT_SUERO, IT_MAPA, IT_CURA_TEMPRANA, IT_PASE,
    IT_BARBIJO, IT_MASCARA_GAS,
    NUM_ITEMS
} ItemId;

typedef struct {
    const char *nombre;
    TipoItem tipo;
    Rareza rareza;
    int valor;        /* dano / curacion / armadura / mejora / usos que repara */
    int usos;         /* armas blancas: usos maximos (-1 = infinitos) */
    int anti_infec;   /* armaduras y mascaras: infeccion que bloquean */
    int municion;     /* armas de fuego: ItemId de su municion */
} ItemDef;

extern const ItemDef ITEMS[NUM_ITEMS];
extern const char *NOMBRE_RAREZA[4];

typedef struct {
    int id;
    int cantidad;     /* items que se apilan */
    int usos;         /* armas blancas */
    int equipado;
} Slot;

typedef struct {
    char nombre[MAX_NOMBRE];
    int clase;
    int vida, vida_max;
    int fuerza;
    int infeccion;
    Slot inv[MAX_INV];
    int n_inv;
    int bloque_actual;
    int turno;
} Jugador;

/* ============================ ENEMIGOS ============================ */

typedef enum { EN_COMUN, EN_POLICIA, EN_STALKER, EN_VANDALO } TipoEnemigo;

typedef struct {
    TipoEnemigo tipo;
    const char *nombre;
    const char *slug;           /* imagen: assets/enemigos/<slug>.png */
    int vida, vida_max, fuerza, armadura;
    int cd_salto, cd_evasion, evadiendo;   /* Stalker */
    int arma;                              /* Vandalo: ItemId de su arma */
} Enemigo;

typedef struct {
    Enemigo cola[MAX_ENEMIGOS];
    int n;
    int actual;
    int color;
    int subterraneo;
    int sin_escape;
} Combate;

/* ============================ MAPA ============================ */

/* (se llama ColorBloque porque raylib ya usa el nombre "Color") */
typedef enum { C_CELESTE, C_VERDE, C_NARANJA, C_ROJO, C_VIOLETA } ColorBloque;

typedef enum {
    ESP_NINGUNO, ESP_INICIO, ESP_CHECKPOINT, ESP_HELIPUERTO,
    ESP_TREN, ESP_BOTE, ESP_SIN_ESCAPE
} Especial;

typedef struct {
    const char *slug;           /* imagen: assets/bloques/<slug>.png */
    const char *nombre;
    ColorBloque color;
    int subterraneo;
    Especial especial;
    int protegido;              /* checkpoint: bloque al que solo se pasa con Pase */
    int item_garantizado;       /* -1 = ninguno */
    const char *descripcion;
    int conexiones[MAX_CONEXIONES];
    int n_con;
    int looteado;
} Bloque;

enum {
    B_INICIO,
    /* norte */
    B_CN1, B_CN2, B_CHECK_N, B_CN3, B_UNIVERSIDAD, B_HELIPUERTO,
    B_METRO_NORTE, B_TUNEL_A, B_MALL,
    B_CALLEJONES_N1, B_CALLEJONES_N2, B_ESTADIO,
    /* oeste */
    B_NW_METRO, B_FABRICA, B_DISTRITO, B_HOSPITAL, B_CALLEJON,
    B_CO1, B_CO2, B_CHECK_O, B_TREN,
    B_SW_METRO, B_TUNEL_D, B_HIDEOUT, B_TUNEL_C,
    B_INUNDACION, B_ALC1, B_ALC2, B_ALC3, B_PUERTO_DESTRUIDO,
    /* sur */
    B_CS1, B_CS2, B_CS3, B_CHECK_S, B_PUERTO, B_BARCO,
    B_INVERNADERO, B_ALMACEN, B_GUARDA_BARCOS,
    /* este */
    B_CE1, B_CE2, B_CHECK_E, B_TURISTA1, B_TURISTA2, B_HOTEL,
    B_ESTACIONAMIENTO, B_CE3, B_AEROPUERTO,
    B_NE_METRO, B_TUNEL_B, B_BLOQUEO, B_CALLEJONES_E1,
    NUM_BLOQUES
};

extern Bloque mapa[NUM_BLOQUES];
extern const char *NOMBRE_COLOR[5];
extern const int PELIGRO_COLOR[5];

/* ============================ FINALES ============================ */

typedef enum {
    F_INFECCION_TARDIA, F_CUARENTENA, F_HELICOPTERO, F_TREN, F_BOTE,
    F_MUERTE, F_CONVERTIDO, NUM_FINALES
} Final;

/* ============================ PANTALLA ============================ */

typedef struct {
    char texto[96];
    int valor;          /* que significa depende de la pantalla */
    int deshabilitada;
} Opcion;

typedef struct {
    char imagen[64];          /* ej: "bloques/inicio" -> assets/bloques/inicio.png */
    char imagen_enemigo[64];  /* vacio = no hay enemigo */
    char titulo[96];
    char texto[TEXTO_MAX];
    Opcion op[MAX_OPCIONES];
    int n_op;
    int pide_nombre;          /* la UI deja escribir el nombre */
    int mostrar_hud;
    int version;              /* cambia cada vez que se arma una pantalla nueva */
} Pantalla;

/* ============================ JUEGO ============================ */

typedef enum {
    E_NOMBRE, E_CLASE, E_BLOQUE, E_MOVER, E_INVENTARIO, E_USAR, E_REPARAR,
    E_ESTADO, E_COMBATE, E_FINAL,
    E_DEBUG, E_DEBUG_ITEMS, E_DEBUG_CANT, E_DEBUG_STATS, E_DEBUG_D20, E_DEBUG_TP
} Estado;

typedef struct {
    Jugador j;
    Combate c;
    int en_combate;
    Estado estado;
    Estado volver;            /* a donde vuelven inventario / usar / debug */
    int forzado;              /* escapo de un combate: tiene que moverse */
    int slot_material;
    int debug_item;
    Final final;
    int quiere_salir;
    char msg[MSG_MAX];        /* lo que acaba de pasar */
    Pantalla pant;
} Juego;

/* ---------- utilidades (logica.c) ---------- */
extern int d20_fijo;
int  azar(int min, int max);
int  tirar_d20(void);
int  minimo(int a, int b);
int  maximo(int a, int b);
void log_msg(Juego *g, const char *fmt, ...);
void agregar_opcion(Pantalla *p, int valor, int deshabilitada, const char *fmt, ...);
void agregar_texto(Pantalla *p, const char *fmt, ...);

/* ---------- logica.c (lo que usa la interfaz) ---------- */
void juego_nuevo(Juego *g);
void juego_elegir(Juego *g, int indice);
void juego_construir_pantalla(Juego *g);
void revisar_muerte(Juego *g);

/* ---------- items.c ---------- */
int  categoria_equipo(int id);
int  buscar_item(const Jugador *j, int id);
int  contar_item(const Jugador *j, int id);
int  equipado_en(const Jugador *j, int categoria);
void equipar(Jugador *j, int slot);
void quitar_slot(Jugador *j, int slot, int cant);
int  agregar_item(Jugador *j, int id, int cant);
int  armadura_total(const Jugador *j);
int  proteccion_infeccion(const Jugador *j);
void describir_slot(const Jugador *j, int s, char *buf, int tam);
const char *nombre_arma_equipada(const Jugador *j);
int  item_aleatorio(int color);
int  cantidad_loot(int id);
void recibir_loot(Juego *g, int id, int cant);
int  es_usable(int id);
int  usar_item(Juego *g, int slot);          /* 1 = gasto el turno */
int  reparar_arma(Juego *g, int slot_arma);  /* usa g->slot_material */

/* ---------- enemigos.c ---------- */
Enemigo crear_enemigo(TipoEnemigo t);
Enemigo enemigo_aleatorio(int color, int subterraneo);
int  cantidad_enemigos(int color, int subterraneo);
void aplicar_danio(Juego *g, int d);
void aplicar_infeccion(Juego *g, int ixi);
int  turno_enemigo(Juego *g, Enemigo *e);    /* 1 = el enemigo se fue */

/* ---------- mapa.c ---------- */
void iniciar_mapa(void);

/* ---------- finales.c ---------- */
void terminar(Juego *g, Final f);
const char *final_titulo(Final f);
const char *final_texto(Final f);
const char *final_slug(Final f);

/* ---------- debug.c ---------- */
void debug_pantalla(Juego *g);
void debug_elegir(Juego *g, int valor);

#endif
