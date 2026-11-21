# JustAHead

VST3 de amp sim de guitarra (head/preamp) hecho con JUCE 8, pensado para ir **antes de `IRScene`**. No incluye cab IR ni wah: la salida es line level para que el loader de IRs haga el trabajo de cabina.

## Cadena

```
In → Pedal (Off / Overdrive / Distorsión) → Head → Out (line level)
```

### Heads (DSP "inspirado", no capturas ML)

- **British OD** — crunch británico tipo JCM800 / Zakk Wylde.
- **Modern 3Ch** — high-gain moderno tipo 5150.
- **UK Iommi** — británico oscuro y gordo, tipo Laney.
- **American Thick** — americano denso, tipo Mesa.

Son aproximaciones con waveshaping + oversampling 4x + tone stack, **no** modelos 1:1 ni capturas de hardware. Se pueden afinar de a poco contra references.

### Pedales internos

- **Overdrive** — clipping suave, ecualizaciónMedia.
- **Distorsión** — clipping más duro y apretado.

### Controles

- Head: Gain, Bass, Mid, Treble, Presence.
- Pedal: Drive, Tone, Level.
- Master de salida.

Doble clic en un knob: vuelve al valor por defecto (comportamiento estándar de JUCE).

## Estado del proyecto

Fase 1: motor + OD/Dist + un head calibrado (`British OD`) + UI + GitHub Actions.
Fases siguientes: otros heads, modulaciones post-head, delay y reverb.

## Cómo conseguir el `.vst3`

1. Pestaña **Actions** → workflow *Build VST3* (corre en cada push a `main` o con *Run workflow*).
2. Bajá el artefacto `JustAHead-Windows-VST3` (o macOS).
3. Descomprimí y copiá la carpeta `JustAHead.vst3` a `C:\Program Files\Common Files\VST3\`.
4. REAPER: *Preferences > Plug-ins > VST > Re-scan*.

## Licencia de JUCE

JUCE 8 se usa bajo los términos de su licencia (EULA / AGPLv3). Ver https://juce.com/legal/juce-8-licence/
