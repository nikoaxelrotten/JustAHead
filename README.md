# JustAHead

VST3 de amp sim de guitarra (head/preamp) hecho con JUCE 8, pensado para ir **antes de `IRScene`**. No incluye cab IR ni wah: la salida es line level para que el loader de IRs haga el trabajo de cabina.

## Cadena

```
In → Pedal (Off / Overdrive / Distorsión) → Head → Mod → Delay → Reverb → Out (line level)
```

### Heads (DSP "inspirado", no capturas ML)

Cada head tiene su propia cadena de etapas de válvula (2 a 4 etapas con recorte asimétrico), su tone stack con las frecuencias del estilo que modela, presence y etapa de potencia con sag. Todo corre sobremuestreado 4x.

- **British OD** — crunch británico tipo JCM800 / Zakk Wylde (3 etapas, EQ entre la 2a y la 3a, medios adelante).
- **Modern 3Ch** — high-gain moderno tipo 5150 (4 etapas, entrada ajustada, EQ después de la ganancia).
- **UK Iommi** — británico oscuro y gordo, tipo Laney (joroba de medios, más sag).
- **American Thick** — americano denso tipo Mesa (graves gordos, medios recortados, mucho sag).

Son aproximaciones, **no** modelos 1:1 ni capturas de hardware. Se afinan de a poco contra referencias.

### Pedales internos (van antes del head)

- **Overdrive** — estilo Tube Screamer: joroba de medios, recorte suave. Con Drive al mínimo casi no colorea; al máximo empuja fuerte al head.
- **Distorsión** — estilo RAT / DS-1: recorte duro; el ancho de banda baja a medida que sube Drive.

### Controles (todas las perillas 0 a 10)

- Head: Gain, Bass, Mid, Treble, Presence (5 = plano; +-14 dB en el EQ).
- Pedal: Drive, Tone, Level (dB).
- Master de salida (dB).

El nivel de salida está compensado: mover Gain no cambia el volumen más de ~1 dB. El plugin es mono por dentro (suma la entrada y copia a los dos canales).

Doble clic en un knob: vuelve al valor por defecto (comportamiento estándar de JUCE).

### Efectos post-head (fase 2)

Un slot de **Mod** (elegís uno) con tres perillas simples (Rate, Depth, Mix), y después **Delay** y **Reverb**, cada uno con sus perillas.

- **Chorus** — dos voces por canal, LFOs en cuadratura (estéreo ancho).
- **Flanger** — retardo corto barrido con realimentación fija.
- **Phaser** — 6 etapas allpass con barrido exponencial.
- **Tremolo** — modulación de amplitud (Depth = profundidad).
- **Slicer** — patrón de 8 pasos sincronizado al tempo del host. Rate elige la división: 1/4, 1/8, 1/8T, 1/16, 1/16T (de 0 a 10 en saltos de 2). Depth = cuánto se corta.
- **Ring Mod** — Rate = frecuencia de la portadora (25 Hz a 2 kHz); Depth pasa de AM a ring mod completo.
- **Delay** — estéreo ping-pong, Time 40 a 1200 ms, Feedback con filtros en la repetición, Mix.
- **Reverb** — FDN de 8 líneas: Size (decay de ~0,3 a ~6 s), Damp (brillo de la cola), Mix.

Con Mod en Off y Mix de Delay y Reverb en 0, la salida es solo el amp (mono copiado a los dos canales).

## Estado del proyecto

- **Fase 1:** motor del amp (4 heads, pedales OD/Dist, EQ y Gain que cambian de verdad, nivel calibrado).
- **Fase 2 (esta versión):** modulaciones, delay y reverb, con una fila de controles nueva en la interfaz.
- **Fase 3:** interfaz nueva estilo Neural DSP, escalable.

## Cómo conseguir el `.vst3`

1. Pestaña **Actions** → workflow *Build VST3* (corre en cada push a `main` o con *Run workflow*).
2. Bajá el artefacto `JustAHead-Windows-VST3` (o macOS).
3. Descomprimí y copiá la carpeta `JustAHead.vst3` a `C:\Program Files\Common Files\VST3\`.
4. REAPER: *Preferences > Plug-ins > VST > Re-scan*.

## Licencia de JUCE

JUCE 8 se usa bajo los términos de su licencia (EULA / AGPLv3). Ver https://juce.com/legal/juce-8-licence/
