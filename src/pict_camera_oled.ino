#include <esp_camera.h>
#include <SPI.h>
#include <SD.h>
#include <M5UnitOLED.h>
#include <M5UnitGLASS2.h>
#include <M5Unified.h>
#include "Unit_Encoder.h"

#define POWER_GPIO_NUM 18
camera_fb_t* fb;

Unit_Encoder encoder;  // Unit Encoderのインスタンス

M5UnitOLED display;
//M5UnitGLASS2 display;

M5Canvas canvas_565_p;
M5Canvas canvas_565_v;
M5Canvas canvas_888_v;
M5Canvas canvas_txt;

const int NUM_PALETTES = 8;
M5Canvas dstSprites[NUM_PALETTES];

// 撮影モードの定義
enum ShootingMode {
  NORMAL,
  BURST,
  TIMELAPSE
};

ShootingMode currentMode = NORMAL;                 // 現在の撮影モード
bool isRecording = false;                          // 撮影中かどうか
bool isModeChanging = false;                       // モード切替画面にいるかどうか
unsigned long lastCaptureTime = 0;                 // 前回の撮影時刻 (ms)
const unsigned long TIMELAPSE_INTERVAL = 10000;    // タイムラプス間隔 (ms)
unsigned long textDisplayStartTime = 0;            // テキストが表示され始めた時刻
const unsigned long TEXT_DISPLAY_DURATION = 1000;  // テキストの表示時間 (1000ms = 1秒)
signed short int last_encoder_value = 0;
bool last_btn_status = true;                                       // ボタンの前回状態 (非押下: true, 押下: false)
unsigned long pressStartTime = 0;                                  // ボタンが押され始めた時刻
const unsigned long HOLD_DURATION = 1000;                          // 長押し判定時間 (ms)
int accumulatedChange = 0;                                         // エンコーダ値の累積変化量
const uint16_t PICTURE_WIDTH_PIX = 240, PICTURE_HEIGHT_PIX = 176;  // 4:3(15:11)
const uint16_t VIDEO_WIDTH_PIX = 128, VIDEO_HEIGHT_PIX = 72;       // 16:9
char filename[64];
char foldername[64];
char currentPaletteSetting[24];
int fileCounter = 0;
int folderCounter = 0;
int currentPaletteIndex = 0;           // 現在選択中のパレットインデックス (0〜7:通常パレット, 8:digital8)
const int maxPaletteIndex = 7;         // 通常カラーパレットの総数 (ColorPalettes[8][8] のインデックス上限)
const int DIGITAL8_PALETTE_INDEX = 8;  // currentPaletteIndex がこの値のときdigital8処理を使う
const int maxPaletteSelectIndex = 8;   // パレット選択の上限 (0〜7:通常パレット, 8:digital8)
int ditheringLevels = 8;               // ディザ階調数 2以上 (SDカードの /Dither8.txt から上書き可能)
bool enableDithering = true;           // ディザリング処理の有無 (SDカードの /Dither8.txt から上書き可能。デフォルトtrue)
uint8_t contrastLUT[256];              // 0〜255の変換結果を保持するテーブル
uint16_t d8_satFactorX128 = 256;       // 彩度倍率 x128 (128=1.0倍, 256=2.0倍)
float d8_sigmoidStrength = 24.0f;      // sigmoidal-contrastの強度
float d8_sigmoidMidpoint = 0.50f;      // sigmoidal-contrastの中心点 (0.0〜1.0)
int16_t d8_biasWidth = 12;             // Bayerディザの中心化バイアス幅 (±)

// カラーパレット
uint32_t ColorPalettes[8][8] = {
  { // パレット0 SLSO8 Created by Luis Miguel Maldonado
    0x0D2B45, 0x203C56, 0x544E68, 0x8D697A, 0xD08159, 0xFFAA5E, 0xFFD4A3, 0xFFECD6 },
  { // パレット1 2BIT DEMIBOY Created by Space Sandwich
    0x252525, 0x252525, 0x4B564D, 0x4B564D, 0x9AA57C, 0x9AA57C, 0xE0E9C4, 0xE0E9C4 },
  { // パレット2 TWILIGHT 5 Created by Star
    0x292831, 0x292831, 0x333F58, 0x333F58, 0x4A7A96, 0xEE8695, 0xEE8695, 0xFBBBAD },
  { // パレット3 BLUE SKY 5 Created by Yuikawa-Akira
    0x0D76D9, 0x3CADEC, 0x3CADEC, 0x71CDFC, 0x71CDFC, 0xBCE2FD, 0xBCE2FD, 0xFFFFFF },
  { // パレット4 MUMBLEBEE PALETTE Created by sudo_whoami
    0x334455, 0x334455, 0x334455, 0x334455, 0xFFBB33, 0xFFBB33, 0xFFBB33, 0xFFBB33 },
  { // パレット5 DREAM HAZE 8 Created by Klafooty
    0x3C42C4, 0x6E51C8, 0xA065CD, 0xCE79D2, 0xD68FB8, 0xDDA2A3, 0xEAC4AE, 0xF4DFBE },
  { // パレット6 DEEP MAZE Created by Ryosuke
    0x001D2A, 0x085562, 0x009A98, 0x00BE91, 0x38D88E, 0x9AF089, 0xF2FF66, 0xF2FF66 },
  { // パレット7 NIGHT RAIN Created by Maber
    0x000000, 0x012036, 0x3A7BAA, 0x7D8FAE, 0xA1B4C1, 0xF0B9B9, 0xFFD159, 0xFFFFFF },
};

uint32_t ColorPalettes8[8] = {
  0x000000, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF
};
const size_t FixedPaletteSize = 8;

// 4x4 Bayerマトリクス (値域 0〜15)
static const uint8_t BAYER4x4[4][4] = {
  { 0, 8, 2, 10 },
  { 12, 4, 14, 6 },
  { 3, 11, 1, 9 },
  { 15, 7, 13, 5 }
};

