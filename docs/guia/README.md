# Arquitectura

PrintStation separa la UI, la lógica de presentación y el motor de impresión en tres capas,
repartidas en dos hilos.

```
            Hilo de UI                                 Hilo worker
 ┌──────────────────────────────────────┐        ┌───────────────────────┐
 │  QML (vistas)                        │        │                       │
 │    HomePage / QueuePage / Delegate   │        │   PrintEngine         │
 │         │ bindings   ▲ NOTIFY        │        │   ("la impresora")    │
 │         ▼            │               │ queued │   QTimer = 1 página   │
 │  PrinterViewModel ───┼───────────────┼───────►│                       │
 │    (despachante)  ◄──┼───────────────┼────────│   un trabajo por vez  │
 │         │ dueño de                   │ queued │                       │
 │         ▼                            │        └───────────────────────┘
 │  JobQueueModel (la cola)             │
 └──────────────────────────────────────┘
```

| Capa | Dónde | Responsabilidad |
|---|---|---|
| Vistas | `qml/` | Bindings y llamadas al ViewModel. Sin lógica de negocio. |
| ViewModels | `src/viewmodels/` | `PrinterViewModel` expone estado y acciones a QML y despacha trabajos; `JobQueueModel` es la cola (`QAbstractListModel`). Viven en el hilo de UI. |
| Core | `src/core/` | `PrintEngine` simula la impresora en un hilo worker. No depende de QML. |

## Threading

- El engine se mueve al worker con `moveToThread`; no se subclasifica `QThread`.
- La comunicación entre hilos es sólo por señales/slots (conexiones queued). No hay estado
  mutable compartido ni mutex.
- El engine avanza con un `QTimer` en vez de `sleep`, así su event loop sigue atendiendo
  pausa y cancelación.
- La cola vive en el hilo de UI y es la única fuente de verdad; el engine reporta eventos
  identificados por `jobId`.
- Cierre ordenado: `quit()` + `wait()` en el destructor del ViewModel.
