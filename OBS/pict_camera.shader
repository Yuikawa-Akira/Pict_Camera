// ============================================================
// Pict Camera - Pixel Art Dither Shader for OBS
// ============================================================
// Ported from Pict_Camera_Editor.html (https://github.com/Yuikawa-Akira/Pict_Camera)
//
// Requires: obs-shaderfilter plugin (exeldro/obs-shaderfilter)
//   https://github.com/exeldro/obs-shaderfilter
//
// Usage:
//   1. Right-click a source -> Filters -> Effect Filters -> Add -> User-defined shader
//   2. Enable "Load shader text from file" and select this .shader file
//      (or paste its contents directly into the shader text box)
//   3. Adjust the exposed properties (Mode, Saturation, Contrast, Palette colors, ...)
//
// This is a .shader (pixel-shader-only) file, so "Use Effect File (.effect)"
// should stay UNCHECKED. The plugin supplies the vertex shader, ViewProj,
// image, and VertData automatically.
// ============================================================

// NOTE: obs-shaderfilter's slider range (minimum/maximum/step) is only
// reliably enforced for `uniform float` parameters. `uniform int` sliders
// have been observed to ignore the annotation's min/max in some plugin
// versions, letting the handle be dragged past the intended range. To make
// the on-screen slider actually respect the limits, pc_mode and pc_levels
// are declared as float here and rounded to whole numbers before use.

uniform float pc_mode<
    string label = "Mode (0 = Digtal 8, 1 = Dither 8)";
    string widget_type = "slider";
    float minimum = 0.0;
    float maximum = 1.0;
    float step = 1.0;
> = 0.0;

// --- Pixelation pre-process (applied before either mode's color processing) ---
// 1.0 = no pixelation (full source resolution). Larger values merge more
// source pixels into a single block, producing a chunkier pixel-art look.
uniform float pc_pixel_size<
    string label = "Pixelation Block Size";
    string widget_type = "slider";
    float minimum = 1.0;
    float maximum = 32.0;
    float step = 1.0;
> = 4.0;

// --- Digtal 8 params (mode 0) ---
uniform float pc_saturation<
    string label = "Saturation (Digtal 8)";
    string widget_type = "slider";
    float minimum = 0.0;
    float maximum = 4.0;
    float step = 0.05;
> = 2.0; // matches sat_factor_x128 = 256

uniform float pc_sigmoid_strength<
    string label = "Contrast Strength (Digtal 8)";
    string widget_type = "slider";
    float minimum = 1.0;
    float maximum = 30.0;
    float step = 0.5;
> = 24.0;

uniform float pc_sigmoid_midpoint<
    string label = "Contrast Midpoint (Digtal 8)";
    string widget_type = "slider";
    float minimum = 0.2;
    float maximum = 0.8;
    float step = 0.01;
> = 0.50;

uniform float pc_bias_width<
    string label = "Dither Bias Width (Digtal 8)";
    string widget_type = "slider";
    float minimum = 0.0;
    float maximum = 40.0;
    float step = 1.0;
> = 12.0;

// --- Dither 8 params (mode 1) ---
uniform float pc_levels<
    string label = "Dither Levels (Dither 8)";
    string widget_type = "slider";
    float minimum = 1.0;
    float maximum = 16.0;
    float step = 1.0;
> = 8.0;

uniform float pc_dither_palette<
    string label = "Dither Palette (0-7=PRESET, 8=CUSTOM)";
    string widget_type = "slider";
    float minimum = 0.0;
    float maximum = 8.0;
    float step = 1.0;
> = 0.0;