camera_config_t camera_config = {
  .pin_pwdn = -1,
  .pin_reset = -1,
  .pin_xclk = 21,
  .pin_sscb_sda = 12,
  .pin_sscb_scl = 9,
  .pin_d7 = 13,
  .pin_d6 = 11,
  .pin_d5 = 17,
  .pin_d4 = 4,
  .pin_d3 = 48,
  .pin_d2 = 46,
  .pin_d1 = 42,
  .pin_d0 = 3,

  .pin_vsync = 10,
  .pin_href = 14,
  .pin_pclk = 40,

  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,

  .pixel_format = PIXFORMAT_RGB565,
  //.frame_size = FRAMESIZE_96X96,    // 96x96
  //.frame_size = FRAMESIZE_128X128,  // 128x128
  //.frame_size = FRAMESIZE_QQVGA,    // 160x120
  //.frame_size = FRAMESIZE_QCIF,     // 176x144
  .frame_size = FRAMESIZE_HQVGA,  // 240x176
  //.frame_size = FRAMESIZE_240X240,  // 240x240
  //.frame_size = FRAMESIZE_QVGA,     // 320x240

  .jpeg_quality = 0,
  .fb_count = 2,
  .fb_location = CAMERA_FB_IN_PSRAM,
  .grab_mode = CAMERA_GRAB_LATEST,
  .sccb_i2c_port = 0,
};

void loadfolderCounter_internal() {
  if (SD.exists("/Config.txt")) {
    File file = SD.open("/Config.txt", FILE_READ);
    if (file) {
      String line = file.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        int loaded_count = line.toInt();
        folderCounter = loaded_count % 10000;  // 読み込んだ値を0-9999の範囲に制限して格納
        Serial.printf("Loaded folderCounter: %d (Clamped to: %d)\n", loaded_count, folderCounter);
      } else {
        folderCounter = 0;  // ファイルが空の場合は0
        Serial.println("Config.txt is empty. Setting folderCounter to 0.");
      }
      file.close();
    } else {
      Serial.println("ERROR: Failed to open Config.txt for reading. Setting folderCounter to 0.");
      folderCounter = 0;
    }
  } else {
    // Config.txtが存在しない場合、作成して0を書き込む
    File file = SD.open("/Config.txt", FILE_WRITE);
    if (file) {
      file.println(0);
      file.close();
      Serial.println("Created new Config.txt with folderCounter 0");
    } else {
      Serial.println("ERROR: Failed to create Config.txt.");
      errorLed();
    }
    folderCounter = 0;
  }
}

// SDカードが既にマウントされていることを前提として、folderCounterをConfig.txtに書き込む
void writefolderCounter_internal(int count) {
  // 'w' (FILE_WRITE)で開くと、ファイルが存在する場合は内容がクリアされる
  File file = SD.open("/Config.txt", FILE_WRITE);
  if (file) {
    // 【修正箇所: 0-9999でループするように制限してから書き込み】
    int save_count = count % 10000;
    file.println(save_count);
    file.close();
    Serial.printf("Saved folderCounter: %d to Config.txt\n", save_count);
  } else {
    Serial.println("ERROR: Failed to open Config.txt for writing (SD mounted).");
    errorLed();
  }
}

bool loadPaletteFromSD(int paletteIndex) {
  if (paletteIndex < 0 || paletteIndex > maxPaletteIndex) {
    return false;
  }
  String filename = "/ColorPalette" + String(paletteIndex) + ".txt";  // ファイル名を生成 (例: /ColorPalette0.txt)
  if (!SD.exists(filename)) {
    return false;  // ファイルが存在しない場合はデフォルトを使うのでfalseを返す
  }
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    return false;  // ファイルオープン失敗
  }

  // ファイルから8つのカラーコードを読み込む
  int colorCount = 0;
  while (file.available() && colorCount < 8) {
    String line = file.readStringUntil('\n');  // 1行読み込む
    line.trim();                               // 前後の空白や改行文字を削除

    if (line.length() > 0) {
      // strtoul(const char *str, char **endptr, int base)
      // base=0 で 0x (16進), 0 (8進), それ以外 (10進) を自動判別
      uint32_t colorValue = strtoul(line.c_str(), NULL, 0);

      // エラーチェック (strtoulはエラー時に0を返すことがあるが、0x000000も有効な色なので完全ではない)
      // ここでは単純に読み込んだ値を格納する
      ColorPalettes[paletteIndex][colorCount] = colorValue;
      colorCount++;
    }
  }

  file.close();

  // 8色読み込めたか確認
  if (colorCount == 8) {
    return true;  // 成功
  } else {
    // もし8色以下の場合は読み込めた分だけ反映して残りはデフォルトを使用する
    return false;  // 読み込み失敗（色が足りない）
  }
}

bool loadColorPalettes8FromSD() {
  const char* filename = "/ColorPalette8.txt";
  if (!SD.exists(filename)) {
    return false;  // ファイルが存在しない場合はデフォルトを使うのでfalseを返す
  }
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    return false;  // ファイルオープン失敗
  }

  // ファイルから8つのカラーコードを読み込む
  int colorCount = 0;
  while (file.available() && colorCount < 8) {
    String line = file.readStringUntil('\n');  // 1行読み込む
    line.trim();                               // 前後の空白や改行文字を削除

    if (line.length() > 0) {
      uint32_t colorValue = strtoul(line.c_str(), NULL, 0);
      ColorPalettes8[colorCount] = colorValue;
      colorCount++;
    }
  }

  file.close();

  // 8色読み込めたか確認
  if (colorCount == 8) {
    return true;  // 成功
  } else {
    // もし8色以下の場合は読み込めた分だけ反映して残りはデフォルトを使用する
    return false;  // 読み込み失敗（色が足りない）
  }
}

bool loadDigital8ParamsFromSD() {
  const char* path = "/Digital8.txt";
  if (!SD.exists(path)) {
    return false;  // ファイルが存在しない場合はデフォルトを使うのでfalseを返す
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    return false;  // ファイルオープン失敗
  }

  int appliedCount = 0;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();

    if (line.length() == 0 || line.startsWith("#")) {
      continue;  // 空行・コメント行はスキップ
    }

    int eqIndex = line.indexOf('=');
    if (eqIndex <= 0) {
      continue;  // "=" が無い、または先頭にある不正な行はスキップ
    }

    String key = line.substring(0, eqIndex);
    String value = line.substring(eqIndex + 1);
    key.trim();
    value.trim();

    if (key == "sat_factor_x128") {
      d8_satFactorX128 = (uint16_t)value.toInt();
      appliedCount++;
    } else if (key == "sigmoid_strength") {
      d8_sigmoidStrength = value.toFloat();
      appliedCount++;
    } else if (key == "sigmoid_midpoint") {
      d8_sigmoidMidpoint = value.toFloat();
      appliedCount++;
    } else if (key == "bias_width") {
      d8_biasWidth = (int16_t)value.toInt();
      appliedCount++;
    }
    // 未知のキーは無視 (将来の拡張やコメント代わりの記述を許容)
  }

  file.close();

  return (appliedCount > 0);  // 1つでも反映できればtrue
}

