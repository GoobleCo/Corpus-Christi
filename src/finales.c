/*
 * MODULO FINALES
 * Cada final usa la imagen assets/finales/<slug>.png
 */
#include "juego.h"

static const char *TITULOS[NUM_FINALES] = {
    [F_INFECCION_TARDIA] = "INFECCIÓN TARDÍA",
    [F_CUARENTENA]       = "CUARENTENA",
    [F_HELICOPTERO]      = "HELICÓPTERO",
    [F_TREN]             = "EL TREN",
    [F_BOTE]             = "EL BOTE",
    [F_MUERTE]           = "MUERTE",
    [F_CONVERTIDO]       = "CONVERTIDO",
};

static const char *SLUGS[NUM_FINALES] = {
    [F_INFECCION_TARDIA] = "infeccion_tardia",
    [F_CUARENTENA]       = "cuarentena",
    [F_HELICOPTERO]      = "helicoptero",
    [F_TREN]             = "tren",
    [F_BOTE]             = "bote",
    [F_MUERTE]           = "muerte",
    [F_CONVERTIDO]       = "convertido",
};

static const char *TEXTOS[NUM_FINALES] = {
    [F_INFECCION_TARDIA] =
        "Lograste escapar de la ciudad, pero ya era demasiado tarde. Los eventos de esa "
        "noche te seguirán por el resto de tu corta vida.",
    [F_CUARENTENA] =
        "Los soldados, vestidos en sus fríos trajes protectores, te llevan a punta de "
        "pistola a un vehículo blindado con otros civiles. Escapaste de la ciudad, pero "
        "no sos libre. Tu futuro es incierto.",
    [F_HELICOPTERO] =
        "Te subís al helicóptero de HERO, soldados y civiles esperando ser evacuados. Te "
        "llevan a una base lejos de la ciudad, y te ofrecen trabajar con ellos. Por ahora, "
        "parece que hay una chance de ganar esta guerra.",
    [F_TREN] =
        "Te subís al tren de HERO, civiles se cuelgan de los costados y los techos. "
        "Soldados gritan algo, pero no llegás a escuchar qué. Pronto escapás de la ciudad, "
        "pero el tren era muy lento para escapar de la horda. Encontrás el mismo destino "
        "que todos los demás \"afortunados\" que escaparon con vos.",
    [F_BOTE] =
        "Te escondés en el barco de HERO, y pronto salís de la ciudad. Los soldados no "
        "tardan en encontrarte. Al principio no están felices, pero al explicarles cómo "
        "llegaste - y un breve chequeo médico - dejan que te unas a ellos.",
    [F_MUERTE] =
        "Caíste en las calles de la ciudad. Uno más entre los miles que no llegaron a ver "
        "el amanecer.",
    [F_CONVERTIDO] =
        "La fiebre te consume. Tus pensamientos se apagan uno por uno, hasta que solo "
        "queda el hambre.",
};

const char *final_titulo(Final f) { return TITULOS[f]; }
const char *final_texto(Final f)  { return TEXTOS[f]; }
const char *final_slug(Final f)   { return SLUGS[f]; }

void terminar(Juego *g, Final f)
{
    /* INFECCIÓN TARDÍA reemplaza a los finales de escape si hay algo de infección */
    if ((f == F_HELICOPTERO || f == F_TREN || f == F_BOTE) && g->j.infeccion > 0)
        f = F_INFECCION_TARDIA;
    g->final = f;
    g->en_combate = 0;
    g->estado = E_FINAL;
}