// Used only when Dither Palette is set to 8 (CUSTOM).  The defaults are
// SLSO8, making palette 8 a convenient editable copy of palette 0.
uniform float4 pc_custom_pal0<string label = "Custom Dither Color 0 (dark)";>  = {0.0510, 0.1686, 0.2706, 1.0};
uniform float4 pc_custom_pal1<string label = "Custom Dither Color 1";>         = {0.1255, 0.2353, 0.3373, 1.0};
uniform float4 pc_custom_pal2<string label = "Custom Dither Color 2";>         = {0.3294, 0.3059, 0.4078, 1.0};
uniform float4 pc_custom_pal3<string label = "Custom Dither Color 3";>         = {0.5529, 0.4118, 0.4784, 1.0};
uniform float4 pc_custom_pal4<string label = "Custom Dither Color 4";>         = {0.8157, 0.5059, 0.3490, 1.0};
uniform float4 pc_custom_pal5<string label = "Custom Dither Color 5";>         = {1.0000, 0.6667, 0.3686, 1.0};
uniform float4 pc_custom_pal6<string label = "Custom Dither Color 6";>         = {1.0000, 0.8314, 0.6392, 1.0};
uniform float4 pc_custom_pal7<string label = "Custom Dither Color 7 (light)";> = {1.0000, 0.9255, 0.8392, 1.0};

// --- Fixed 8-color palette used by Digtal 8 mode (mode 0) ---
uniform float4 pc_fixed0<string label = "Digtal 8 Color 0";> = {0.0, 0.0, 0.0, 1.0};
uniform float4 pc_fixed1<string label = "Digtal 8 Color 1";> = {0.0, 0.0, 1.0, 1.0};
uniform float4 pc_fixed2<string label = "Digtal 8 Color 2";> = {0.0, 1.0, 0.0, 1.0};
uniform float4 pc_fixed3<string label = "Digtal 8 Color 3";> = {0.0, 1.0, 1.0, 1.0};
uniform float4 pc_fixed4<string label = "Digtal 8 Color 4";> = {1.0, 0.0, 0.0, 1.0};
uniform float4 pc_fixed5<string label = "Digtal 8 Color 5";> = {1.0, 0.0, 1.0, 1.0};
uniform float4 pc_fixed6<string label = "Digtal 8 Color 6";> = {1.0, 1.0, 0.0, 1.0};
uniform float4 pc_fixed7<string label = "Digtal 8 Color 7";> = {1.0, 1.0, 1.0, 1.0};