bool loadDither8ParamsFromSD() {
  const char* path = "/Dither8.txt";
  if (!SD.exists(path)) {
    return false;  // ファイルが存在しない場合はデフォルトを使うのでfalseを返す
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    return false;  // ファイルオープン失敗
  }

  int appliedCount = 0;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();

    if (line.length() == 0 || line.startsWith("#")) {
      continue;  // 空行・コメント行はスキップ
    }

    int eqIndex = line.indexOf('=');
    if (eqIndex <= 0) {
      continue;  // "=" が無い、または先頭にある不正な行はスキップ
    }

    String key = line.substring(0, eqIndex);
    String value = line.substring(eqIndex + 1);
    key.trim();
    value.trim();
    value.toLowerCase();

    if (key == "dithering_levels") {
      int loaded = value.toInt();
      if (loaded < 2) loaded = 2;    // 0除算防止の下限
      if (loaded > 16) loaded = 16;  // Bayer4x4の階調上限に合わせてクランプ
      ditheringLevels = loaded;
      appliedCount++;
    } else if (key == "dithering_enabled") {
      enableDithering = (value == "true" || value == "1");
      appliedCount++;
    }
    // 未知のキーは無視 (将来の拡張やコメント代わりの記述を許容)
  }

  file.close();

  return (appliedCount > 0);  // 1つでも反映できればtrue
}

void makeDir() {
  sprintf(foldername, "/%04d", folderCounter);
  SD.mkdir(foldername);
}

bool CameraBegin() {
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    return false;
  }

  // カメラ追加設定
  sensor_t* s = esp_camera_sensor_get();
  s->set_hmirror(s, 0);         // 左右反転 0無効 1有効 ひっくり返して使う場合は1
  s->set_vflip(s, 0);           // 上下反転 0無効 1有効 ひっくり返して使う場合は1
  s->set_saturation(s, 0);      // 彩度 (-2〜2)
  s->set_ae_level(s, 0);        // 自動露出 (-2〜2)
  s->set_contrast(s, 0);        // コントラスト (-2〜2) 0を推奨
  s->set_special_effect(s, 0);  // ノーマル/ネガ/モノクロ (0〜6)(ただし3〜6は未対応)
  s->set_wb_mode(s, 0);         // オート/晴天/曇天/電球/蛍光灯 (0〜4) オートを推奨
  //s->set_colorbar(s, 1);        // カラーバー 0無効 1有効
  return true;
}

bool CameraGet() {
  fb = esp_camera_fb_get();
  if (!fb) {
    return false;
  }
  return true;
}

bool CameraFree() {
  if (fb) {
    esp_camera_fb_return(fb);
    return true;
  }
  return false;
}

uint16_t swap16(uint16_t value) {
  return (value << 8) | (value >> 8);
}

void applyDitherAndPalette_dither8(M5Canvas& srcSprite, M5Canvas& dstSprite, int levelsPerChannel, int paletteIndex, bool useDither) {

  // =======================================================
  // 1. 定数定義と初期化
  // =======================================================

  static const float BAYER_DENOMINATOR = 16.0f;

  const int width = srcSprite.width();
  const int height = srcSprite.height();
  const int total_pixels = width * height;

  // パレットインデックスの範囲チェックとポインタ設定
  if (paletteIndex < 0 || paletteIndex > maxPaletteIndex) {
    paletteIndex = 0;
  }
  const uint32_t* targetPalette = ColorPalettes[paletteIndex];

  // M5Canvasのメモリバッファへ直接アクセス
  uint16_t* src_buffer = (uint16_t*)srcSprite.getBuffer();  // RGB565
  uint8_t* dst_buffer = (uint8_t*)dstSprite.getBuffer();    // RGB888

  if (!src_buffer || !dst_buffer) {
    Serial.println("Error: Canvas buffer not accessible.");
    errorLed();
    return;
  }

  // =======================================================
  // 2. Ditherなしの場合 (スキップパス)
  // =======================================================
  if (!useDither) {
    for (int i = 0; i < total_pixels; ++i) {
      uint16_t rgb565Color = swap16(src_buffer[i]);

      // RGB565 -> RGB888 変換
      uint8_t r = (rgb565Color >> 11) << 3;
      r |= r >> 5;
      uint8_t g = ((rgb565Color >> 5) & 0x3F) << 2;
      g |= g >> 6;
      uint8_t b = (rgb565Color & 0x1F) << 3;
      b |= b >> 5;

      // 輝度の計算 (BT.709係数に基づく)
      // Y = (54 * R + 183 * G + 19 * B) / 256
      uint32_t luminance_int = (54U * r + 183U * g + 19U * b) >> 8;

      // 輝度を8階調 (0-7)に量子化 (256/32 = 8)
      uint8_t grayLevel = luminance_int >> 5;
      if (grayLevel > 7) grayLevel = 7;

      // パレットルックアップ
      uint32_t paletted_color = targetPalette[grayLevel];

      // RGB888 (3バイト) として書き込み (R, G, Bの順を仮定)
      dst_buffer[i * 3 + 0] = (paletted_color >> 16) & 0xFF;  // R
      dst_buffer[i * 3 + 1] = (paletted_color >> 8) & 0xFF;   // G
      dst_buffer[i * 3 + 2] = paletted_color & 0xFF;          // B
    }
    return;  // 処理終了
  }

  // =======================================================
  // 3. LUTディザ処理
  // =======================================================

  const uint32_t intervals = levelsPerChannel - 1;

  // マイコンの処理を極限まで軽くするため、事前に計算結果をテーブル(LUT)化します。
  // スタックメモリを約768バイト消費しますが、速度は飛躍的に向上します。
  uint8_t base_level[256];
  uint16_t error_scaled_16[256];
  uint8_t quantized_values[256];  // levelsPerChannelの最大値は256を想定

  // 出力値のLUT生成 (浮動小数点時の std::roundf(level_low) 相当)
  for (int i = 0; i <= intervals; ++i) {
    // 整数演算による四捨五入: (値 + 割る数/2) / 割る数
    quantized_values[i] = (uint8_t)((i * 255 + intervals / 2) / intervals);
  }

  // 入力値(0〜255)ごとのベース階調と、比較用誤差のLUT生成
  for (int i = 0; i < 256; ++i) {
    uint32_t val_scaled = i * intervals;

    // 整数除算で std::floorf 相当の切り捨て
    uint32_t l_idx = val_scaled / 255;
    if (l_idx >= intervals) {
      l_idx = intervals - 1;
    }
    base_level[i] = l_idx;

    // 誤差 (val_src - level_low) に intervals と 16 を掛けたスケールアップ値
    error_scaled_16[i] = (uint16_t)((val_scaled - l_idx * 255) * 16);
  }

  for (int y = 0; y < height; ++y) {
    const int y_idx = y & 3;

    // 行ごとのバッファ開始ポインタを計算
    uint16_t* current_src_ptr = src_buffer + (ptrdiff_t)y * width;
    uint8_t* current_dst_ptr = dst_buffer + (ptrdiff_t)y * width * 3;

    for (int x = 0; x < width; ++x) {
      const int x_idx = x & 3;

      // bayerValue(0〜15) * 255 を整数比較用の閾値とする
      // (浮動小数点時の bayerValue * step 相当)
      const uint16_t bayer_thresh = BAYER4x4[y_idx][x_idx] * 255;

      // 【読み込みとRGB成分抽出】
      uint16_t rgb565Color = swap16(*current_src_ptr);

      uint8_t r_src = (rgb565Color >> 11) << 3;
      r_src |= r_src >> 5;
      uint8_t g_src = ((rgb565Color >> 5) & 0x3F) << 2;
      g_src |= g_src >> 6;
      uint8_t b_src = (rgb565Color & 0x1F) << 3;
      b_src |= b_src >> 5;

      uint8_t channels_src[3] = { r_src, g_src, b_src };
      uint8_t channels_dithered[3];

      // 【ディザリング適用 (LUTによる超高速化)】
      for (int ch = 0; ch < 3; ++ch) {
        uint8_t val_src = channels_src[ch];

        // LUTからベースとなる階調インデックスを取得
        uint32_t l_idx = base_level[val_src];

        // スケールアップされた誤差と閾値を整数比較
        if (error_scaled_16[val_src] >= bayer_thresh) {
          l_idx++;
        }

        // LUTから量子化後の出力値を取得
        channels_dithered[ch] = quantized_values[l_idx];
      }

      // 【パレット変換】ディザリング後の色を使って輝度を計算
      uint8_t r_dst = channels_dithered[0];
      uint8_t g_dst = channels_dithered[1];
      uint8_t b_dst = channels_dithered[2];

      uint32_t luminance_int = (54U * r_dst + 183U * g_dst + 19U * b_dst) >> 8;

      uint8_t grayLevel = luminance_int >> 5;
      if (grayLevel > 7) grayLevel = 7;

      // パレットルックアップ
      uint32_t paletted_color = targetPalette[grayLevel];

      // 【書き込み】RGB888 (3バイト) をバッファに直接書き込む
      *current_dst_ptr++ = (paletted_color >> 16) & 0xFF;  // R
      *current_dst_ptr++ = (paletted_color >> 8) & 0xFF;   // G
      *current_dst_ptr++ = paletted_color & 0xFF;          // B

      // srcポインタを次のピクセルへ進める
      current_src_ptr++;
    }
  }
}

