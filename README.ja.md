# Pict Camera

![Pict Camera](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/pictcamera_002.png)

**Pict Camera** は、[AtomS3R CAM](https://docs.m5stack.com/en/core/AtomS3R%20CAM) で撮影した写真を、その場でドット絵（ピクセルアート）風の画像へ変換して microSD カードに保存するカメラです。  
基本構成ではボタンを一度押すだけで、元画像と複数の変換画像を保存できます。画面・エンコーダーを加えた ADV 構成では、プレビュー、連写、タイムラプスにも対応します。

## できること

- 1回の撮影で、元画像・8種類の Dither 8 パレット画像・Digital 8 画像を PNG 形式で保存
- microSD カード上のテキストファイルによる、カラーパレットと変換パラメータのカスタマイズ
- SIMPLE 構成ではボタンだけで手軽に撮影
- ADV 構成ではライブプレビュー、パレット選択、連写、タイムラプスを利用

## 目次

- [必要なもの](#必要なもの)
- [ハードウェア構成](#ハードウェア構成)
- [はじめ方](#はじめ方)
- [撮影・変換モード](#撮影変換モード)
- [カスタマイズ](#カスタマイズ)
- [関連ツール](#関連ツール)
- [トラブルシューティング](#トラブルシューティング)

## 必要なもの

すべての構成で次の3点が必要です。

- [M5Stack AtomS3R CAM](https://docs.m5stack.com/en/core/AtomS3R%20CAM)
- [Atomic TFCard Base](https://docs.m5stack.com/en/atom/Atomic%20TF-Card%20Reader)
- microSD カード（撮影画像と設定ファイルの保存用）

## ハードウェア構成

| 構成 | 向いている用途 | 追加部品 | 使用するスケッチ |
| --- | --- | --- | --- |
| SIMPLE | まず手軽に撮影したい | [Tail Bat](https://docs.m5stack.com/en/atom/tailbat)、[Unit Key](https://docs.m5stack.com/en/unit/key) | `src/pict_camera.ino` |
| ADV（Glass2） | 画面で確認しながら撮影したい | [Unit Glass2](https://docs.m5stack.com/en/unit/Glass2%20Unit)、[Unit Encoder](https://docs.m5stack.com/en/unit/encoder) | `src/pict_camera_glass2.ino` |
| ADV（OLED） | OLED 表示で操作したい | [Unit OLED](https://docs.m5stack.com/en/unit/oled)、[Unit Encoder](https://docs.m5stack.com/en/unit/encoder) | `src/pict_camera_oled.ino` |

### SIMPLE

はんだ付け不要の、ボタンで撮影する基本構成です。

![SIMPLE 構成](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/hardware_002.png)

### ADV

画面でプレビューを確認し、エンコーダーでパレットと撮影モードを操作する構成です。

![ADV 構成](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/hardware_003.png)

## はじめ方

### M5Burner で使う

![M5Burner](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/m5burner.png)

1. [M5Burner](https://docs.m5stack.com/en/download) をインストールします。
2. M5Burner で **`Pict Camera`** を検索します。
3. AtomS3R CAM を USB 接続し、ファームウェアを書き込みます。
4. microSD カードを Atomic TFCard Base に挿入してから電源を入れます。

### ソースからビルドする

1. このリポジトリをダウンロードまたはクローンし、Arduino IDE を用意します。
2. AtomS3R CAM 用のボード定義を導入し、Arduino IDE で対象ボードとシリアルポートを選択します。
3. 使用する構成に対応した `.ino` ファイルを開きます。

   | 構成 | 開くファイル |
   | --- | --- |
   | SIMPLE | `src/pict_camera.ino` |
   | ADV（Glass2） | `src/pict_camera_glass2.ino` |
   | ADV（OLED） | `src/pict_camera_oled.ino` |

4. 必要なライブラリを導入します。基本構成には `M5Unified`、`M5GFX`、`esp32-camera` が必要です。ADV 構成では、接続する表示器および Unit Encoder に対応するライブラリも導入してください。
5. microSD カードを挿入し、検証・書き込みを実行します。

設定ファイルを使う場合は、書き込み後に [`params/`](./params) 内のファイルを microSD カードのルートディレクトリへコピーします。設定ファイルがなくても、プログラムに組み込まれた既定値で動作します。

## 撮影・変換モード

### Dither 8

画像の明るさを段階化し、選択した8色のカラーパレットへ置き換えるモードです。`ColorPalette0.txt` 〜 `ColorPalette7.txt` により、8種類の色調を定義できます。

![Dither 8 の例](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/dither8.png)

### Digital 8

レトロ PC のような、固定8色で表現するモードです。専用パレットと彩度・コントラストに関するパラメータを調整できます。

![Digital 8 の例](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/digital8.png)

### ADV 構成の撮影モード

ADV 構成では、エンコーダーを**1秒以上長押し**するとモード選択画面を開けます。回してモードを選び、短押しで確定します。

| モード | 動作 |
| --- | --- |
| NORMAL | 1回の操作で通常撮影します。 |
| BURST | 操作で開始・停止し、連続して撮影します。 |
| TIMELAPSE | 操作で開始・停止し、10秒間隔で撮影します。 |

BURST と TIMELAPSE では、エンコーダーを回して使用するパレットを選択できます。

![BURST MODE](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/burst_mode.gif)

![TIMELAPSE MODE](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/timelapse_mode.gif)

## カスタマイズ

### microSD カードへの配置

設定ファイルは microSD カードの**ルートディレクトリ**に置きます。ファイルがない場合は既定値が使用されます。

| ファイル | 役割 |
| --- | --- |
| `ColorPalette0.txt` 〜 `ColorPalette7.txt` | Dither 8 用の8種類のパレット |
| `ColorPalette8.txt` | Digital 8 用の8色パレット（任意） |
| `Dither8.txt` | Dither 8 の階調数とディザリングの有効/無効 |
| `Digital8.txt` | Digital 8 の色調パラメータ |

### カラーパレットの書式

各パレットファイルには、`0xRRGGBB` 形式の色を1行に1色ずつ、**8色**記述します。

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

`ColorPalette0.txt` 〜 `ColorPalette7.txt` は Dither 8 に、`ColorPalette8.txt` は Digital 8 に使われます。空行は読み飛ばされます。8色に満たない場合は、読み込めた色だけが反映され、残りには既定値が使われます。

### Dither 8 の設定例

`Dither8.txt` では次のキーを指定できます。`dithering_levels` は 2〜16 の範囲に収められます。

```ini
dithering_levels=8
dithering_enabled=true
```

`dithering_enabled` には `true` / `false` または `1` / `0` を指定できます。

### Digital 8 の設定例

`Digital8.txt` では、次のキーを指定できます。

```ini
sat_factor_x128=256
sigmoid_strength=24.0
sigmoid_midpoint=0.50
bias_width=12
```

値を少しずつ変え、後述の Web Simulator で見え方を確認してから実機へ反映するのがおすすめです。

## 関連ツール

### Web Simulator

![Pict Camera Editor](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/Pict_Camera_Editor.png)

[Pict Camera Editor](https://yuikawa-akira.github.io/Pict_Camera/) では、ブラウザ上で変換パラメータとパレットを試せます。実機に書き込む前の色調確認やパレット作成に利用してください。

### OBS Shader

Pict Camera の表現を、OBS 上でリアルタイムに適用するシェーダーフィルターも用意しています。

![OBS Shader の設定例](https://github.com/Yuikawa-Akira/Pict_Camera/blob/main/images/obs_shader.png)

1. [obs-shaderfilter](https://github.com/exeldro/obs-shaderfilter) をインストールします。
2. 加工したいソースを右クリックし、**フィルタ** → **エフェクトフィルタ** → **User-defined shader** を追加します。
3. **Load shader text from file** を有効にし、[`OBS/pict_camera.shader`](./OBS/pict_camera.shader) を選択します。**Use Effect File (.effect)** は有効にしません。

## トラブルシューティング

| 状況 | 確認すること |
| --- | --- |
| 撮影・保存できない | microSD カードが Atomic TFCard Base に正しく挿入され、認識されているか確認してください。 |
| パレットや設定が反映されない | ファイル名、配置先（microSD のルート）、キー名、パレットの8色を確認してください。 |
| ADV 構成が動作しない | 接続した表示器に対応する `.ino` を選び、表示器と Unit Encoder 用のライブラリを導入してください。 |
| ビルドに失敗する | AtomS3R CAM 用のボード定義、選択中のボード、必要ライブラリの導入状況を確認してください。 |

## 依存関係

- [esp32-camera](https://github.com/espressif/esp32-camera) — Apache License 2.0 / Espressif Systems
- [M5Unified](https://github.com/m5stack/M5Unified) — MIT License / M5Stack
- [M5GFX](https://github.com/m5stack/M5GFX) — MIT License / M5Stack

ADV 構成では、このほか接続する Unit に対応したライブラリが必要です。

## ライセンス

[MIT License](./LICENSE)

## 作者

[Akira Yuikawa](https://github.com/Yuikawa-Akira)
