# Bundled fonts

These TrueType fonts are embedded into the plugin binary via `juce_add_binary_data`
(see the root `CMakeLists.txt`) and loaded at runtime by `TapeFonts`
(`Source/TapeStopLookAndFeel.cpp`). They render the "Neon Cassette" editor: the
sans for all chrome, the mono for every numeric readout.

| File                        | Family / weight        | Used for                          |
|-----------------------------|------------------------|-----------------------------------|
| `SpaceGroteskRegular.ttf`   | Space Grotesk 400      | section labels (TAPE SPEED, …)    |
| `SpaceGroteskSemiBold.ttf`  | Space Grotesk 600      | title, WIND-DOWN pill             |
| `SpaceGroteskBold.ttf`      | Space Grotesk 700      | STOP / STOPPING button            |
| `JetBrainsMonoRegular.ttf`  | JetBrains Mono 400     | all numeric readouts, TYPE IV     |

The TTFs are Latin-subset static instances (≈32 KB each for Space Grotesk) to keep
the embedded payload small.

## Licensing

Both families are licensed under the **SIL Open Font License, Version 1.1**, which
permits bundling and redistribution inside an application. The full license texts
are included here and must travel with the fonts:

- `OFL-SpaceGrotesk.txt` — Space Grotesk © 2020 The Space Grotesk Project Authors
  (https://github.com/floriankarsten/space-grotesk)
- `OFL-JetBrainsMono.txt` — JetBrains Mono © 2020 The JetBrains Mono Project Authors
  (https://github.com/JetBrains/JetBrainsMono)