// 1行ずつ輝度を抽出し、一気に複数パレットへ展開する超高速バッチ処理関数
void generatePaletteImages_LineByLine(M5Canvas& srcSprite, M5Canvas* dstSprites, const uint32_t** palettes, int numPalettes, int levelsPerChannel, bool useDither) {

  const int width = srcSprite.width();
  const int height = srcSprite.height();

  // 1行分の階調インデックス(0〜7)を保存するバッファ
  uint8_t* gray_buffer = (uint8_t*)malloc(width);
  if (!gray_buffer) {
    Serial.println("Memory allocation failed");
    return;
  }

  // ==========================================
  // 1. LUT初期化 (ディザリングありの場合のみ)
  // ==========================================
  const uint32_t intervals = levelsPerChannel - 1;
  uint8_t base_level[256];
  uint16_t error_scaled_16[256];
  uint8_t quantized_values[256];

  if (useDither && intervals > 0) {
    for (int i = 0; i <= intervals; ++i) {
      quantized_values[i] = (uint8_t)((i * 255 + intervals / 2) / intervals);
    }
    for (int i = 0; i < 256; ++i) {
      uint32_t val_scaled = i * intervals;
      uint32_t l_idx = val_scaled / 255;
      if (l_idx >= intervals) l_idx = intervals - 1;
      base_level[i] = l_idx;
      error_scaled_16[i] = (uint16_t)((val_scaled - l_idx * 255) * 16);
    }
  }

  uint16_t* src_buffer_base = (uint16_t*)srcSprite.getBuffer();

  // ==========================================
  // 2. メインループ (行単位での処理)
  // ==========================================
  for (int y = 0; y < height; ++y) {
    const int y_idx = y & 3;
    uint16_t* src_line_ptr = src_buffer_base + y * width;

    // ------------------------------------------------
    // ステージA: 1行分の輝度(階調)を計算
    // ------------------------------------------------
    for (int x = 0; x < width; ++x) {
      uint16_t rgb565 = swap16(src_line_ptr[x]);

      // RGB565 -> 888展開
      uint8_t r = (rgb565 >> 11) << 3;
      r |= r >> 5;
      uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
      g |= g >> 6;
      uint8_t b = (rgb565 & 0x1F) << 3;
      b |= b >> 5;

      // ディザリング適用
      if (useDither && intervals > 0) {
        const int x_idx = x & 3;
        const uint16_t bayer_thresh = BAYER4x4[y_idx][x_idx] * 255;
        uint8_t channels[3] = { r, g, b };

        for (int ch = 0; ch < 3; ++ch) {
          uint8_t val = channels[ch];
          uint32_t l_idx = base_level[val];
          if (error_scaled_16[val] >= bayer_thresh) {
            l_idx++;
          }
          channels[ch] = quantized_values[l_idx];
        }
        r = channels[0];
        g = channels[1];
        b = channels[2];
      }

      // 輝度を計算して8階調(0-7)インデックスに直結
      uint32_t luminance = (54U * r + 183U * g + 19U * b) >> 8;
      uint8_t grayLevel = luminance >> 5;
      if (grayLevel > 7) grayLevel = 7;

      // 結果を「パレットインデックス」として保存
      gray_buffer[x] = grayLevel;
    }

    // ------------------------------------------------
    // ステージB: パレットルックアップと書き込み (8枚分)
    // ------------------------------------------------
    for (int p = 0; p < numPalettes; ++p) {
      // 該当パレットの該当行のバッファ位置を計算
      uint8_t* dst_line_ptr = (uint8_t*)dstSprites[p].getBuffer() + y * width * 3;
      const uint32_t* currentPalette = palettes[p];

      for (int x = 0; x < width; ++x) {
        // ステージAで計算済みの階調(0-7)を読み出して色に変換するだけ
        uint32_t color = currentPalette[gray_buffer[x]];

        *dst_line_ptr++ = (color >> 16) & 0xFF;  // R
        *dst_line_ptr++ = (color >> 8) & 0xFF;   // G
        *dst_line_ptr++ = color & 0xFF;          // B
      }
    }
  }

  // メモリ解放
  free(gray_buffer);
}

