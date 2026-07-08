
# Pict Camera

**Simple Pixel-Art Like Camera with AtomS3R CAM**

AtomS3R CAM で撮影した写真を、その場でドット絵（ピクセルアート）風の画像に変換して保存する、シンプルなカメラです。
ボタンを1回押すだけで、8種類のプリセットカラーパレットを使ったエモーショナルな写真が撮れます。

## Features

- **ワンクリック撮影** — 難解な設定なし。ボタン1つで撮影から変換・保存まで完結
- **8種類のプリセットカラーパレット** — 内蔵の8色パレット
- **カスタムパレット対応** — microSD (TFカード) にオリジナルのカラーパレットを用意すれば、自分だけの色調で撮影可能
- **Digital 8 モード** — 彩度・コントラストを強調した、より鮮やかな8色変換モード
- **SDカードから各種パラメータを調整可能** — パレット、ディザ階調数、彩度・コントラストのパラメータなどをコード変更なしに調整

## How it works

内蔵カメラで撮影したRGB画像を、以下のようなシンプルなアルゴリズムで変換しています。

1. 撮影した画像のRGB値を輝度データに変換し、8段階の階調に量子化
2. 選択中のカラーパレットの8色に、階調ごとの色を割り当て
3. Bayerディザリングにより階調の境界を滑らかに補間

より鮮やかな発色を求める場合は、彩度強調・コントラスト強調（Sigmoidal Contrast）・Bayerディザ・加重距離によるパレット最近傍マッピングを組み合わせた **Digital 8** モードも利用できます。

## Hardware

- [M5Stack AtomS3R CAM](https://docs.m5stack.com/en/core/AtomS3R%20CAM)
- microSD (TF) カード（カスタムパレット・設定ファイル読み込み用）
- はんだ付け不要、3ステップの簡単な組み立て

## Getting Started

### 書き込み済みファームウェアを使う場合

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
[Pict Camera Editor](https://yuikawa-akira.github.io/Pict_Camera/)
ブラウザ上で変換パラメータやパレットを試せるシミュレーターも同梱しています。
実機に書き込む前に、パラメータの当たりをつけたり、パレットを編集したりするのに利用できます。

## License

[MIT License](./LICENSE)

## Author

[Akira Yuikawa](https://github.com/Yuikawa-Akira)

## Dependencies
   - [esp32-camera](https://github.com/espressif/esp32-camera) (Apache License 2.0) — Espressif Systems
   - M5Unified / M5GFX (MIT License) — M5Stack
