# Pict Camera

[日本語版はこちら](./README.ja.md)

![Pict Camera](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/pictcamera_002.png)

**Pict Camera** is a camera that turns photos captured with an [AtomS3R CAM](https://docs.m5stack.com/en/core/AtomS3R%20CAM) into pixel-art-style images and saves them to a microSD card.  
With the basic configuration, one button press saves the original image along with several converted images. The ADV configuration adds a display and encoder for live preview, burst shooting, and time-lapse shooting.

## Features

- Saves the original image, eight Dither 8 palette variants, and one Digital 8 variant as PNG files for every capture
- Customizes color palettes and conversion parameters through text files on the microSD card
- Takes photos easily with a single button in the SIMPLE configuration
- Provides live preview, palette selection, burst shooting, and time-lapse shooting in the ADV configuration

## Contents

- [Requirements](#requirements)
- [Hardware Configurations](#hardware-configurations)
- [Getting Started](#getting-started)
- [Shooting and Conversion Modes](#shooting-and-conversion-modes)
- [Customization](#customization)
- [Additional Tools](#additional-tools)
- [Troubleshooting](#troubleshooting)

## Requirements

Every configuration requires the following:

- [M5Stack AtomS3R CAM](https://docs.m5stack.com/en/core/AtomS3R%20CAM)
- [Atomic TFCard Base](https://docs.m5stack.com/en/atom/Atomic%20TF-Card%20Reader)
- A microSD card for captured images and settings files

## Hardware Configurations

| Configuration | Best for | Additional hardware | Sketch |
| --- | --- | --- | --- |
| SIMPLE | Quick, button-only shooting | [Tail Bat](https://docs.m5stack.com/en/atom/tailbat), [Unit Key](https://docs.m5stack.com/en/unit/key) | `src/pict_camera.ino` |
| ADV (Glass2) | Shooting while viewing a display | [Unit Glass2](https://docs.m5stack.com/en/unit/Glass2%20Unit), [Unit Encoder](https://docs.m5stack.com/en/unit/encoder) | `src/pict_camera_glass2.ino` |
| ADV (OLED) | Shooting with an OLED display | [Unit OLED](https://docs.m5stack.com/en/unit/oled), [Unit Encoder](https://docs.m5stack.com/en/unit/encoder) | `src/pict_camera_oled.ino` |

### SIMPLE

The basic button-operated configuration requires no soldering.

![SIMPLE configuration](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/hardware_002.png)

### ADV

This configuration lets you check a live preview on the display and use an encoder to select palettes and shooting modes.

![ADV configuration](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/hardware_003.png)

## Getting Started

### Use M5Burner

![M5Burner](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/m5burner.png)

1. Install [M5Burner](https://docs.m5stack.com/en/download).
2. Search for **`Pict Camera`** in M5Burner.
3. Connect the AtomS3R CAM over USB and flash the firmware.
4. Insert a microSD card into the Atomic TFCard Base, then power on the device.

### Build from Source

1. Download or clone this repository and install the Arduino IDE.
2. Install the board definition for the AtomS3R CAM, then select the board and serial port in the Arduino IDE.
3. Open the `.ino` file that matches your configuration.

   | Configuration | File to open |
   | --- | --- |
   | SIMPLE | `src/pict_camera.ino` |
   | ADV (Glass2) | `src/pict_camera_glass2.ino` |
   | ADV (OLED) | `src/pict_camera_oled.ino` |

4. Install the required libraries. The basic configuration requires `M5Unified`, `M5GFX`, and `esp32-camera`. ADV configurations also require libraries for the connected display and Unit Encoder.
5. Insert the microSD card, then verify and upload the sketch.

To use custom settings, copy the files in [`params/`](./params) to the root directory of the microSD card after uploading. The camera uses built-in defaults when settings files are absent.

## Shooting and Conversion Modes

### Dither 8

This mode quantizes image brightness and maps it to a selected eight-color palette. `ColorPalette0.txt` through `ColorPalette7.txt` define eight different color looks.

![Dither 8 example](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/dither8.png)

### Digital 8

This mode renders images in a fixed eight-color, retro-PC-inspired style. Its dedicated palette and color parameters can be customized.

![Digital 8 example](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/digital8.png)

### ADV Shooting Modes

In the ADV configuration, hold the encoder button for **at least one second** to open the mode selection screen. Turn the encoder to select a mode, then short-press it to confirm.

| Mode | Behavior |
| --- | --- |
| NORMAL | Takes a single, standard capture. |
| BURST | Starts and stops continuous shooting. |
| TIMELAPSE | Starts and stops shooting at 10-second intervals. |

In BURST and TIMELAPSE modes, turn the encoder to select the palette to use.

![BURST MODE](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/burst_mode.gif)

![TIMELAPSE MODE](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/timelapse_mode.gif)

## Customization

### Place Settings on the microSD Card

Place settings files in the **root directory** of the microSD card. Built-in defaults are used when a file is not present.

| File | Purpose |
| --- | --- |
| `ColorPalette0.txt` to `ColorPalette7.txt` | Eight palettes for Dither 8 |
| `ColorPalette8.txt` | Optional eight-color palette for Digital 8 |
| `Dither8.txt` | Dither 8 levels and dithering on/off setting |
| `Digital8.txt` | Digital 8 color parameters |

### Palette File Format

Each palette file contains **eight** colors, one per line, in `0xRRGGBB` format.

```text
0x000000
0x000B22
0x112B43
0x437290
0x437290
0xE0D8D1
0xE0D8D1
0xFFFFFF
```

`ColorPalette0.txt` through `ColorPalette7.txt` are used by Dither 8, while `ColorPalette8.txt` is used by Digital 8. Empty lines are skipped. If fewer than eight colors are provided, loaded colors are applied and the remaining colors keep their defaults.

### Dither 8 Settings

`Dither8.txt` accepts the following keys. `dithering_levels` is constrained to the range 2–16.

```ini
dithering_levels=8
dithering_enabled=true
```

For `dithering_enabled`, use `true` / `false` or `1` / `0`.

### Digital 8 Settings

`Digital8.txt` accepts the following keys:

```ini
sat_factor_x128=256
sigmoid_strength=24.0
sigmoid_midpoint=0.50
bias_width=12
```

Try small adjustments, then use the Web Simulator below to preview the result before applying it on the device.

## Additional Tools

### Web Simulator

![Pict Camera Editor](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/Pict_Camera_Editor.png)

[Pict Camera Editor](https://yuikawa-akira.github.io/Pict_Camera/) lets you try conversion parameters and palettes in your browser. Use it to check colors or build palettes before writing settings to the device.

### OBS Shader

An OBS shader filter is also included to apply the Pict Camera look in real time.

![OBS Shader setup example](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/obs_shader.png)

1. Install [obs-shaderfilter](https://github.com/exeldro/obs-shaderfilter).
2. Right-click the source to process, then choose **Filters** → **Effect Filters** → **User-defined shader**.
3. Enable **Load shader text from file** and select [`OBS/pict_camera.shader`](./OBS/pict_camera.shader). Do not enable **Use Effect File (.effect)**.

## Troubleshooting

| Issue | What to check |
| --- | --- |
| Cannot capture or save images | Confirm that the microSD card is correctly inserted in the Atomic TFCard Base and can be mounted. |
| Palette or settings are not applied | Check the filename, location (the microSD root), key names, and that palettes contain eight colors. |
| ADV configuration does not work | Confirm that the sketch matches the connected display and that display and Unit Encoder libraries are installed. |
| Build fails | Check the AtomS3R CAM board definition, selected board, and required libraries. |

## Dependencies

- [esp32-camera](https://github.com/espressif/esp32-camera) — Apache License 2.0 / Espressif Systems
- [M5Unified](https://github.com/m5stack/M5Unified) — MIT License / M5Stack
- [M5GFX](https://github.com/m5stack/M5GFX) — MIT License / M5Stack

ADV configurations also require libraries for their connected M5Stack units.

## License

[MIT License](./LICENSE)

## Author

[Akira Yuikawa](https://github.com/Yuikawa-Akira)