long calculate_weighted_distance_sq(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
  // long型で差分を計算し、オーバーフローを防ぐ
  long dr = (long)r1 - (long)r2;
  long dg = (long)g1 - (long)g2;
  long db = (long)b1 - (long)b2;

  // 加重二乗距離を計算
  // 重み R: x2, G: x4, B: x3
  long distance_sq =
    2 * (dr * dr) + 4 * (dg * dg) + 3 * (db * db);

  return distance_sq;
}

void quantize_color_to_closest(uint8_t r, uint8_t g, uint8_t b, uint8_t* out_r, uint8_t* out_g, uint8_t* out_b) {
  long min_distance_sq = -1;
  uint32_t closest_color = 0;

  // 1. パレット全体を走査し、最小距離の色を探索
  for (size_t i = 0; i < FixedPaletteSize; ++i) {
    uint32_t palette_color = ColorPalettes8[i];

    // パレット色からRGB成分を分解 (0xRRGGBB形式)
    uint8_t r_pal = (palette_color >> 16) & 0xFF;
    uint8_t g_pal = (palette_color >> 8) & 0xFF;
    uint8_t b_pal = palette_color & 0xFF;

    // 2. 加重二乗距離を計算
    long distance_sq = calculate_weighted_distance_sq(r, g, b, r_pal, g_pal, b_pal);

    // 3. 最小距離の更新
    if (min_distance_sq == -1 || distance_sq < min_distance_sq) {
      min_distance_sq = distance_sq;
      closest_color = palette_color;
    }
  }

  // 4. 最も近い色を分解して出力に格納
  *out_r = (closest_color >> 16) & 0xFF;
  *out_g = (closest_color >> 8) & 0xFF;
  *out_b = closest_color & 0xFF;
}

void applyContrastLUT(uint8_t& r, uint8_t& g, uint8_t& b) {
  r = contrastLUT[r];
  g = contrastLUT[g];
  b = contrastLUT[b];
}

void initSigmoidContrastLUT(float strength = 8.0f, float midpoint = 0.5f) {
  for (int i = 0; i < 256; i++) {
    float x = i / 255.0f;

    // コントラスト低下 = sigmoid曲線の逆関数
    // f^-1(x) = midpoint - ln(1/y - 1) / strength
    // ただし端点正規化のため y を [sig_0, sig_1] 空間に写してから逆変換する
    float sig_0 = 1.0f / (1.0f + expf(-strength * (0.0f - midpoint)));
    float sig_1 = 1.0f / (1.0f + expf(-strength * (1.0f - midpoint)));

    // x を [sig_0, sig_1] 空間に写す
    float y = x * (sig_1 - sig_0) + sig_0;
    y = constrain(y, 0.0001f, 0.9999f);  // log保護

    // 逆sigmoid
    float result = midpoint - logf(1.0f / y - 1.0f) / strength;

    contrastLUT[i] = (uint8_t)constrain(result * 255.0f + 0.5f, 0, 255);
  }
}

void applyModulate_integer(uint8_t& r, uint8_t& g, uint8_t& b, uint16_t sat_factor_x128 = 256) {

  uint8_t cmax = std::max({ r, g, b });
  uint8_t cmin = std::min({ r, g, b });
  uint8_t delta = cmax - cmin;

  if (delta == 0 || cmax == 0) return;  // 無彩色 or 黒 → 変化なし

  uint16_t new_chroma = (uint16_t)std::min(
    (uint32_t)delta * sat_factor_x128 / 128,
    (uint32_t)cmax);
  uint8_t m = cmax - (uint8_t)new_chroma;

  if (cmax == r) {
    if (g >= b) {
      r = cmax;
      g = m + (uint8_t)((uint32_t)new_chroma * (g - cmin) / delta);
      b = m;
    } else {
      r = cmax;
      g = m;
      b = m + (uint8_t)((uint32_t)new_chroma * (b - cmin) / delta);
    }
  } else if (cmax == g) {
    if (r >= b) {
      r = m + (uint8_t)((uint32_t)new_chroma * (r - cmin) / delta);
      g = cmax;
      b = m;
    } else {
      r = m;
      g = cmax;
      b = m + (uint8_t)((uint32_t)new_chroma * (b - cmin) / delta);
    }
  } else {
    if (g >= r) {
      r = m;
      g = m + (uint8_t)((uint32_t)new_chroma * (g - cmin) / delta);
      b = cmax;
    } else {
      r = m + (uint8_t)((uint32_t)new_chroma * (r - cmin) / delta);
      g = m;
      b = cmax;
    }
  }
}

