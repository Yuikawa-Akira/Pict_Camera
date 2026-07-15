
# Pict Camera

![Pict Camera](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/pictcamera_002.png)

AtomS3R CAM で撮影した写真を、その場でドット絵（ピクセルアート）風の画像に変換して保存する、シンプルなカメラです。  
ボタンを1回押すだけで、8種類のプリセットカラーパレットを使ったエモーショナルな写真が撮れます。

## Features

- **ワンクリック撮影** — 難解な設定なし。ボタン1つで撮影から変換・保存まで完結
- **カスタムパレット対応** — microSDにオリジナルのカラーパレットを用意すれば、自分だけの色調で撮影可能

## How it works
撮影した画像を以下のモードで変換します。  

### Dither 8
8階調の輝度データを基に、カラーパレットの8色に置き換えるシンプルなアルゴリズム
![Dither_8](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/dither8.png)

### Digital 8
レトロPCのようなデジタル8色で表現したビビットな画像に変換  
![Digital_8](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/digtal8.png)


## Hardware

### ・SIMPLE
はんだ付け不要、3ステップの簡単な組み立て  
![hardware_002](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/hardware_002.png)

- [M5Stack AtomS3R CAM](https://docs.m5stack.com/en/core/AtomS3R%20CAM)
- [Atomic TFCard Base](https://docs.m5stack.com/en/atom/Atomic%20TF-Card%20Reader)
- [Tail Bat](https://docs.m5stack.com/en/atom/tailbat)
- [Unit Key](https://docs.m5stack.com/en/unit/key)


### ・ADV
画面付き、追加の撮影モード  
![hardware_003](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/hardware_003.png)

- [M5Stack AtomS3R CAM](https://docs.m5stack.com/en/core/AtomS3R%20CAM)
- [Atomic TFCard Base](https://docs.m5stack.com/en/atom/Atomic%20TF-Card%20Reader)
- [Unit Glass2](https://docs.m5stack.com/en/unit/Glass2%20Unit) or [Unit OLED](https://docs.m5stack.com/en/unit/oled)
- [Unit Encoder](https://docs.m5stack.com/en/unit/encoder)

#### BURST MODE
![BURST MODE](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/burst_mode.gif)

#### TIMELAPSE MODE
![TIMELAPSE MODE](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/timelapse_mode.gif)

## Getting Started

### 書き込み済みファームウェアを使う場合

![m5burner](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/m5burner.png)  
[M5Burner](https://docs.m5stack.com/en/download) で **"Pict Camera"** を検索し、書き込むだけで利用できます。

### ソースからビルドする場合

1. Arduino IDE を用意し、AtomS3R CAM 向けのボード設定・必要なライブラリ（M5Unified / M5GFX / esp32-camera 等）をインストール
2. `src/` 内の `.ino` ファイルを開いてビルド・書き込み
3. 必要に応じて microSD カードにカラーパレットや設定ファイルを配置

## Custom Palette / Settings

microSD カードのルートディレクトリに以下のようなファイルを配置することで、コードを変更せずに見た目や挙動を調整できます。

| ファイル | 内容 |
|---|---|
| `ColorPalette0.txt` 〜 `ColorPalette7.txt` | 8種のカラーパレット |
| `ColorPalette8.txt` | Digital 8 モード用の専用8色パレット |
| `Dither8.txt` | ディザ階調数・ディザリングの有無 |
| `Digital8.txt` | Digital 8 モードの彩度・コントラストパラメータ |


## Web Simulator
![Pict Camera Editor](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/Pict_Camera_Editor.png)  
[Pict Camera Editor](https://yuikawa-akira.github.io/Pict_Camera/)  
ブラウザ上で変換パラメータやパレットを試せるシミュレーターも同梱しています。  
実機に書き込む前に、パラメータの当たりをつけたり、パレットを編集したりするのに利用できます。

## OBS Shader
OBS上でリアルタイムで動作するシェーダーフィルタとしても利用できます。  

### 導入手順

![obs_shader](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/obs_shader.png)　　

1. [obs-shaderfilter](https://github.com/exeldro/obs-shaderfilter)  をインストール
2. Webカメラなど加工したいソースを右クリック → フィルタ → エフェクトフィルタ → 「User-defined shader」を追加
3. 「Load shader text from file」を有効にして pict_camera.shader を選択（Use Effect File (.effect) はチェックしない）

## License

[MIT License](./LICENSE)

## Author

[Akira Yuikawa](https://github.com/Yuikawa-Akira)

## Dependencies
   - [esp32-camera](https://github.com/espressif/esp32-camera) (Apache License 2.0) — Espressif Systems
   - M5Unified / M5GFX (MIT License) — M5Stack
