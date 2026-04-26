# PRD — VolumeControl v2

**Autor:** Cristian Carreras
**Fecha:** 2026-04-25
**Estado:** Draft

---

## 1. Contexto

El firmware actual de VolumeControl funciona como un dispositivo USB HID Consumer que controla volumen del sistema mediante un encoder rotatorio y un botón. Después de uso real se identificaron varios bugs y oportunidades de mejora en UX que justifican una v2.

## 2. Problemas identificados (v1)

| # | Problema | Impacto |
|---|----------|---------|
| P1 | El encoder reporta 4 ticks por detent físico, pero el firmware dispara un comando por cada tick → el volumen sube/baja 4 niveles por click. | Alto — UX inconsistente |
| P2 | El botón no tiene debounce ni detección de flanco — mantenerlo presionado dispara MUTE repetidamente (~20 veces/seg). | Alto — bug funcional |
| P3 | `delay(50)` bloqueante en el loop principal — bloquea el procesamiento USB y agrega latencia en otros eventos. | Medio — latencia perceptible |
| P4 | `Serial.println` siempre activo — overhead de CPU/RAM en producción. | Bajo — limpieza |

## 3. Objetivos

- **O1:** 1 detent del encoder = 1 nivel de volumen, de forma determinista.
- **O2:** Cada presión del botón dispara exactamente una acción.
- **O3:** Loop no bloqueante; latencia botón→HID < 30ms.
- **O4:** Ampliar funcionalidad multimedia sin agregar más botones físicos.

## 4. Requerimientos funcionales

### 4.1 Encoder rotatorio
- **RF-1.1:** Rotación CW = `MEDIA_VOLUME_UP`, una vez por detent.
- **RF-1.2:** Rotación CCW = `MEDIA_VOLUME_DOWN`, una vez por detent.
- **RF-1.3 (opcional):** Aceleración — si se detectan ≥ 3 detents en < 200ms, incrementar el step a x2.

### 4.2 Botón del encoder
- **RF-2.1:** Click corto (press y release en < 400ms) = `MEDIA_VOL_MUTE` (toggle).
- **RF-2.2:** Long press (mantener > 600ms) = `MEDIA_PLAY_PAUSE`. Solo dispara una vez al cruzar el umbral.
- **RF-2.3:** Double click (dos clicks cortos en < 350ms) = `MEDIA_NEXT`.
- **RF-2.4:** El botón debe tener debounce mínimo de 20ms.

### 4.3 Inicialización
- **RF-3.1:** El firmware debe esperar enumeración USB antes de aceptar entrada (evitar comandos perdidos al arrancar).

## 5. Requerimientos no funcionales

- **RNF-1:** Loop principal sin `delay()` — usar `millis()` para timing.
- **RNF-2:** Footprint: < 50% de la flash disponible (14KB de 28KB), < 50% de RAM (1.2KB de 2.5KB).
- **RNF-3:** Logs serie envueltos en macro `#ifdef DEBUG` — desactivados por defecto en builds release.
- **RNF-4:** Constantes con `constexpr` en lugar de `#define` cuando aplique (type safety).
- **RNF-5:** Compatible con la board custom `booleMicro` actual sin cambios de hardware.

## 6. Fuera de alcance (v2)

- Feedback visual (LED de estado de mute).
- Display OLED con nivel de volumen.
- Perfiles configurables vía USB Serial.
- Soporte para múltiples salidas de audio (per-app volume).
- OTA firmware update.

## 7. Criterios de aceptación

- [ ] Girar el encoder un click sube/baja exactamente un nivel de volumen del sistema.
- [ ] Mantener el botón presionado durante 5 segundos no dispara MUTE más de una vez (en su lugar, dispara PLAY_PAUSE una sola vez al pasar 600ms).
- [ ] Double click en < 350ms cambia a la siguiente pista.
- [ ] No hay `delay()` en el `loop()`.
- [ ] La aceleración (si se implementa) no genera saltos en uso normal/lento.

## 8. Plan de implementación sugerido

| Fase | Scope | Estimación |
|------|-------|------------|
| F1 | Fix bugs P1, P2 (encoder detents + botón con flanco/debounce) | 1 sesión |
| F2 | Refactor a loop no-bloqueante (RNF-1) | 1 sesión |
| F3 | Long press + double click (RF-2.2, RF-2.3) | 1 sesión |
| F4 | Aceleración encoder + cleanup (RF-1.3, P4, RNF-3/4) | opcional |

## 9. Riesgos

- **R1:** El umbral de double click (350ms) puede sentirse lento o rápido según el usuario — puede requerir tuning.
- **R2:** La aceleración puede generar overshoot en ajustes finos — mitigación: solo activarla por encima de un threshold de velocidad claro.
- **R3:** El botón compartido entre 3 gestos (click/long/double) introduce latencia inherente: el firmware debe esperar el "double click window" antes de confirmar un click simple. Aceptable hasta ~350ms.