void applyDitherAndPalette_digital8(M5Canvas& srcSprite, M5Canvas& dstSprite) {

  const int width = srcSprite.width();
  const int height = srcSprite.height();

  uint16_t* src_buffer = (uint16_t*)srcSprite.getBuffer();
  uint8_t* dst_buffer = (uint8_t*)dstSprite.getBuffer();

  if (!src_buffer || !dst_buffer) {
    Serial.println("Error: Canvas buffer not accessible.");
    errorLed();
    return;
  }

  for (int y = 0; y < height; ++y) {
    const int y_idx = y & 3;
    uint16_t* src_row = src_buffer + (ptrdiff_t)y * width;
    uint8_t* dst_row = dst_buffer + (ptrdiff_t)y * width * 3;

    for (int x = 0; x < width; ++x) {
      const int x_idx = x & 3;

      // --- ステージ0: RGB565 → RGB888 ---
      uint16_t rgb565 = swap16(src_row[x]);
      uint8_t r = (rgb565 >> 11) << 3;
      r |= r >> 5;
      uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
      g |= g >> 6;
      uint8_t b = (rgb565 & 0x1F) << 3;
      b |= b >> 5;

      // --- ステージ1: 彩度アップ ---
      applyModulate_integer(r, g, b, d8_satFactorX128);  // 128 → 1.0x, 256 → 2.0x

      // --- ステージ2: 逆シグモイド関数 ---
      r = contrastLUT[r];
      g = contrastLUT[g];
      b = contrastLUT[b];

      // --- ステージ3: ディザリング処理 ---
      uint8_t bv = BAYER4x4[y_idx][x_idx];
      int16_t bayer_bias = (int16_t)(((int32_t)bv * (d8_biasWidth * 2)) >> 4) - d8_biasWidth;

      // バイアス加算
      int16_t r_biased = (int16_t)r + bayer_bias;
      int16_t g_biased = (int16_t)g + bayer_bias;
      int16_t b_biased = (int16_t)b + bayer_bias;

      auto quantize8 = [](int16_t val) -> uint8_t {
        // 0〜255にクランプ
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        // 8段階(0〜7)に量子化: val*7/255 を四捨五入
        uint8_t step = (uint8_t)((val * 7 + 127) / 255);
        // 255スケールに復元: step*255/7
        return (uint8_t)(step * 255 / 7);
      };

      uint8_t r_temp = quantize8(r_biased);
      uint8_t g_temp = quantize8(g_biased);
      uint8_t b_temp = quantize8(b_biased);

      // --- ステージ4: 最近傍パレットマッピング ---
      uint8_t r_out, g_out, b_out;
      quantize_color_to_closest(r_temp, g_temp, b_temp, &r_out, &g_out, &b_out);

      dst_row[x * 3 + 0] = r_out;
      dst_row[x * 3 + 1] = g_out;
      dst_row[x * 3 + 2] = b_out;
    }
  }
}


bool savePNGToSD(M5Canvas& canvasRef, const char* filename) {
  // 1. PNGデータの生成準備
  //Serial.printf("Creating PNG data in memory for canvas (W:%d, H:%d)...\n", canvasRef.width(), canvasRef.height());
  size_t datalen = 0;
  // 2. createPngを呼び出し、PNGデータをメモリに作成
  void* pngData = canvasRef.createPng(&datalen, 0, 0, canvasRef.width(), canvasRef.height());

  if (pngData == nullptr || datalen == 0) {
    Serial.printf("ERROR: Failed to create PNG data in memory (datalen: %u).\n", datalen);
    errorLed();
    return false;
  }
  //Serial.printf("PNG data created successfully (Size: %u bytes). Opening file: %s\n", datalen, filename);

  bool result = false;
  // 3. SDカードにファイルをオープン
  File file = SD.open(filename, "w");

  if (file) {
    // 4. メモリ上のPNGデータをファイルに書き込み
    size_t writtenBytes = file.write((const uint8_t*)pngData, datalen);
    file.close();

    if (writtenBytes == datalen) {
      //Serial.printf("Successfully saved PNG file: %s (Wrote %u bytes)\n", filename, writtenBytes);
      result = true;
    } else {
      Serial.printf("ERROR: Failed to write full PNG data (Expected: %u, Wrote: %u)\n", datalen, writtenBytes);
      errorLed();
    }
  } else {
    Serial.printf("ERROR: Could not open file for writing: %s\n", filename);
    errorLed();
  }
  // 5. createPngで確保されたメモリを解放
  free(pngData);
  return result;
}

String getModeName(ShootingMode mode) {
  switch (mode) {
    case NORMAL: return "NORMAL";
    case BURST: return "BURST";
    case TIMELAPSE: return "TIMELAPSE";
    default: return "UNKNOWN";
  }
}

// 処理開始時に一度だけ実行される関数
bool initializeProcess() {
  Serial.println(">>> Process STARTED! <<<");
  if (!SD.begin(15, SPI, 10000000)) {
    Serial.println("ERROR: SD Card mount failed.");
    errorLed();
    return false;
  }
  lastCaptureTime = millis();
  return true;
}

// NORMAL撮影
void executeSingleProcess() {
  if (!CameraGet()) {
    Serial.println("ERROR: Camera capture failed (Single).");
    errorLed();
    return;
  }
  canvas_565_p.pushImage(0, 0, PICTURE_WIDTH_PIX, PICTURE_HEIGHT_PIX, (uint16_t*)fb->buf);
  CameraFree();

  // パレットのポインタ配列を準備
  const uint32_t* paletteArray[8];
  for (int i = 0; i < 8; i++) {
    paletteArray[i] = ColorPalettes[i];
  }

  // ==========================================
  // 1. 8パレット一括生成 (ディザリングの有無は enableDithering に従う)
  // ==========================================
  generatePaletteImages_LineByLine(canvas_565_p, dstSprites, paletteArray, 8, ditheringLevels, enableDithering);
  for (int i = 0; i <= maxPaletteIndex; i++) {
    sprintf(filename, "/%04d_d%02d.png", folderCounter, i);
    display.drawCenterString(filename, display.width() / 2, display.height() / 2);
    savePNGToSD(dstSprites[i], filename);
  }

  // ==========================================
  // 2. digital8 (専用処理)
  // ==========================================
  applyDitherAndPalette_digital8(canvas_565_p, dstSprites[0]);
  sprintf(filename, "/%04d_d08.png", folderCounter);
  display.drawCenterString(filename, display.width() / 2, display.height() / 2);
  savePNGToSD(dstSprites[0], filename);

  // ==========================================
  // 3. raw画像保存
  // ==========================================
  sprintf(filename, "/%04d_raw.png", folderCounter);
  display.drawCenterString(filename, display.width() / 2, display.height() / 2);
  savePNGToSD(canvas_565_p, filename);

  currentPaletteIndex = 0;
}

// BURST撮影
void executeBurstProcess() {
  if (!CameraGet()) {
    Serial.println("ERROR: Camera capture failed (Burst).");
    errorLed();
    return;
  }
  canvas_565_v.pushImageRotateZoom(0, 0, 0, 16, 0, 0.534, 0.5, PICTURE_WIDTH_PIX, PICTURE_HEIGHT_PIX - 16, (uint16_t*)fb->buf);
  CameraFree();

  if (currentPaletteIndex == DIGITAL8_PALETTE_INDEX) {
    applyDitherAndPalette_digital8(canvas_565_v, canvas_888_v);
  } else {
    applyDitherAndPalette_dither8(canvas_565_v, canvas_888_v, ditheringLevels, currentPaletteIndex, enableDithering);
  }

  sprintf(filename, "%s/v%06d.png", foldername, fileCounter);
  savePNGToSD(canvas_888_v, filename);
  fileCounter++;  // 連番を更新
}