// --- Built-in palettes for Dither 8 mode (mode 1) ---
// The selected palette and luminance level are both expressed with simple
// conditionals for compatibility with OBS's HLSL dialect.
float3 getDitherPaletteColor(int palette, int level)
{
    // Palette 8: user-configurable custom palette
    if (palette == 8) {
        if (level == 0) return pc_custom_pal0.rgb;
        if (level == 1) return pc_custom_pal1.rgb;
        if (level == 2) return pc_custom_pal2.rgb;
        if (level == 3) return pc_custom_pal3.rgb;
        if (level == 4) return pc_custom_pal4.rgb;
        if (level == 5) return pc_custom_pal5.rgb;
        if (level == 6) return pc_custom_pal6.rgb;
        return pc_custom_pal7.rgb;
    }

    // Palette 0: SLSO8, created by Luis Miguel Maldonado
    if (palette == 0) {
        if (level == 0) return float3(0.0510, 0.1686, 0.2706);
        if (level == 1) return float3(0.1255, 0.2353, 0.3373);
        if (level == 2) return float3(0.3294, 0.3059, 0.4078);
        if (level == 3) return float3(0.5529, 0.4118, 0.4784);
        if (level == 4) return float3(0.8157, 0.5059, 0.3490);
        if (level == 5) return float3(1.0000, 0.6667, 0.3686);
        if (level == 6) return float3(1.0000, 0.8314, 0.6392);
        return float3(1.0000, 0.9255, 0.8392);
    }

    // Palette 1: 2BIT DEMIBOY, created by Space Sandwich
    if (palette == 1) {
        if (level <= 1) return float3(0.1451, 0.1451, 0.1451);
        if (level <= 3) return float3(0.2941, 0.3373, 0.3020);
        if (level <= 5) return float3(0.6039, 0.6471, 0.4863);
        return float3(0.8784, 0.9137, 0.7686);
    }

    // Palette 2: TWILIGHT 5, created by Star
    if (palette == 2) {
        if (level <= 1) return float3(0.1608, 0.1569, 0.1922);
        if (level <= 3) return float3(0.2000, 0.2471, 0.3451);
        if (level == 4) return float3(0.2902, 0.4784, 0.5882);
        if (level <= 6) return float3(0.9333, 0.5255, 0.5843);
        return float3(0.9843, 0.7333, 0.6784);
    }

    // Palette 3: BLUE SKY 5, created by Yuikawa-Akira
    if (palette == 3) {
        if (level == 0) return float3(0.0510, 0.4627, 0.8510);
        if (level <= 2) return float3(0.2353, 0.6784, 0.9255);
        if (level <= 4) return float3(0.4431, 0.8039, 0.9882);
        if (level <= 6) return float3(0.7373, 0.8863, 0.9922);
        return float3(1.0000, 1.0000, 1.0000);
    }

    // Palette 4: MUMBLEBEE PALETTE, created by sudo_whoami
    if (palette == 4) {
        if (level <= 3) return float3(0.2000, 0.2667, 0.3333);
        return float3(1.0000, 0.7333, 0.2000);
    }

    // Palette 5: DREAM HAZE 8, created by Klafooty
    if (palette == 5) {
        if (level == 0) return float3(0.2353, 0.2588, 0.7686);
        if (level == 1) return float3(0.4314, 0.3176, 0.7843);
        if (level == 2) return float3(0.6275, 0.3961, 0.8039);
        if (level == 3) return float3(0.8078, 0.4745, 0.8235);
        if (level == 4) return float3(0.8392, 0.5608, 0.7216);
        if (level == 5) return float3(0.8667, 0.6353, 0.6392);
        if (level == 6) return float3(0.9176, 0.7686, 0.6824);
        return float3(0.9569, 0.8745, 0.7451);
    }

    // Palette 6: DEEP MAZE, created by Ryosuke
    if (palette == 6) {
        if (level == 0) return float3(0.0000, 0.1137, 0.1647);
        if (level == 1) return float3(0.0314, 0.3333, 0.3843);
        if (level == 2) return float3(0.0000, 0.6039, 0.5961);
        if (level == 3) return float3(0.0000, 0.7451, 0.5686);
        if (level == 4) return float3(0.2196, 0.8471, 0.5569);
        if (level == 5) return float3(0.6039, 0.9412, 0.5373);
        return float3(0.9490, 1.0000, 0.4000);
    }

    // Palette 7: NIGHT RAIN, created by Maber
    if (level == 0) return float3(0.0000, 0.0000, 0.0000);
    if (level == 1) return float3(0.0039, 0.1255, 0.2118);
    if (level == 2) return float3(0.2275, 0.4824, 0.6667);
    if (level == 3) return float3(0.4902, 0.5608, 0.6824);
    if (level == 4) return float3(0.6314, 0.7059, 0.7569);
    if (level == 5) return float3(0.9412, 0.7255, 0.7255);
    if (level == 6) return float3(1.0000, 0.8196, 0.3490);
    return float3(1.0000, 1.0000, 1.0000);
}

// NOTE: `textureSampler` is provided automatically by obs-shaderfilter.
// Do NOT declare your own `sampler_state textureSampler { ... }` block —
// doing so conflicts with the one the plugin already injects into the
// wrapped template and can produce confusing parser errors.

// --- Pixelation pre-process ---
// Computes the block index for a given source pixel: every pixel that maps
// to the same block index belongs to the same NxN mosaic cell, where
// N = pixelSize.
float2 pixelateBlockCoord(float2 uv, float2 texSize, float pixelSize)
{
    float ps = max(pixelSize, 1.0);
    return floor(uv * texSize / ps);
}

// Converts a block index back into the UV of that block's center pixel.
// Sampling at this UV (instead of the original per-pixel UV) is what makes
// every pixel inside the block read the same flat color — the standard
// "mosaic" / pixelation technique.
float2 pixelateSnappedUV(float2 blockCoord, float2 texSize, float pixelSize)
{
    float ps = max(pixelSize, 1.0);
    float2 snappedPixelCoord = blockCoord * ps + (ps * 0.5);
    return snappedPixelCoord / texSize;
}

