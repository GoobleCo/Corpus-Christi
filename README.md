# Escape de la Ciudad

Juego de texto por turnos, estilo novela visual, hecho en **C** con [raylib](https://www.raylib.com/).
Estás en una ciudad llena de infectados y tenés que llegar al helicóptero de H.E.R.O. para escapar.

## Cómo jugar

- **En el navegador o el celular:** entrá a la página de GitHub Pages del repo (`https://<usuario>.github.io/<repo>/`).
  En el celu conviene ponerlo horizontal.
- **En Windows:** en esa misma página está el link *Descargar para Windows*. Descomprimí el zip y abrí `EscapeZombi.exe`
  (la carpeta `assets` tiene que quedar al lado del .exe).

**Controles:** cada opción se elige con su número en el teclado (1–8) o tocándola / clickeándola.
Si hay más de 8 opciones, la 8 es *Más opciones* y pasa de página. Si el texto no entra, se puede
arrastrar o usar la rueda del mouse.

## Cómo se ve la pantalla

```
+--------------------------------------------------+
|  IMAGEN del bloque (+ enemigo encima)       HUD  |
+-----------------------------+--------------------+
|  DESCRIPCIÓN / lo que pasó  |  OPCIONES 1..8     |
+-----------------------------+--------------------+
```

## Módulos

| Archivo | Módulo | Qué tiene |
|---|---|---|
| `src/juego.h` | — | Todos los tipos y funciones compartidas |
| `src/mapa.c` | BASE | Los 53 bloques del mapa y sus conexiones |
| `src/logica.c` | BASE | Creación del jugador, movimiento, loot, combate y las pantallas |
| `src/items.c` | ITEMS | Tabla de ítems, inventario, loot y uso de ítems |
| `src/enemigos.c` | ENEMIGOS | Enemigos, eventos y qué hace cada uno en su turno |
| `src/finales.c` | FINALES | Los textos de cada final |
| `src/debug.c` | DEBUG | Agregar ítems, cambiar stats, Always d20, teletransporte |
| `src/main.c` | Interfaz | Lo único que usa raylib: dibuja y lee teclado/mouse/toques |

La lógica no depende de raylib: cada estado del juego arma una *Pantalla* (imagen + texto + opciones)
y `main.c` solamente la dibuja. Para esconder el menú de debug, poné `DEBUG_HABILITADO` en `0` en `src/juego.h`.

## Poner tus propias imágenes

Las imágenes que vienen son **de prueba**. Reemplazá cualquier PNG por el tuyo con **el mismo nombre**:

| Carpeta | Tamaño recomendado | Notas |
|---|---|---|
| `assets/bloques/` | 1280 × 400 | Se recorta para cubrir la zona de la imagen |
| `assets/enemigos/` | ~600 × 700 | **Con fondo transparente**; se dibuja encima del bloque |
| `assets/finales/` | 1280 × 400 | Una por final |
| `assets/portada.png` | 1280 × 400 | Pantalla del nombre y la clase |

Enemigos: `infectado_comun.png`, `policia_infectado.png`, `stalker.png`, `vandalo.png`.
Finales: `helicoptero.png`, `tren.png`, `bote.png`, `cuarentena.png`, `infeccion_tardia.png`, `muerte.png`, `convertido.png`.

Si borrás una imagen, el juego igual anda y muestra qué archivo falta. Para volver a crear las de prueba que falten:
`python3 herramientas/generar_imagenes.py`.

<details>
<summary>Imágenes de los 53 bloques</summary>

| Archivo (en `assets/bloques/`) | Bloque | Color | Subterráneo |
|---|---|---|---|
| `inicio.png` | Inicio | Celeste |  |
| `calle_norte_1.png` | Calle Norte 1 | Verde |  |
| `calle_norte_2.png` | Calle Norte 2 | Naranja |  |
| `checkpoint_norte.png` | H.E.R.O. Checkpoint Norte | Celeste |  |
| `calle_norte_3.png` | Calle Norte 3 | Verde |  |
| `universidad.png` | Universidad | Celeste |  |
| `helipuerto.png` | Helipuerto | Celeste |  |
| `metro_norte.png` | Metro Norte | Naranja | Sí |
| `tunel_a.png` | Túnel de Mantenimiento A | Naranja | Sí |
| `mall.png` | Mall | Rojo |  |
| `callejones_norte_1.png` | Callejones Norte 1 | Naranja |  |
| `callejones_norte_2.png` | Callejones Norte 2 | Naranja |  |
| `estadio.png` | Estadio (evacuado) | Rojo |  |
| `northwest_metro.png` | Northwest Metro | Verde | Sí |
| `fabrica.png` | Fábrica | Naranja |  |
| `distrito_comercial.png` | Distrito Comercial | Naranja |  |
| `hospital.png` | Hospital | Violeta |  |
| `callejon.png` | Callejón | Naranja |  |
| `calle_oeste_1.png` | Calle Oeste 1 | Verde |  |
| `calle_oeste_2.png` | Calle Oeste 2 | Naranja |  |
| `checkpoint_oeste.png` | H.E.R.O. Checkpoint Oeste | Celeste |  |
| `tren.png` | Tren | Celeste |  |
| `southwest_metro.png` | Southwest Metro | Verde | Sí |
| `tunel_d.png` | Túnel de Mantenimiento D | Naranja | Sí |
| `vandal_hideout.png` | Vandal Hideout | Violeta | Sí |
| `tunel_c.png` | Túnel de Mantenimiento C | Naranja | Sí |
| `inundacion.png` | Inundación | Rojo | Sí |
| `alcantarillas_1.png` | Alcantarillas 1 | Naranja | Sí |
| `alcantarillas_2.png` | Alcantarillas 2 | Naranja | Sí |
| `alcantarillas_3.png` | Alcantarillas 3 | Naranja | Sí |
| `puerto_destruido.png` | Puerto Destruido | Rojo |  |
| `calle_sur_1.png` | Calle Sur 1 | Verde |  |
| `calle_sur_2.png` | Calle Sur 2 | Naranja |  |
| `calle_sur_3.png` | Calle Sur 3 | Rojo |  |
| `checkpoint_sur.png` | H.E.R.O. Checkpoint Sur | Celeste |  |
| `puerto.png` | Puerto | Verde |  |
| `barco.png` | Barco | Celeste |  |
| `invernadero.png` | Invernadero | Verde |  |
| `almacen.png` | Almacén | Naranja |  |
| `guarda_barcos.png` | Guarda Barcos | Naranja |  |
| `calle_este_1.png` | Calle Este 1 | Verde |  |
| `calle_este_2.png` | Calle Este 2 | Naranja |  |
| `checkpoint_este.png` | H.E.R.O. Checkpoint (cerrado) | Rojo |  |
| `calle_turista_1.png` | Calle Turista 1 | Naranja |  |
| `calle_turista_2.png` | Calle Turista 2 | Rojo |  |
| `hotel.png` | Hotel | Rojo |  |
| `estacionamiento.png` | Estacionamiento | Verde |  |
| `calle_este_3.png` | Calle Este 3 | Rojo |  |
| `aeropuerto.png` | Aeropuerto | Rojo |  |
| `northeast_metro.png` | Northeast Metro | Verde | Sí |
| `tunel_b.png` | Túnel de Mantenimiento B | Naranja | Sí |
| `bloqueo.png` | Bloqueo | Naranja | Sí |
| `callejones_este_1.png` | Callejones Este 1 | Naranja |  |

</details>

## Reglas que no estaban en el documento

- **Mapa:** se puede ir y volver por cualquier conexión. Los checkpoints de H.E.R.O. revisan la infección
  (más de 30 = final *Cuarentena*) y solo te dejan pasar al otro lado con el **Pase de Prioridad**.
  El Metro Norte, el Callejón y el Guarda Barcos / Puerto Destruido son caminos que esquivan los checkpoints.
- **Callejones sin salida:** Bloqueo, Estadio, Checkpoint Este (cerrado) y Aeropuerto. En el **Aeropuerto no se puede escapar** de un combate.
- **Ítems garantizados:** Navaja en el Inicio, Botiquín en el Hospital y Pase de Prioridad en el Estadio.
- **Peligro oculto:** sin el Mapa Satelital no ves el color/peligro de los bloques a los que vas.
- **Sin arma** peleás a puños (5 de daño + fuerza).
- **Violeta:** 8 a 10 infectados por evento. Los Vándalos vienen en grupos más chicos (1 a 4) porque tienen armas de fuego, y sueltan su arma al morir.
- **Ruido:** cada disparo tiene 25 % de chance de atraer otro enemigo.
- **Turnos:** moverse y cada combate cuentan un turno. El helicóptero espera hasta el turno 20; después se va y quedan el tren y el barco.
- Agregué dos finales de derrota: **Muerte** (vida en 0) y **Convertido** (infección en 100).

## Compilar en una computadora

Necesitás raylib instalado ([raylib.com](https://www.raylib.com/), en Windows viene con el compilador incluido).

```bash
make RAYLIB=/ruta/a/raylib/src     # compila EscapeZombi
make prueba                        # juega 2000 partidas al azar para buscar errores (no usa raylib)
```

## Subir el proyecto desde el celular

GitHub no deja subir carpetas desde el navegador del celu, así que:

1. Crear el archivo `.github/workflows/compilar.yml` con **Add file → Create new file** y pegar su contenido.
2. Subir `proyecto.zip` a la raíz del repo con **Add file → Upload files**.

GitHub Actions descomprime el zip, guarda los archivos sueltos en el repo y compila.

## Publicación automática

Cada vez que se sube algo a `main`, GitHub Actions (`.github/workflows/compilar.yml`) compila la versión web
y la de Windows y las publica en GitHub Pages. La primera vez hay que activar Pages en
**Settings → Pages → Source: GitHub Actions**.

La fuente es DejaVu Sans (licencia libre, ver `assets/LICENCIA_FUENTE.txt`).