// TIMELAPSE撮影
void executeTimelapseProcess() {
  // タイマーチェック
  if (millis() - lastCaptureTime >= TIMELAPSE_INTERVAL) {
    if (!CameraGet()) {
      Serial.println("ERROR: Camera capture failed (Timelapse).");
      errorLed();
      lastCaptureTime = millis();  // 失敗してもタイマーをリセット
      return;
    }
    canvas_565_p.pushImage(0, 0, PICTURE_WIDTH_PIX, PICTURE_HEIGHT_PIX, (uint16_t*)fb->buf);
    CameraFree();  // フレームバッファを解放

    if (currentPaletteIndex == DIGITAL8_PALETTE_INDEX) {
      applyDitherAndPalette_digital8(canvas_565_p, dstSprites[0]);
    } else {
      applyDitherAndPalette_dither8(canvas_565_p, dstSprites[0], ditheringLevels, currentPaletteIndex, enableDithering);
    }

    // フォルダ内に連番で保存: /0001/t000001.png
    sprintf(filename, "%s/t%06d.png", foldername, fileCounter);
    Serial.printf("Saving Timelapse: %s\n", filename);
    if (savePNGToSD(dstSprites[0], filename)) {
      fileCounter++;  // 連番を更新
    } else {
      errorLed();
    }

    // 5. 撮影時刻を更新
    lastCaptureTime = millis();
  }
}

// 処理停止時に一度だけ実行される関数
void cleanupProcess() {
  Serial.println(">>> Process STOPPED! <<<");
  folderCounter = (folderCounter + 1) % 10000;
  writefolderCounter_internal(folderCounter);
  SD.end();
  fileCounter = 0;
  encoder.setLEDColor(0, 0x000000);
}

void errorLed() {
  encoder.setLEDColor(0, 0xFF0000);  // Red
}

void setModeLED() {
  if (isModeChanging) {
    encoder.setLEDColor(0, 0x000000);  // Black
  } else {
    switch (currentMode) {
      case NORMAL:
        encoder.setLEDColor(0, 0x00FFFF);  // Cyan
        break;
      case BURST:
        encoder.setLEDColor(0, 0x0000FF);  // Blue
        break;
      case TIMELAPSE:
        encoder.setLEDColor(0, 0x008000);  // Green
        break;
    }
  }
  if (isRecording) {
    encoder.setLEDColor(0, 0xFFA500);  // Orange
  }
}

void setup() {
  display.init(2, 1);
  display.setTextScroll(true);
  display.setRotation(1);
  delay(10);
  encoder.begin(&Wire, 0x40, 2, 1);

  Serial.begin(115200);
  pinMode(POWER_GPIO_NUM, OUTPUT);
  digitalWrite(POWER_GPIO_NUM, LOW);

  canvas_565_v.setColorDepth(16);
  canvas_565_v.createSprite(VIDEO_WIDTH_PIX, VIDEO_HEIGHT_PIX);
  canvas_565_v.setPsram(false);

  canvas_txt.setColorDepth(1);
  canvas_txt.createSprite(128, 12);

  Serial.println("Allocating 8 Canvases in PSRAM...");

  for (int i = 0; i < NUM_PALETTES; ++i) {

    dstSprites[i].setPsram(true);     // 描画先を必ずPSRAMに強制する
    dstSprites[i].setColorDepth(24);  // RGB888 (24bit) フォーマットを指定

    // Canvasの生成 (内部でPSRAMにメモリが確保される)
    if (dstSprites[i].createSprite(PICTURE_WIDTH_PIX, PICTURE_HEIGHT_PIX) == nullptr) {
      Serial.printf("Error: Failed to create Canvas %d. Check PSRAM.\n", i);
      errorLed();
      while (1) { delay(100); }
    }
  }

  Serial.println("Canvas allocation successful.");

  canvas_565_p.setColorDepth(16);
  canvas_565_p.createSprite(PICTURE_WIDTH_PIX, PICTURE_HEIGHT_PIX);

  canvas_888_v.setColorDepth(24);
  canvas_888_v.createSprite(VIDEO_WIDTH_PIX, VIDEO_HEIGHT_PIX);

  // 一度SDカードをマウントして確認
  SPI.begin(7, 8, 6, -1);
  if (!SD.begin(15, SPI, 10000000)) {
    Serial.println("SD Card initialization failed!");
    display.println("SD Card NG!");
    errorLed();
    delay(500);
    return;
  } else {
    Serial.println("SD Card initialized...");
    display.println("SD Card OK!");

    loadfolderCounter_internal();
    // パレット0から7までループ
    for (int i = 0; i <= maxPaletteIndex; i++) {
      if (loadPaletteFromSD(i)) {
        Serial.printf("Palette %d loaded from SD.\n", i);
        display.printf("Palette %d from SD\n", i);
      } else {
        Serial.printf("Palette %d use default.\n", i);
        display.printf("Palette %d default\n", i);
      }
      delay(10);
    }

    // digital8処理用の固定8色パレットの読み込み (無ければ現状のデフォルト値のまま)
    if (loadColorPalettes8FromSD()) {
      Serial.println("Palette 8 loaded from SD.");
      display.println("Palette 8 from SD");
    } else {
      Serial.println("Palette 8 use default.");
      display.println("Palette 8 default");
    }
    delay(10);

    // 8-Color Ditherモードの階調数読み込み (無ければ現状のデフォルト値のまま)
    if (loadDither8ParamsFromSD()) {
      Serial.println("Dither params loaded from SD.");
      display.println("Dither from SD");
    } else {
      Serial.println("Dither params use default.");
      display.println("Dither default");
    }
    delay(10);

    // digital8処理パラメータの読み込み (無ければ現状のデフォルト値のまま)
    if (loadDigital8ParamsFromSD()) {
      Serial.println("Digital 8 params loaded from SD.");
      display.println("Digital 8 from SD");
    } else {
      Serial.println("Digital 8 params use default.");
      display.println("Digital 8 default");
    }
    delay(10);
  }
  SD.end();  // 一旦ENDしておく

  if (psramFound()) {
    camera_config.pixel_format = PIXFORMAT_RGB565;
    camera_config.fb_location = CAMERA_FB_IN_PSRAM;
    camera_config.fb_count = 2;
  } else {
    errorLed();
    Serial.println("PSRAM not found!");
    display.println("PSRAM NG!");
    delay(500);
  }

  if (!CameraBegin()) {
    errorLed();
    Serial.println("Camera initialization failed!");
    display.println("Camera NG!");
    delay(1000);
    ESP.restart();
  }
  delay(10);
  Serial.println("Camera initialized...");
  display.println("Camera OK!");
  initSigmoidContrastLUT(d8_sigmoidStrength, d8_sigmoidMidpoint);
  delay(10);
  display.clear(TFT_BLACK);
}