// 4x4 Bayer matrix, normalized 0..15, looked up without any array indexing
// (kept as plain conditionals for maximum compatibility with OBS's HLSL dialect).
float getBayerValue(float2 pixelCoord)
{
    int xi = (int)(fmod(pixelCoord.x, 4.0));
    int yi = (int)(fmod(pixelCoord.y, 4.0));

    if (yi == 0) {
        if (xi == 0) return 0.0;
        if (xi == 1) return 8.0;
        if (xi == 2) return 2.0;
        return 10.0;
    } else if (yi == 1) {
        if (xi == 0) return 12.0;
        if (xi == 1) return 4.0;
        if (xi == 2) return 14.0;
        return 6.0;
    } else if (yi == 2) {
        if (xi == 0) return 3.0;
        if (xi == 1) return 11.0;
        if (xi == 2) return 1.0;
        return 9.0;
    } else {
        if (xi == 0) return 15.0;
        if (xi == 1) return 7.0;
        if (xi == 2) return 13.0;
        return 5.0;
    }
}

// --- applyModulate_integer, ported to normalized (0-1) float math ---
// satFactor: multiplier such that 2.0 == "sat_factor_x128=256" in the original code
float3 applyModulate(float3 c, float satFactor)
{
    float cmax = max(c.r, max(c.g, c.b));
    float cmin = min(c.r, min(c.g, c.b));
    float delta = cmax - cmin;

    if (delta < 1e-6 || cmax < 1e-6) {
        return c;
    }

    float newChroma = min(delta * satFactor, cmax);
    float m = cmax - newChroma;

    float3 outc;
    if (cmax == c.r) {
        if (c.g >= c.b) {
            outc = float3(cmax, m + newChroma * (c.g - cmin) / delta, m);
        } else {
            outc = float3(cmax, m, m + newChroma * (c.b - cmin) / delta);
        }
    } else if (cmax == c.g) {
        if (c.r >= c.b) {
            outc = float3(m + newChroma * (c.r - cmin) / delta, cmax, m);
        } else {
            outc = float3(m, cmax, m + newChroma * (c.b - cmin) / delta);
        }
    } else {
        if (c.g >= c.r) {
            outc = float3(m, m + newChroma * (c.g - cmin) / delta, cmax);
        } else {
            outc = float3(m + newChroma * (c.r - cmin) / delta, m, cmax);
        }
    }
    return outc;
}

// --- initSigmoidContrastLUT, evaluated per-pixel instead of via a lookup table ---
float sigmoidContrast(float x, float strength, float midpoint)
{
    float sig0 = 1.0 / (1.0 + exp(-strength * (0.0 - midpoint)));
    float sig1 = 1.0 / (1.0 + exp(-strength * (1.0 - midpoint)));

    float y = x * (sig1 - sig0) + sig0;
    y = clamp(y, 0.0001, 0.9999);

    float result = midpoint - log(1.0 / y - 1.0) / strength;
    return saturate(result);
}

float3 applySigmoid(float3 c, float strength, float midpoint)
{
    return float3(
        sigmoidContrast(c.r, strength, midpoint),
        sigmoidContrast(c.g, strength, midpoint),
        sigmoidContrast(c.b, strength, midpoint)
    );
}

// --- quantize_color_to_closest against the fixed 8-color palette (weighted R:2 G:4 B:3) ---
// Written without arrays/loops for maximum compatibility with OBS's HLSL dialect.
float weightedDistSq(float3 c, float3 p)
{
    float3 d = c - p;
    return 2.0 * d.r * d.r + 4.0 * d.g * d.g + 3.0 * d.b * d.b;
}

float3 quantizeToClosestFixed(float3 c)
{
    float3 best = pc_fixed0.rgb;
    float bestDist = weightedDistSq(c, pc_fixed0.rgb);

    float d1 = weightedDistSq(c, pc_fixed1.rgb);
    if (d1 < bestDist) { bestDist = d1; best = pc_fixed1.rgb; }

    float d2 = weightedDistSq(c, pc_fixed2.rgb);
    if (d2 < bestDist) { bestDist = d2; best = pc_fixed2.rgb; }

    float d3 = weightedDistSq(c, pc_fixed3.rgb);
    if (d3 < bestDist) { bestDist = d3; best = pc_fixed3.rgb; }

    float d4 = weightedDistSq(c, pc_fixed4.rgb);
    if (d4 < bestDist) { bestDist = d4; best = pc_fixed4.rgb; }

    float d5 = weightedDistSq(c, pc_fixed5.rgb);
    if (d5 < bestDist) { bestDist = d5; best = pc_fixed5.rgb; }

    float d6 = weightedDistSq(c, pc_fixed6.rgb);
    if (d6 < bestDist) { bestDist = d6; best = pc_fixed6.rgb; }

    float d7 = weightedDistSq(c, pc_fixed7.rgb);
    if (d7 < bestDist) { bestDist = d7; best = pc_fixed7.rgb; }

    return best;
}