void loop() {

  if (isModeChanging) {
    // --- モード切替画面 ---
    canvas_565_v.clear(TFT_BLACK);
    canvas_565_v.drawCenterString("--- MODE SELECT ---", 64, 10);
    int y = 24;
    for (int i = 0; i <= TIMELAPSE; i++) {
      String modeName = getModeName((ShootingMode)i);
      if ((ShootingMode)i == currentMode) {
        canvas_565_v.fillRect(0, y, 128, 16, TFT_BLACK);
        canvas_565_v.drawCenterString(">> " + modeName + " <<", 64, y);  // 選択中のモードをハイライト
      } else {
        canvas_565_v.drawCenterString(modeName, 64, y);
      }
      y += 16;
    }
  } else {
    // --- 通常画面 ---
    if (!CameraGet()) {
      Serial.println("ERROR: Camera capture failed (Preview).");
      errorLed();
      return;
    }
    canvas_565_v.pushImageRotateZoom(0, 0, 0, 16, 0, 0.534, 0.5, PICTURE_WIDTH_PIX, PICTURE_HEIGHT_PIX - 16, (uint16_t*)fb->buf);
    CameraFree();
  }

  // --------------------------------------------------
  // A. エンコーダ入力の取得と状態管理
  // --------------------------------------------------

  signed short int encoder_value = encoder.getEncoderValue();
  bool btn_status = encoder.getButtonStatus();                  // ボタンの状態 (押下: false, 非押下: true)
  signed short int delta = encoder_value - last_encoder_value;  // 回転量 (Delta) の計算

  // --------------------------------------------------
  // B. 回転操作 (ロータリー) の処理
  // --------------------------------------------------

  if (delta != 0) {
    accumulatedChange += delta;
    last_encoder_value = encoder_value;

    if (isModeChanging) {
      // モード切替画面 (長押し後)
      int modeInt = (int)currentMode;

      while (accumulatedChange >= 2) {
        modeInt++;  // モードを+1
        if (modeInt > 2) {
          modeInt = 2;
        }
        accumulatedChange -= 2;
      }

      while (accumulatedChange <= -2) {
        modeInt--;  // モードを-1
        if (modeInt < 0) {
          modeInt = 0;
        }
        accumulatedChange += 2;
      }

      currentMode = (ShootingMode)modeInt;
    } else if (currentMode == BURST || currentMode == TIMELAPSE) {

      int nextPalette = currentPaletteIndex;

      while (accumulatedChange >= 2) {
        nextPalette++;  // モードを+1
        if (nextPalette > maxPaletteSelectIndex) {
          nextPalette = 0;  // 0に戻る
        }
        accumulatedChange -= 2;
      }

      while (accumulatedChange <= -2) {
        nextPalette--;  // モードを-1
        if (nextPalette < 0) {
          nextPalette = maxPaletteSelectIndex;  // maxに戻る
        }
        accumulatedChange += 2;
      }

      currentPaletteIndex = nextPalette;

      if (currentPaletteIndex == DIGITAL8_PALETTE_INDEX) {
        Serial.println("Digital 8");
        sprintf(currentPaletteSetting, "Digital 8");
      } else {
        Serial.printf("Palette %d\n", currentPaletteIndex);
        sprintf(currentPaletteSetting, "Palette %d", currentPaletteIndex);
      }
      textDisplayStartTime = millis();
    }
  }

  // --------------------------------------------------
  // C. クリック操作 (短押し・長押し) の処理
  // --------------------------------------------------
  if (!btn_status && last_btn_status) {
    pressStartTime = millis();
  }

  // 長押し判定 (長押し検出後、ボタンを離すまで繰り返し判定しないように注意)
  bool isLongPress = !btn_status && (millis() - pressStartTime >= HOLD_DURATION);

  if (isLongPress && !isModeChanging) {
    // --- 長押し: モード切替画面への移行 (全モードで有効) ---
    display.clear(TFT_BLACK);
    isModeChanging = true;
    isRecording = false;
    pressStartTime = 0;  // 長押し判定をリセット
    cleanupProcess();
    Serial.println("--- MODECHANGING ---");

  } else if (btn_status && !last_btn_status) {
    unsigned long pressDuration = millis() - (pressStartTime > 0 ? pressStartTime : 0);

    if (pressDuration < HOLD_DURATION) {
      // --- 短いクリック判定 ---
      if (isModeChanging) {
        // 1. モード切替画面でのクリック: モード確定
        isModeChanging = false;
        Serial.printf("Current Mode: %s\n", getModeName(currentMode).c_str());
      } else {
        // 2. 通常画面でのクリック: 撮影開始/停止
        if (!initializeProcess()) {
          return;  // SDマウント失敗なら撮影処理に進まない
        }
        if (currentMode == NORMAL) {
          if (!isRecording) {
            isRecording = true;
            Serial.println("--- Start Recording (NORMAL) ---");
            executeSingleProcess();
            cleanupProcess();
            isRecording = false;
            Serial.println("--- Stop Recording (NORMAL) ---");
          }
        } else if (currentMode == BURST || currentMode == TIMELAPSE) {
          isRecording = !isRecording;
          if (isRecording) {
            makeDir();
            Serial.println("--- Start Recording (BURST/TIMELAPSE) ---");
          } else {
            Serial.println("--- Stop Recording (BURST/TIMELAPSE) ---");
            cleanupProcess();
          }
        }
      }
    }
    pressStartTime = 0;  // 短押し処理後、タイマーリセット
  }

  // 次のループのためのボタン状態を保存
  last_btn_status = btn_status;

  if (currentMode == BURST && isRecording == true) {
    executeBurstProcess();
  }

  if (currentMode == TIMELAPSE && isRecording == true) {
    executeTimelapseProcess();
  }

  if (textDisplayStartTime > 0) {
    if (millis() - textDisplayStartTime < TEXT_DISPLAY_DURATION) {
      canvas_txt.clear(TFT_BLACK);
      canvas_txt.drawCenterString(currentPaletteSetting, 64, 2);
    } else {
      canvas_txt.clear(TFT_BLACK);
      textDisplayStartTime = 0;
    }
    canvas_txt.pushSprite(&canvas_565_v, 0, 56);
  }
  canvas_565_v.pushSprite(&display, 0, -4);

  setModeLED();
}