// --- Digtal 8 mode ---
float4 processVivid8Color(float2 uv, float2 pixelCoord)
{
    float3 c = image.Sample(textureSampler, uv).rgb;

    // Stage 1: saturation boost
    c = applyModulate(c, pc_saturation);

    // Stage 2: inverse sigmoid contrast
    c = applySigmoid(c, pc_sigmoid_strength, pc_sigmoid_midpoint);

    // Stage 3: centered Bayer bias + 8-level quantize
    float bv = getBayerValue(pixelCoord);
    // Mirrors: bayer_bias = ((bv * (biasWidth*2)) / 16) - biasWidth   [0..255 integer domain]
    // normalized to the 0..1 float domain used by this shader
    float bayerBias01 = (bv * (pc_bias_width * 2.0) / 16.0 - pc_bias_width) / 255.0;

    float3 biased = saturate(c + bayerBias01);
    float3 step7 = round(biased * 7.0);
    float3 quantized = step7 / 7.0;

    // Stage 4: nearest-neighbor mapping onto the fixed 8-color palette
    float3 result = quantizeToClosestFixed(quantized);

    return float4(result, 1.0);
}

// --- Dither 8 mode ---
float4 process8ColorDither(float2 uv, float2 pixelCoord)
{
    float3 c = image.Sample(textureSampler, uv).rgb;
    float3 dithered;

    // Round first (the value comes from a float slider), then clamp to the
    // valid range regardless of what the UI slider currently allows.
    int levels = clamp((int)round(pc_levels), 1, 16);

    if (levels <= 1) {
        dithered = c;
    } else {
        float intervals = float(levels - 1);
        float bv = getBayerValue(pixelCoord) / 16.0; // 0..0.9375

        float3 valScaled = c * intervals;
        float3 lIdx = floor(valScaled);
        lIdx = min(lIdx, intervals - 1.0);
        float3 frac = valScaled - lIdx;

        // frac >= bv  <=>  errorScaled16 >= bayerThresh (both sides scaled equivalently)
        lIdx.r += (frac.r >= bv) ? 1.0 : 0.0;
        lIdx.g += (frac.g >= bv) ? 1.0 : 0.0;
        lIdx.b += (frac.b >= bv) ? 1.0 : 0.0;

        dithered = lIdx / intervals;
    }

    // Luminance (matches the original 54/183/19 over 256 integer weights)
    float luminance = dot(dithered, float3(54.0, 183.0, 19.0) / 256.0);
    int grayLevel = clamp(int(luminance * 8.0), 0, 7);

    // Round and clamp the slider value so an out-of-range UI value still
    // resolves to one of the eight presets or the custom palette.
    int palette = clamp((int)round(pc_dither_palette), 0, 8);
    float3 result = getDitherPaletteColor(palette, grayLevel);

    return float4(result, 1.0);
}

float4 mainImage(VertData v_in) : TARGET
{
    // --- Pixelation pre-process ---
    // Snap the sampling UV onto a coarse grid so that every pixel inside a
    // block reads the same source color, then use the block index (rather
    // than the raw source pixel index) for the Bayer dither pattern so the
    // dither grain matches the pixelated block size.
    float2 blockCoord = pixelateBlockCoord(v_in.uv, uv_size, pc_pixel_size);
    float2 pixelatedUV = pixelateSnappedUV(blockCoord, uv_size, pc_pixel_size);

    // Round first (the value comes from a float slider), then clamp to the
    // valid range regardless of what the UI slider currently allows.
    int mode = clamp((int)round(pc_mode), 0, 1);

    if (mode == 0) {
        return processVivid8Color(pixelatedUV, blockCoord);
    } else {
        return process8ColorDither(pixelatedUV, blockCoord);
    }
}
