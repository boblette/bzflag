/* bzflag
 * Copyright (c) 1993 - 2009 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named LICENSE that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "common.h"

// interface header
#include "ParseColor.h"

// system headers
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <map>

// common headers
#include "TextUtils.h"
#include "vectors.h"


//============================================================================//
//============================================================================//

struct ColorMapData {
  ColorMapData()
  {
    data = fvec4(1.0f, 1.0f, 1.0f, 1.0f);
  }
  ColorMapData(float r, float g, float b, float a = 1.0f)
  {
    data = fvec4(r, g, b, a);
  }
  fvec4 data;
};

typedef std::map<std::string, ColorMapData> ColorMap;

static const ColorMap& getColorMap();


//============================================================================//
//============================================================================//

static int parseHexChar(char c)
{
  if ((c >= '0') && (c <= '9')) {
    return (c - '0');
  }
  c = tolower(c);
  if ((c >= 'a') && (c <= 'f')) {
    return (c - 'a') + 10;
  }
  return -1;
}


static bool parseHexFormat(const char* str, fvec4& color)
{
  int bytes[8];
  int index;
  for (index = 0; index < 8; index++) {
    const char c = str[index];
    if ((c == 0) || isspace(c)) {
      break;
    }
    const int byte = parseHexChar(c);
    if (byte < 0) {
      return false; // not a hex character
    }
    bytes[index] = byte;
  }

  // check for a valid termination
  if (index == 8) {
    const char c = str[8];
    if ((c != 0) && !isspace(c)) {
      return false;
    }
  }

  switch (index) {
    case 3: { // rgb
      color[0] = (float)bytes[0] / 15.0f;
      color[1] = (float)bytes[1] / 15.0f;
      color[2] = (float)bytes[2] / 15.0f;
      return true;
    }
    case 4: { // rgba
      color[0] = (float)bytes[0] / 15.0f;
      color[1] = (float)bytes[1] / 15.0f;
      color[2] = (float)bytes[2] / 15.0f;
      color[3] = (float)bytes[3] / 15.0f;
      return true;
    }
    case 6: { // rrggbb
      color[0] = (float)((bytes[0] << 4) + bytes[1]) / 255.0f;
      color[1] = (float)((bytes[2] << 4) + bytes[3]) / 255.0f;
      color[2] = (float)((bytes[4] << 4) + bytes[5]) / 255.0f;
      return true;
    }
    case 8: { // rrggbbaa
      color[0] = (float)((bytes[0] << 4) + bytes[1]) / 255.0f;
      color[1] = (float)((bytes[2] << 4) + bytes[3]) / 255.0f;
      color[2] = (float)((bytes[4] << 4) + bytes[5]) / 255.0f;
      color[3] = (float)((bytes[6] << 4) + bytes[7]) / 255.0f;
      return true;
    }
  }

  return false;
}


//============================================================================//
//============================================================================//

static bool parseFloatFormat(const char* str, fvec4& color)
{
  int count;
  float tmp[4];
  count = sscanf(str, "%f %f %f %f", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
  if (count < 3) {
    return false;
  }
  memcpy(color, tmp, count * sizeof(float));
  return true;
}


//============================================================================//
//============================================================================//

static bool parseNamedFormat(const char* str, fvec4& color)
{
  const char* end = TextUtils::skipNonWhitespace(str);
  const size_t nameLen = (end - str);
  if (nameLen <= 0) {
    return false;
  }
  const std::string name(str, nameLen);

  const ColorMap& colorMap = getColorMap();
  ColorMap::const_iterator it = colorMap.find(TextUtils::tolower(name));
  if (it == colorMap.end()) {
    return false;
  }
  memcpy(color, it->second.data, sizeof(float[3]));

  str = TextUtils::skipWhitespace(end);
  if (*str == 0) {
    return true;
  }

  float alpha;
  if (sscanf(str, "%f", &alpha) > 0) {
    color[3] = alpha;
  }

  return true;
}


//============================================================================//
//============================================================================//

bool parseColorCString(const char* str, fvec4& color)
{
  static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

  // default to opaque white
  memcpy(color, white, sizeof(float[4]));

  // strip leading space
  str = TextUtils::skipWhitespace(str);

  // no string
  if (str[0] == 0) {
    return false;
  }

  // hexadecimal format (#rgb, #rgba, #rrggbb, or #rrggbbaa)
  if (str[0] == '#') {
    return parseHexFormat(str + 1, color);
  }

  // hexadecimal format (0xRGB, 0xRGBA, 0xRRGGBB, or 0xRRGGBBAA)
  if ((str[0] == '0') && (str[1] == 'x')) {
    return parseHexFormat(str + 2, color);
  }

  // float format (either 3 or 4 floating point values)
  if (((str[0] >= '0') && (str[0] <= '9'))
      || (str[0] == '.')
      || (str[0] == '+')
      || (str[0] == '-')) {
    return parseFloatFormat(str, color);
  }

  // named string format  ("red 0.2" format is accepted for alpha values)
  return parseNamedFormat(str, color);
}


bool parseColorString(const std::string& str, fvec4& color)
{
  return parseColorCString(str.c_str(), color);
}


bool parseColorStream(std::istream& input, fvec4& color)
{
  std::string line;
  std::getline(input, line);
  input.putback('\n');

  return parseColorString(line, color);
}


//============================================================================//
//============================================================================//

static const ColorMap& getColorMap()
{
  static ColorMap colorMap;

  if (!colorMap.empty()) {
    return colorMap;
  }

  struct ColorArrayData {
    const char* name;
    float r;
    float g;
    float b;
  };

  // these name/color pairs were generated using the X11 rgb.txt file
  static const ColorArrayData colorArray[] = {
    { "snow",                   1.000000f, 0.980392f, 0.980392f },
    { "ghost_white",            0.972549f, 0.972549f, 1.000000f },
    { "GhostWhite",             0.972549f, 0.972549f, 1.000000f },
    { "white_smoke",            0.960784f, 0.960784f, 0.960784f },
    { "WhiteSmoke",             0.960784f, 0.960784f, 0.960784f },
    { "gainsboro",              0.862745f, 0.862745f, 0.862745f },
    { "floral_white",           1.000000f, 0.980392f, 0.941176f },
    { "FloralWhite",            1.000000f, 0.980392f, 0.941176f },
    { "old_lace",               0.992157f, 0.960784f, 0.901961f },
    { "OldLace",                0.992157f, 0.960784f, 0.901961f },
    { "linen",                  0.980392f, 0.941176f, 0.901961f },
    { "antique_white",          0.980392f, 0.921569f, 0.843137f },
    { "AntiqueWhite",           0.980392f, 0.921569f, 0.843137f },
    { "papaya_whip",            1.000000f, 0.937255f, 0.835294f },
    { "PapayaWhip",             1.000000f, 0.937255f, 0.835294f },
    { "blanched_almond",        1.000000f, 0.921569f, 0.803922f },
    { "BlanchedAlmond",         1.000000f, 0.921569f, 0.803922f },
    { "bisque",                 1.000000f, 0.894118f, 0.768627f },
    { "peach_puff",             1.000000f, 0.854902f, 0.725490f },
    { "PeachPuff",              1.000000f, 0.854902f, 0.725490f },
    { "navajo_white",           1.000000f, 0.870588f, 0.678431f },
    { "NavajoWhite",            1.000000f, 0.870588f, 0.678431f },
    { "moccasin",               1.000000f, 0.894118f, 0.709804f },
    { "cornsilk",               1.000000f, 0.972549f, 0.862745f },
    { "ivory",                  1.000000f, 1.000000f, 0.941176f },
    { "lemon_chiffon",          1.000000f, 0.980392f, 0.803922f },
    { "LemonChiffon",           1.000000f, 0.980392f, 0.803922f },
    { "seashell",               1.000000f, 0.960784f, 0.933333f },
    { "honeydew",               0.941176f, 1.000000f, 0.941176f },
    { "mint_cream",             0.960784f, 1.000000f, 0.980392f },
    { "MintCream",              0.960784f, 1.000000f, 0.980392f },
    { "azure",                  0.941176f, 1.000000f, 1.000000f },
    { "alice_blue",             0.941176f, 0.972549f, 1.000000f },
    { "AliceBlue",              0.941176f, 0.972549f, 1.000000f },
    { "lavender",               0.901961f, 0.901961f, 0.980392f },
    { "lavender_blush",         1.000000f, 0.941176f, 0.960784f },
    { "LavenderBlush",          1.000000f, 0.941176f, 0.960784f },
    { "misty_rose",             1.000000f, 0.894118f, 0.882353f },
    { "MistyRose",              1.000000f, 0.894118f, 0.882353f },
    { "white",                  1.000000f, 1.000000f, 1.000000f },
    { "black",                  0.000000f, 0.000000f, 0.000000f },
    { "dark_slate_gray",        0.184314f, 0.309804f, 0.309804f },
    { "DarkSlateGray",          0.184314f, 0.309804f, 0.309804f },
    { "dark_slate_grey",        0.184314f, 0.309804f, 0.309804f },
    { "DarkSlateGrey",          0.184314f, 0.309804f, 0.309804f },
    { "dim_gray",               0.411765f, 0.411765f, 0.411765f },
    { "DimGray",                0.411765f, 0.411765f, 0.411765f },
    { "dim_grey",               0.411765f, 0.411765f, 0.411765f },
    { "DimGrey",                0.411765f, 0.411765f, 0.411765f },
    { "slate_gray",             0.439216f, 0.501961f, 0.564706f },
    { "SlateGray",              0.439216f, 0.501961f, 0.564706f },
    { "slate_grey",             0.439216f, 0.501961f, 0.564706f },
    { "SlateGrey",              0.439216f, 0.501961f, 0.564706f },
    { "light_slate_gray",       0.466667f, 0.533333f, 0.600000f },
    { "LightSlateGray",         0.466667f, 0.533333f, 0.600000f },
    { "light_slate_grey",       0.466667f, 0.533333f, 0.600000f },
    { "LightSlateGrey",         0.466667f, 0.533333f, 0.600000f },
    { "gray",                   0.745098f, 0.745098f, 0.745098f },
    { "grey",                   0.745098f, 0.745098f, 0.745098f },
    { "light_grey",             0.827451f, 0.827451f, 0.827451f },
    { "LightGrey",              0.827451f, 0.827451f, 0.827451f },
    { "light_gray",             0.827451f, 0.827451f, 0.827451f },
    { "LightGray",              0.827451f, 0.827451f, 0.827451f },
    { "midnight_blue",          0.098039f, 0.098039f, 0.439216f },
    { "MidnightBlue",           0.098039f, 0.098039f, 0.439216f },
    { "navy",                   0.000000f, 0.000000f, 0.501961f },
    { "navy_blue",              0.000000f, 0.000000f, 0.501961f },
    { "NavyBlue",               0.000000f, 0.000000f, 0.501961f },
    { "cornflower_blue",        0.392157f, 0.584314f, 0.929412f },
    { "CornflowerBlue",         0.392157f, 0.584314f, 0.929412f },
    { "dark_slate_blue",        0.282353f, 0.239216f, 0.545098f },
    { "DarkSlateBlue",          0.282353f, 0.239216f, 0.545098f },
    { "slate_blue",             0.415686f, 0.352941f, 0.803922f },
    { "SlateBlue",              0.415686f, 0.352941f, 0.803922f },
    { "medium_slate_blue",      0.482353f, 0.407843f, 0.933333f },
    { "MediumSlateBlue",        0.482353f, 0.407843f, 0.933333f },
    { "light_slate_blue",       0.517647f, 0.439216f, 1.000000f },
    { "LightSlateBlue",         0.517647f, 0.439216f, 1.000000f },
    { "medium_blue",            0.000000f, 0.000000f, 0.803922f },
    { "MediumBlue",             0.000000f, 0.000000f, 0.803922f },
    { "royal_blue",             0.254902f, 0.411765f, 0.882353f },
    { "RoyalBlue",              0.254902f, 0.411765f, 0.882353f },
    { "blue",                   0.000000f, 0.000000f, 1.000000f },
    { "dodger_blue",            0.117647f, 0.564706f, 1.000000f },
    { "DodgerBlue",             0.117647f, 0.564706f, 1.000000f },
    { "deep_sky_blue",          0.000000f, 0.749020f, 1.000000f },
    { "DeepSkyBlue",            0.000000f, 0.749020f, 1.000000f },
    { "sky_blue",               0.529412f, 0.807843f, 0.921569f },
    { "SkyBlue",                0.529412f, 0.807843f, 0.921569f },
    { "light_sky_blue",         0.529412f, 0.807843f, 0.980392f },
    { "LightSkyBlue",           0.529412f, 0.807843f, 0.980392f },
    { "steel_blue",             0.274510f, 0.509804f, 0.705882f },
    { "SteelBlue",              0.274510f, 0.509804f, 0.705882f },
    { "light_steel_blue",       0.690196f, 0.768627f, 0.870588f },
    { "LightSteelBlue",         0.690196f, 0.768627f, 0.870588f },
    { "light_blue",             0.678431f, 0.847059f, 0.901961f },
    { "LightBlue",              0.678431f, 0.847059f, 0.901961f },
    { "powder_blue",            0.690196f, 0.878431f, 0.901961f },
    { "PowderBlue",             0.690196f, 0.878431f, 0.901961f },
    { "pale_turquoise",         0.686275f, 0.933333f, 0.933333f },
    { "PaleTurquoise",          0.686275f, 0.933333f, 0.933333f },
    { "dark_turquoise",         0.000000f, 0.807843f, 0.819608f },
    { "DarkTurquoise",          0.000000f, 0.807843f, 0.819608f },
    { "medium_turquoise",       0.282353f, 0.819608f, 0.800000f },
    { "MediumTurquoise",        0.282353f, 0.819608f, 0.800000f },
    { "turquoise",              0.250980f, 0.878431f, 0.815686f },
    { "cyan",                   0.000000f, 1.000000f, 1.000000f },
    { "light_cyan",             0.878431f, 1.000000f, 1.000000f },
    { "LightCyan",              0.878431f, 1.000000f, 1.000000f },
    { "cadet_blue",             0.372549f, 0.619608f, 0.627451f },
    { "CadetBlue",              0.372549f, 0.619608f, 0.627451f },
    { "medium_aquamarine",      0.400000f, 0.803922f, 0.666667f },
    { "MediumAquamarine",       0.400000f, 0.803922f, 0.666667f },
    { "aquamarine",             0.498039f, 1.000000f, 0.831373f },
    { "dark_green",             0.000000f, 0.392157f, 0.000000f },
    { "DarkGreen",              0.000000f, 0.392157f, 0.000000f },
    { "dark_olive_green",       0.333333f, 0.419608f, 0.184314f },
    { "DarkOliveGreen",         0.333333f, 0.419608f, 0.184314f },
    { "dark_sea_green",         0.560784f, 0.737255f, 0.560784f },
    { "DarkSeaGreen",           0.560784f, 0.737255f, 0.560784f },
    { "sea_green",              0.180392f, 0.545098f, 0.341176f },
    { "SeaGreen",               0.180392f, 0.545098f, 0.341176f },
    { "medium_sea_green",       0.235294f, 0.701961f, 0.443137f },
    { "MediumSeaGreen",         0.235294f, 0.701961f, 0.443137f },
    { "light_sea_green",        0.125490f, 0.698039f, 0.666667f },
    { "LightSeaGreen",          0.125490f, 0.698039f, 0.666667f },
    { "pale_green",             0.596078f, 0.984314f, 0.596078f },
    { "PaleGreen",              0.596078f, 0.984314f, 0.596078f },
    { "spring_green",           0.000000f, 1.000000f, 0.498039f },
    { "SpringGreen",            0.000000f, 1.000000f, 0.498039f },
    { "lawn_green",             0.486275f, 0.988235f, 0.000000f },
    { "LawnGreen",              0.486275f, 0.988235f, 0.000000f },
    { "green",                  0.000000f, 1.000000f, 0.000000f },
    { "chartreuse",             0.498039f, 1.000000f, 0.000000f },
    { "medium_spring_green",    0.000000f, 0.980392f, 0.603922f },
    { "MediumSpringGreen",      0.000000f, 0.980392f, 0.603922f },
    { "green_yellow",           0.678431f, 1.000000f, 0.184314f },
    { "GreenYellow",            0.678431f, 1.000000f, 0.184314f },
    { "lime_green",             0.196078f, 0.803922f, 0.196078f },
    { "LimeGreen",              0.196078f, 0.803922f, 0.196078f },
    { "yellow_green",           0.603922f, 0.803922f, 0.196078f },
    { "YellowGreen",            0.603922f, 0.803922f, 0.196078f },
    { "forest_green",           0.133333f, 0.545098f, 0.133333f },
    { "ForestGreen",            0.133333f, 0.545098f, 0.133333f },
    { "olive_drab",             0.419608f, 0.556863f, 0.137255f },
    { "OliveDrab",              0.419608f, 0.556863f, 0.137255f },
    { "dark_khaki",             0.741176f, 0.717647f, 0.419608f },
    { "DarkKhaki",              0.741176f, 0.717647f, 0.419608f },
    { "khaki",                  0.941176f, 0.901961f, 0.549020f },
    { "pale_goldenrod",         0.933333f, 0.909804f, 0.666667f },
    { "PaleGoldenrod",          0.933333f, 0.909804f, 0.666667f },
    { "light_goldenrod_yellow", 0.980392f, 0.980392f, 0.823529f },
    { "LightGoldenrodYellow",   0.980392f, 0.980392f, 0.823529f },
    { "light_yellow",           1.000000f, 1.000000f, 0.878431f },
    { "LightYellow",            1.000000f, 1.000000f, 0.878431f },
    { "yellow",                 1.000000f, 1.000000f, 0.000000f },
    { "gold",                   1.000000f, 0.843137f, 0.000000f },
    { "light_goldenrod",        0.933333f, 0.866667f, 0.509804f },
    { "LightGoldenrod",         0.933333f, 0.866667f, 0.509804f },
    { "goldenrod",              0.854902f, 0.647059f, 0.125490f },
    { "dark_goldenrod",         0.721569f, 0.525490f, 0.043137f },
    { "DarkGoldenrod",          0.721569f, 0.525490f, 0.043137f },
    { "rosy_brown",             0.737255f, 0.560784f, 0.560784f },
    { "RosyBrown",              0.737255f, 0.560784f, 0.560784f },
    { "indian_red",             0.803922f, 0.360784f, 0.360784f },
    { "IndianRed",              0.803922f, 0.360784f, 0.360784f },
    { "saddle_brown",           0.545098f, 0.270588f, 0.074510f },
    { "SaddleBrown",            0.545098f, 0.270588f, 0.074510f },
    { "sienna",                 0.627451f, 0.321569f, 0.176471f },
    { "peru",                   0.803922f, 0.521569f, 0.247059f },
    { "burlywood",              0.870588f, 0.721569f, 0.529412f },
    { "beige",                  0.960784f, 0.960784f, 0.862745f },
    { "wheat",                  0.960784f, 0.870588f, 0.701961f },
    { "sandy_brown",            0.956863f, 0.643137f, 0.376471f },
    { "SandyBrown",             0.956863f, 0.643137f, 0.376471f },
    { "tan",                    0.823529f, 0.705882f, 0.549020f },
    { "chocolate",              0.823529f, 0.411765f, 0.117647f },
    { "firebrick",              0.698039f, 0.133333f, 0.133333f },
    { "brown",                  0.647059f, 0.164706f, 0.164706f },
    { "dark_salmon",            0.913725f, 0.588235f, 0.478431f },
    { "DarkSalmon",             0.913725f, 0.588235f, 0.478431f },
    { "salmon",                 0.980392f, 0.501961f, 0.447059f },
    { "light_salmon",           1.000000f, 0.627451f, 0.478431f },
    { "LightSalmon",            1.000000f, 0.627451f, 0.478431f },
    { "orange",                 1.000000f, 0.647059f, 0.000000f },
    { "dark_orange",            1.000000f, 0.549020f, 0.000000f },
    { "DarkOrange",             1.000000f, 0.549020f, 0.000000f },
    { "coral",                  1.000000f, 0.498039f, 0.313726f },
    { "light_coral",            0.941176f, 0.501961f, 0.501961f },
    { "LightCoral",             0.941176f, 0.501961f, 0.501961f },
    { "tomato",                 1.000000f, 0.388235f, 0.278431f },
    { "orange_red",             1.000000f, 0.270588f, 0.000000f },
    { "OrangeRed",              1.000000f, 0.270588f, 0.000000f },
    { "red",                    1.000000f, 0.000000f, 0.000000f },
    { "hot_pink",               1.000000f, 0.411765f, 0.705882f },
    { "HotPink",                1.000000f, 0.411765f, 0.705882f },
    { "deep_pink",              1.000000f, 0.078431f, 0.576471f },
    { "DeepPink",               1.000000f, 0.078431f, 0.576471f },
    { "pink",                   1.000000f, 0.752941f, 0.796078f },
    { "light_pink",             1.000000f, 0.713726f, 0.756863f },
    { "LightPink",              1.000000f, 0.713726f, 0.756863f },
    { "pale_violet_red",        0.858824f, 0.439216f, 0.576471f },
    { "PaleVioletRed",          0.858824f, 0.439216f, 0.576471f },
    { "maroon",                 0.690196f, 0.188235f, 0.376471f },
    { "medium_violet_red",      0.780392f, 0.082353f, 0.521569f },
    { "MediumVioletRed",        0.780392f, 0.082353f, 0.521569f },
    { "violet_red",             0.815686f, 0.125490f, 0.564706f },
    { "VioletRed",              0.815686f, 0.125490f, 0.564706f },
    { "magenta",                1.000000f, 0.000000f, 1.000000f },
    { "violet",                 0.933333f, 0.509804f, 0.933333f },
    { "plum",                   0.866667f, 0.627451f, 0.866667f },
    { "orchid",                 0.854902f, 0.439216f, 0.839216f },
    { "medium_orchid",          0.729412f, 0.333333f, 0.827451f },
    { "MediumOrchid",           0.729412f, 0.333333f, 0.827451f },
    { "dark_orchid",            0.600000f, 0.196078f, 0.800000f },
    { "DarkOrchid",             0.600000f, 0.196078f, 0.800000f },
    { "dark_violet",            0.580392f, 0.000000f, 0.827451f },
    { "DarkViolet",             0.580392f, 0.000000f, 0.827451f },
    { "blue_violet",            0.541176f, 0.168627f, 0.886275f },
    { "BlueViolet",             0.541176f, 0.168627f, 0.886275f },
    { "purple",                 0.627451f, 0.125490f, 0.941176f },
    { "medium_purple",          0.576471f, 0.439216f, 0.858824f },
    { "MediumPurple",           0.576471f, 0.439216f, 0.858824f },
    { "thistle",                0.847059f, 0.749020f, 0.847059f },
    { "snow1",                  1.000000f, 0.980392f, 0.980392f },
    { "snow2",                  0.933333f, 0.913725f, 0.913725f },
    { "snow3",                  0.803922f, 0.788235f, 0.788235f },
    { "snow4",                  0.545098f, 0.537255f, 0.537255f },
    { "seashell1",              1.000000f, 0.960784f, 0.933333f },
    { "seashell2",              0.933333f, 0.898039f, 0.870588f },
    { "seashell3",              0.803922f, 0.772549f, 0.749020f },
    { "seashell4",              0.545098f, 0.525490f, 0.509804f },
    { "AntiqueWhite1",          1.000000f, 0.937255f, 0.858824f },
    { "AntiqueWhite2",          0.933333f, 0.874510f, 0.800000f },
    { "AntiqueWhite3",          0.803922f, 0.752941f, 0.690196f },
    { "AntiqueWhite4",          0.545098f, 0.513726f, 0.470588f },
    { "bisque1",                1.000000f, 0.894118f, 0.768627f },
    { "bisque2",                0.933333f, 0.835294f, 0.717647f },
    { "bisque3",                0.803922f, 0.717647f, 0.619608f },
    { "bisque4",                0.545098f, 0.490196f, 0.419608f },
    { "PeachPuff1",             1.000000f, 0.854902f, 0.725490f },
    { "PeachPuff2",             0.933333f, 0.796078f, 0.678431f },
    { "PeachPuff3",             0.803922f, 0.686275f, 0.584314f },
    { "PeachPuff4",             0.545098f, 0.466667f, 0.396078f },
    { "NavajoWhite1",           1.000000f, 0.870588f, 0.678431f },
    { "NavajoWhite2",           0.933333f, 0.811765f, 0.631373f },
    { "NavajoWhite3",           0.803922f, 0.701961f, 0.545098f },
    { "NavajoWhite4",           0.545098f, 0.474510f, 0.368627f },
    { "LemonChiffon1",          1.000000f, 0.980392f, 0.803922f },
    { "LemonChiffon2",          0.933333f, 0.913725f, 0.749020f },
    { "LemonChiffon3",          0.803922f, 0.788235f, 0.647059f },
    { "LemonChiffon4",          0.545098f, 0.537255f, 0.439216f },
    { "cornsilk1",              1.000000f, 0.972549f, 0.862745f },
    { "cornsilk2",              0.933333f, 0.909804f, 0.803922f },
    { "cornsilk3",              0.803922f, 0.784314f, 0.694118f },
    { "cornsilk4",              0.545098f, 0.533333f, 0.470588f },
    { "ivory1",                 1.000000f, 1.000000f, 0.941176f },
    { "ivory2",                 0.933333f, 0.933333f, 0.878431f },
    { "ivory3",                 0.803922f, 0.803922f, 0.756863f },
    { "ivory4",                 0.545098f, 0.545098f, 0.513726f },
    { "honeydew1",              0.941176f, 1.000000f, 0.941176f },
    { "honeydew2",              0.878431f, 0.933333f, 0.878431f },
    { "honeydew3",              0.756863f, 0.803922f, 0.756863f },
    { "honeydew4",              0.513726f, 0.545098f, 0.513726f },
    { "LavenderBlush1",         1.000000f, 0.941176f, 0.960784f },
    { "LavenderBlush2",         0.933333f, 0.878431f, 0.898039f },
    { "LavenderBlush3",         0.803922f, 0.756863f, 0.772549f },
    { "LavenderBlush4",         0.545098f, 0.513726f, 0.525490f },
    { "MistyRose1",             1.000000f, 0.894118f, 0.882353f },
    { "MistyRose2",             0.933333f, 0.835294f, 0.823529f },
    { "MistyRose3",             0.803922f, 0.717647f, 0.709804f },
    { "MistyRose4",             0.545098f, 0.490196f, 0.482353f },
    { "azure1",                 0.941176f, 1.000000f, 1.000000f },
    { "azure2",                 0.878431f, 0.933333f, 0.933333f },
    { "azure3",                 0.756863f, 0.803922f, 0.803922f },
    { "azure4",                 0.513726f, 0.545098f, 0.545098f },
    { "SlateBlue1",             0.513726f, 0.435294f, 1.000000f },
    { "SlateBlue2",             0.478431f, 0.403922f, 0.933333f },
    { "SlateBlue3",             0.411765f, 0.349020f, 0.803922f },
    { "SlateBlue4",             0.278431f, 0.235294f, 0.545098f },
    { "RoyalBlue1",             0.282353f, 0.462745f, 1.000000f },
    { "RoyalBlue2",             0.262745f, 0.431373f, 0.933333f },
    { "RoyalBlue3",             0.227451f, 0.372549f, 0.803922f },
    { "RoyalBlue4",             0.152941f, 0.250980f, 0.545098f },
    { "blue1",                  0.000000f, 0.000000f, 1.000000f },
    { "blue2",                  0.000000f, 0.000000f, 0.933333f },
    { "blue3",                  0.000000f, 0.000000f, 0.803922f },
    { "blue4",                  0.000000f, 0.000000f, 0.545098f },
    { "DodgerBlue1",            0.117647f, 0.564706f, 1.000000f },
    { "DodgerBlue2",            0.109804f, 0.525490f, 0.933333f },
    { "DodgerBlue3",            0.094118f, 0.454902f, 0.803922f },
    { "DodgerBlue4",            0.062745f, 0.305882f, 0.545098f },
    { "SteelBlue1",             0.388235f, 0.721569f, 1.000000f },
    { "SteelBlue2",             0.360784f, 0.674510f, 0.933333f },
    { "SteelBlue3",             0.309804f, 0.580392f, 0.803922f },
    { "SteelBlue4",             0.211765f, 0.392157f, 0.545098f },
    { "DeepSkyBlue1",           0.000000f, 0.749020f, 1.000000f },
    { "DeepSkyBlue2",           0.000000f, 0.698039f, 0.933333f },
    { "DeepSkyBlue3",           0.000000f, 0.603922f, 0.803922f },
    { "DeepSkyBlue4",           0.000000f, 0.407843f, 0.545098f },
    { "SkyBlue1",               0.529412f, 0.807843f, 1.000000f },
    { "SkyBlue2",               0.494118f, 0.752941f, 0.933333f },
    { "SkyBlue3",               0.423529f, 0.650980f, 0.803922f },
    { "SkyBlue4",               0.290196f, 0.439216f, 0.545098f },
    { "LightSkyBlue1",          0.690196f, 0.886275f, 1.000000f },
    { "LightSkyBlue2",          0.643137f, 0.827451f, 0.933333f },
    { "LightSkyBlue3",          0.552941f, 0.713726f, 0.803922f },
    { "LightSkyBlue4",          0.376471f, 0.482353f, 0.545098f },
    { "SlateGray1",             0.776471f, 0.886275f, 1.000000f },
    { "SlateGray2",             0.725490f, 0.827451f, 0.933333f },
    { "SlateGray3",             0.623529f, 0.713726f, 0.803922f },
    { "SlateGray4",             0.423529f, 0.482353f, 0.545098f },
    { "LightSteelBlue1",        0.792157f, 0.882353f, 1.000000f },
    { "LightSteelBlue2",        0.737255f, 0.823529f, 0.933333f },
    { "LightSteelBlue3",        0.635294f, 0.709804f, 0.803922f },
    { "LightSteelBlue4",        0.431373f, 0.482353f, 0.545098f },
    { "LightBlue1",             0.749020f, 0.937255f, 1.000000f },
    { "LightBlue2",             0.698039f, 0.874510f, 0.933333f },
    { "LightBlue3",             0.603922f, 0.752941f, 0.803922f },
    { "LightBlue4",             0.407843f, 0.513726f, 0.545098f },
    { "LightCyan1",             0.878431f, 1.000000f, 1.000000f },
    { "LightCyan2",             0.819608f, 0.933333f, 0.933333f },
    { "LightCyan3",             0.705882f, 0.803922f, 0.803922f },
    { "LightCyan4",             0.478431f, 0.545098f, 0.545098f },
    { "PaleTurquoise1",         0.733333f, 1.000000f, 1.000000f },
    { "PaleTurquoise2",         0.682353f, 0.933333f, 0.933333f },
    { "PaleTurquoise3",         0.588235f, 0.803922f, 0.803922f },
    { "PaleTurquoise4",         0.400000f, 0.545098f, 0.545098f },
    { "CadetBlue1",             0.596078f, 0.960784f, 1.000000f },
    { "CadetBlue2",             0.556863f, 0.898039f, 0.933333f },
    { "CadetBlue3",             0.478431f, 0.772549f, 0.803922f },
    { "CadetBlue4",             0.325490f, 0.525490f, 0.545098f },
    { "turquoise1",             0.000000f, 0.960784f, 1.000000f },
    { "turquoise2",             0.000000f, 0.898039f, 0.933333f },
    { "turquoise3",             0.000000f, 0.772549f, 0.803922f },
    { "turquoise4",             0.000000f, 0.525490f, 0.545098f },
    { "cyan1",                  0.000000f, 1.000000f, 1.000000f },
    { "cyan2",                  0.000000f, 0.933333f, 0.933333f },
    { "cyan3",                  0.000000f, 0.803922f, 0.803922f },
    { "cyan4",                  0.000000f, 0.545098f, 0.545098f },
    { "DarkSlateGray1",         0.592157f, 1.000000f, 1.000000f },
    { "DarkSlateGray2",         0.552941f, 0.933333f, 0.933333f },
    { "DarkSlateGray3",         0.474510f, 0.803922f, 0.803922f },
    { "DarkSlateGray4",         0.321569f, 0.545098f, 0.545098f },
    { "aquamarine1",            0.498039f, 1.000000f, 0.831373f },
    { "aquamarine2",            0.462745f, 0.933333f, 0.776471f },
    { "aquamarine3",            0.400000f, 0.803922f, 0.666667f },
    { "aquamarine4",            0.270588f, 0.545098f, 0.454902f },
    { "DarkSeaGreen1",          0.756863f, 1.000000f, 0.756863f },
    { "DarkSeaGreen2",          0.705882f, 0.933333f, 0.705882f },
    { "DarkSeaGreen3",          0.607843f, 0.803922f, 0.607843f },
    { "DarkSeaGreen4",          0.411765f, 0.545098f, 0.411765f },
    { "SeaGreen1",              0.329412f, 1.000000f, 0.623529f },
    { "SeaGreen2",              0.305882f, 0.933333f, 0.580392f },
    { "SeaGreen3",              0.262745f, 0.803922f, 0.501961f },
    { "SeaGreen4",              0.180392f, 0.545098f, 0.341176f },
    { "PaleGreen1",             0.603922f, 1.000000f, 0.603922f },
    { "PaleGreen2",             0.564706f, 0.933333f, 0.564706f },
    { "PaleGreen3",             0.486275f, 0.803922f, 0.486275f },
    { "PaleGreen4",             0.329412f, 0.545098f, 0.329412f },
    { "SpringGreen1",           0.000000f, 1.000000f, 0.498039f },
    { "SpringGreen2",           0.000000f, 0.933333f, 0.462745f },
    { "SpringGreen3",           0.000000f, 0.803922f, 0.400000f },
    { "SpringGreen4",           0.000000f, 0.545098f, 0.270588f },
    { "green1",                 0.000000f, 1.000000f, 0.000000f },
    { "green2",                 0.000000f, 0.933333f, 0.000000f },
    { "green3",                 0.000000f, 0.803922f, 0.000000f },
    { "green4",                 0.000000f, 0.545098f, 0.000000f },
    { "chartreuse1",            0.498039f, 1.000000f, 0.000000f },
    { "chartreuse2",            0.462745f, 0.933333f, 0.000000f },
    { "chartreuse3",            0.400000f, 0.803922f, 0.000000f },
    { "chartreuse4",            0.270588f, 0.545098f, 0.000000f },
    { "OliveDrab1",             0.752941f, 1.000000f, 0.243137f },
    { "OliveDrab2",             0.701961f, 0.933333f, 0.227451f },
    { "OliveDrab3",             0.603922f, 0.803922f, 0.196078f },
    { "OliveDrab4",             0.411765f, 0.545098f, 0.133333f },
    { "DarkOliveGreen1",        0.792157f, 1.000000f, 0.439216f },
    { "DarkOliveGreen2",        0.737255f, 0.933333f, 0.407843f },
    { "DarkOliveGreen3",        0.635294f, 0.803922f, 0.352941f },
    { "DarkOliveGreen4",        0.431373f, 0.545098f, 0.239216f },
    { "khaki1",                 1.000000f, 0.964706f, 0.560784f },
    { "khaki2",                 0.933333f, 0.901961f, 0.521569f },
    { "khaki3",                 0.803922f, 0.776471f, 0.450980f },
    { "khaki4",                 0.545098f, 0.525490f, 0.305882f },
    { "LightGoldenrod1",        1.000000f, 0.925490f, 0.545098f },
    { "LightGoldenrod2",        0.933333f, 0.862745f, 0.509804f },
    { "LightGoldenrod3",        0.803922f, 0.745098f, 0.439216f },
    { "LightGoldenrod4",        0.545098f, 0.505882f, 0.298039f },
    { "LightYellow1",           1.000000f, 1.000000f, 0.878431f },
    { "LightYellow2",           0.933333f, 0.933333f, 0.819608f },
    { "LightYellow3",           0.803922f, 0.803922f, 0.705882f },
    { "LightYellow4",           0.545098f, 0.545098f, 0.478431f },
    { "yellow1",                1.000000f, 1.000000f, 0.000000f },
    { "yellow2",                0.933333f, 0.933333f, 0.000000f },
    { "yellow3",                0.803922f, 0.803922f, 0.000000f },
    { "yellow4",                0.545098f, 0.545098f, 0.000000f },
    { "gold1",                  1.000000f, 0.843137f, 0.000000f },
    { "gold2",                  0.933333f, 0.788235f, 0.000000f },
    { "gold3",                  0.803922f, 0.678431f, 0.000000f },
    { "gold4",                  0.545098f, 0.458824f, 0.000000f },
    { "goldenrod1",             1.000000f, 0.756863f, 0.145098f },
    { "goldenrod2",             0.933333f, 0.705882f, 0.133333f },
    { "goldenrod3",             0.803922f, 0.607843f, 0.113725f },
    { "goldenrod4",             0.545098f, 0.411765f, 0.078431f },
    { "DarkGoldenrod1",         1.000000f, 0.725490f, 0.058824f },
    { "DarkGoldenrod2",         0.933333f, 0.678431f, 0.054902f },
    { "DarkGoldenrod3",         0.803922f, 0.584314f, 0.047059f },
    { "DarkGoldenrod4",         0.545098f, 0.396078f, 0.031373f },
    { "RosyBrown1",             1.000000f, 0.756863f, 0.756863f },
    { "RosyBrown2",             0.933333f, 0.705882f, 0.705882f },
    { "RosyBrown3",             0.803922f, 0.607843f, 0.607843f },
    { "RosyBrown4",             0.545098f, 0.411765f, 0.411765f },
    { "IndianRed1",             1.000000f, 0.415686f, 0.415686f },
    { "IndianRed2",             0.933333f, 0.388235f, 0.388235f },
    { "IndianRed3",             0.803922f, 0.333333f, 0.333333f },
    { "IndianRed4",             0.545098f, 0.227451f, 0.227451f },
    { "sienna1",                1.000000f, 0.509804f, 0.278431f },
    { "sienna2",                0.933333f, 0.474510f, 0.258824f },
    { "sienna3",                0.803922f, 0.407843f, 0.223529f },
    { "sienna4",                0.545098f, 0.278431f, 0.149020f },
    { "burlywood1",             1.000000f, 0.827451f, 0.607843f },
    { "burlywood2",             0.933333f, 0.772549f, 0.568627f },
    { "burlywood3",             0.803922f, 0.666667f, 0.490196f },
    { "burlywood4",             0.545098f, 0.450980f, 0.333333f },
    { "wheat1",                 1.000000f, 0.905882f, 0.729412f },
    { "wheat2",                 0.933333f, 0.847059f, 0.682353f },
    { "wheat3",                 0.803922f, 0.729412f, 0.588235f },
    { "wheat4",                 0.545098f, 0.494118f, 0.400000f },
    { "tan1",                   1.000000f, 0.647059f, 0.309804f },
    { "tan2",                   0.933333f, 0.603922f, 0.286275f },
    { "tan3",                   0.803922f, 0.521569f, 0.247059f },
    { "tan4",                   0.545098f, 0.352941f, 0.168627f },
    { "chocolate1",             1.000000f, 0.498039f, 0.141176f },
    { "chocolate2",             0.933333f, 0.462745f, 0.129412f },
    { "chocolate3",             0.803922f, 0.400000f, 0.113725f },
    { "chocolate4",             0.545098f, 0.270588f, 0.074510f },
    { "firebrick1",             1.000000f, 0.188235f, 0.188235f },
    { "firebrick2",             0.933333f, 0.172549f, 0.172549f },
    { "firebrick3",             0.803922f, 0.149020f, 0.149020f },
    { "firebrick4",             0.545098f, 0.101961f, 0.101961f },
    { "brown1",                 1.000000f, 0.250980f, 0.250980f },
    { "brown2",                 0.933333f, 0.231373f, 0.231373f },
    { "brown3",                 0.803922f, 0.200000f, 0.200000f },
    { "brown4",                 0.545098f, 0.137255f, 0.137255f },
    { "salmon1",                1.000000f, 0.549020f, 0.411765f },
    { "salmon2",                0.933333f, 0.509804f, 0.384314f },
    { "salmon3",                0.803922f, 0.439216f, 0.329412f },
    { "salmon4",                0.545098f, 0.298039f, 0.223529f },
    { "LightSalmon1",           1.000000f, 0.627451f, 0.478431f },
    { "LightSalmon2",           0.933333f, 0.584314f, 0.447059f },
    { "LightSalmon3",           0.803922f, 0.505882f, 0.384314f },
    { "LightSalmon4",           0.545098f, 0.341176f, 0.258824f },
    { "orange1",                1.000000f, 0.647059f, 0.000000f },
    { "orange2",                0.933333f, 0.603922f, 0.000000f },
    { "orange3",                0.803922f, 0.521569f, 0.000000f },
    { "orange4",                0.545098f, 0.352941f, 0.000000f },
    { "DarkOrange1",            1.000000f, 0.498039f, 0.000000f },
    { "DarkOrange2",            0.933333f, 0.462745f, 0.000000f },
    { "DarkOrange3",            0.803922f, 0.400000f, 0.000000f },
    { "DarkOrange4",            0.545098f, 0.270588f, 0.000000f },
    { "coral1",                 1.000000f, 0.447059f, 0.337255f },
    { "coral2",                 0.933333f, 0.415686f, 0.313726f },
    { "coral3",                 0.803922f, 0.356863f, 0.270588f },
    { "coral4",                 0.545098f, 0.243137f, 0.184314f },
    { "tomato1",                1.000000f, 0.388235f, 0.278431f },
    { "tomato2",                0.933333f, 0.360784f, 0.258824f },
    { "tomato3",                0.803922f, 0.309804f, 0.223529f },
    { "tomato4",                0.545098f, 0.211765f, 0.149020f },
    { "OrangeRed1",             1.000000f, 0.270588f, 0.000000f },
    { "OrangeRed2",             0.933333f, 0.250980f, 0.000000f },
    { "OrangeRed3",             0.803922f, 0.215686f, 0.000000f },
    { "OrangeRed4",             0.545098f, 0.145098f, 0.000000f },
    { "red1",                   1.000000f, 0.000000f, 0.000000f },
    { "red2",                   0.933333f, 0.000000f, 0.000000f },
    { "red3",                   0.803922f, 0.000000f, 0.000000f },
    { "red4",                   0.545098f, 0.000000f, 0.000000f },
    { "DeepPink1",              1.000000f, 0.078431f, 0.576471f },
    { "DeepPink2",              0.933333f, 0.070588f, 0.537255f },
    { "DeepPink3",              0.803922f, 0.062745f, 0.462745f },
    { "DeepPink4",              0.545098f, 0.039216f, 0.313726f },
    { "HotPink1",               1.000000f, 0.431373f, 0.705882f },
    { "HotPink2",               0.933333f, 0.415686f, 0.654902f },
    { "HotPink3",               0.803922f, 0.376471f, 0.564706f },
    { "HotPink4",               0.545098f, 0.227451f, 0.384314f },
    { "pink1",                  1.000000f, 0.709804f, 0.772549f },
    { "pink2",                  0.933333f, 0.662745f, 0.721569f },
    { "pink3",                  0.803922f, 0.568627f, 0.619608f },
    { "pink4",                  0.545098f, 0.388235f, 0.423529f },
    { "LightPink1",             1.000000f, 0.682353f, 0.725490f },
    { "LightPink2",             0.933333f, 0.635294f, 0.678431f },
    { "LightPink3",             0.803922f, 0.549020f, 0.584314f },
    { "LightPink4",             0.545098f, 0.372549f, 0.396078f },
    { "PaleVioletRed1",         1.000000f, 0.509804f, 0.670588f },
    { "PaleVioletRed2",         0.933333f, 0.474510f, 0.623529f },
    { "PaleVioletRed3",         0.803922f, 0.407843f, 0.537255f },
    { "PaleVioletRed4",         0.545098f, 0.278431f, 0.364706f },
    { "maroon1",                1.000000f, 0.203922f, 0.701961f },
    { "maroon2",                0.933333f, 0.188235f, 0.654902f },
    { "maroon3",                0.803922f, 0.160784f, 0.564706f },
    { "maroon4",                0.545098f, 0.109804f, 0.384314f },
    { "VioletRed1",             1.000000f, 0.243137f, 0.588235f },
    { "VioletRed2",             0.933333f, 0.227451f, 0.549020f },
    { "VioletRed3",             0.803922f, 0.196078f, 0.470588f },
    { "VioletRed4",             0.545098f, 0.133333f, 0.321569f },
    { "magenta1",               1.000000f, 0.000000f, 1.000000f },
    { "magenta2",               0.933333f, 0.000000f, 0.933333f },
    { "magenta3",               0.803922f, 0.000000f, 0.803922f },
    { "magenta4",               0.545098f, 0.000000f, 0.545098f },
    { "orchid1",                1.000000f, 0.513726f, 0.980392f },
    { "orchid2",                0.933333f, 0.478431f, 0.913725f },
    { "orchid3",                0.803922f, 0.411765f, 0.788235f },
    { "orchid4",                0.545098f, 0.278431f, 0.537255f },
    { "plum1",                  1.000000f, 0.733333f, 1.000000f },
    { "plum2",                  0.933333f, 0.682353f, 0.933333f },
    { "plum3",                  0.803922f, 0.588235f, 0.803922f },
    { "plum4",                  0.545098f, 0.400000f, 0.545098f },
    { "MediumOrchid1",          0.878431f, 0.400000f, 1.000000f },
    { "MediumOrchid2",          0.819608f, 0.372549f, 0.933333f },
    { "MediumOrchid3",          0.705882f, 0.321569f, 0.803922f },
    { "MediumOrchid4",          0.478431f, 0.215686f, 0.545098f },
    { "DarkOrchid1",            0.749020f, 0.243137f, 1.000000f },
    { "DarkOrchid2",            0.698039f, 0.227451f, 0.933333f },
    { "DarkOrchid3",            0.603922f, 0.196078f, 0.803922f },
    { "DarkOrchid4",            0.407843f, 0.133333f, 0.545098f },
    { "purple1",                0.607843f, 0.188235f, 1.000000f },
    { "purple2",                0.568627f, 0.172549f, 0.933333f },
    { "purple3",                0.490196f, 0.149020f, 0.803922f },
    { "purple4",                0.333333f, 0.101961f, 0.545098f },
    { "MediumPurple1",          0.670588f, 0.509804f, 1.000000f },
    { "MediumPurple2",          0.623529f, 0.474510f, 0.933333f },
    { "MediumPurple3",          0.537255f, 0.407843f, 0.803922f },
    { "MediumPurple4",          0.364706f, 0.278431f, 0.545098f },
    { "thistle1",               1.000000f, 0.882353f, 1.000000f },
    { "thistle2",               0.933333f, 0.823529f, 0.933333f },
    { "thistle3",               0.803922f, 0.709804f, 0.803922f },
    { "thistle4",               0.545098f, 0.482353f, 0.545098f },
    { "gray0",                  0.000000f, 0.000000f, 0.000000f },
    { "grey0",                  0.000000f, 0.000000f, 0.000000f },
    { "gray1",                  0.011765f, 0.011765f, 0.011765f },
    { "grey1",                  0.011765f, 0.011765f, 0.011765f },
    { "gray2",                  0.019608f, 0.019608f, 0.019608f },
    { "grey2",                  0.019608f, 0.019608f, 0.019608f },
    { "gray3",                  0.031373f, 0.031373f, 0.031373f },
    { "grey3",                  0.031373f, 0.031373f, 0.031373f },
    { "gray4",                  0.039216f, 0.039216f, 0.039216f },
    { "grey4",                  0.039216f, 0.039216f, 0.039216f },
    { "gray5",                  0.050980f, 0.050980f, 0.050980f },
    { "grey5",                  0.050980f, 0.050980f, 0.050980f },
    { "gray6",                  0.058824f, 0.058824f, 0.058824f },
    { "grey6",                  0.058824f, 0.058824f, 0.058824f },
    { "gray7",                  0.070588f, 0.070588f, 0.070588f },
    { "grey7",                  0.070588f, 0.070588f, 0.070588f },
    { "gray8",                  0.078431f, 0.078431f, 0.078431f },
    { "grey8",                  0.078431f, 0.078431f, 0.078431f },
    { "gray9",                  0.090196f, 0.090196f, 0.090196f },
    { "grey9",                  0.090196f, 0.090196f, 0.090196f },
    { "gray10",                 0.101961f, 0.101961f, 0.101961f },
    { "grey10",                 0.101961f, 0.101961f, 0.101961f },
    { "gray11",                 0.109804f, 0.109804f, 0.109804f },
    { "grey11",                 0.109804f, 0.109804f, 0.109804f },
    { "gray12",                 0.121569f, 0.121569f, 0.121569f },
    { "grey12",                 0.121569f, 0.121569f, 0.121569f },
    { "gray13",                 0.129412f, 0.129412f, 0.129412f },
    { "grey13",                 0.129412f, 0.129412f, 0.129412f },
    { "gray14",                 0.141176f, 0.141176f, 0.141176f },
    { "grey14",                 0.141176f, 0.141176f, 0.141176f },
    { "gray15",                 0.149020f, 0.149020f, 0.149020f },
    { "grey15",                 0.149020f, 0.149020f, 0.149020f },
    { "gray16",                 0.160784f, 0.160784f, 0.160784f },
    { "grey16",                 0.160784f, 0.160784f, 0.160784f },
    { "gray17",                 0.168627f, 0.168627f, 0.168627f },
    { "grey17",                 0.168627f, 0.168627f, 0.168627f },
    { "gray18",                 0.180392f, 0.180392f, 0.180392f },
    { "grey18",                 0.180392f, 0.180392f, 0.180392f },
    { "gray19",                 0.188235f, 0.188235f, 0.188235f },
    { "grey19",                 0.188235f, 0.188235f, 0.188235f },
    { "gray20",                 0.200000f, 0.200000f, 0.200000f },
    { "grey20",                 0.200000f, 0.200000f, 0.200000f },
    { "gray21",                 0.211765f, 0.211765f, 0.211765f },
    { "grey21",                 0.211765f, 0.211765f, 0.211765f },
    { "gray22",                 0.219608f, 0.219608f, 0.219608f },
    { "grey22",                 0.219608f, 0.219608f, 0.219608f },
    { "gray23",                 0.231373f, 0.231373f, 0.231373f },
    { "grey23",                 0.231373f, 0.231373f, 0.231373f },
    { "gray24",                 0.239216f, 0.239216f, 0.239216f },
    { "grey24",                 0.239216f, 0.239216f, 0.239216f },
    { "gray25",                 0.250980f, 0.250980f, 0.250980f },
    { "grey25",                 0.250980f, 0.250980f, 0.250980f },
    { "gray26",                 0.258824f, 0.258824f, 0.258824f },
    { "grey26",                 0.258824f, 0.258824f, 0.258824f },
    { "gray27",                 0.270588f, 0.270588f, 0.270588f },
    { "grey27",                 0.270588f, 0.270588f, 0.270588f },
    { "gray28",                 0.278431f, 0.278431f, 0.278431f },
    { "grey28",                 0.278431f, 0.278431f, 0.278431f },
    { "gray29",                 0.290196f, 0.290196f, 0.290196f },
    { "grey29",                 0.290196f, 0.290196f, 0.290196f },
    { "gray30",                 0.301961f, 0.301961f, 0.301961f },
    { "grey30",                 0.301961f, 0.301961f, 0.301961f },
    { "gray31",                 0.309804f, 0.309804f, 0.309804f },
    { "grey31",                 0.309804f, 0.309804f, 0.309804f },
    { "gray32",                 0.321569f, 0.321569f, 0.321569f },
    { "grey32",                 0.321569f, 0.321569f, 0.321569f },
    { "gray33",                 0.329412f, 0.329412f, 0.329412f },
    { "grey33",                 0.329412f, 0.329412f, 0.329412f },
    { "gray34",                 0.341176f, 0.341176f, 0.341176f },
    { "grey34",                 0.341176f, 0.341176f, 0.341176f },
    { "gray35",                 0.349020f, 0.349020f, 0.349020f },
    { "grey35",                 0.349020f, 0.349020f, 0.349020f },
    { "gray36",                 0.360784f, 0.360784f, 0.360784f },
    { "grey36",                 0.360784f, 0.360784f, 0.360784f },
    { "gray37",                 0.368627f, 0.368627f, 0.368627f },
    { "grey37",                 0.368627f, 0.368627f, 0.368627f },
    { "gray38",                 0.380392f, 0.380392f, 0.380392f },
    { "grey38",                 0.380392f, 0.380392f, 0.380392f },
    { "gray39",                 0.388235f, 0.388235f, 0.388235f },
    { "grey39",                 0.388235f, 0.388235f, 0.388235f },
    { "gray40",                 0.400000f, 0.400000f, 0.400000f },
    { "grey40",                 0.400000f, 0.400000f, 0.400000f },
    { "gray41",                 0.411765f, 0.411765f, 0.411765f },
    { "grey41",                 0.411765f, 0.411765f, 0.411765f },
    { "gray42",                 0.419608f, 0.419608f, 0.419608f },
    { "grey42",                 0.419608f, 0.419608f, 0.419608f },
    { "gray43",                 0.431373f, 0.431373f, 0.431373f },
    { "grey43",                 0.431373f, 0.431373f, 0.431373f },
    { "gray44",                 0.439216f, 0.439216f, 0.439216f },
    { "grey44",                 0.439216f, 0.439216f, 0.439216f },
    { "gray45",                 0.450980f, 0.450980f, 0.450980f },
    { "grey45",                 0.450980f, 0.450980f, 0.450980f },
    { "gray46",                 0.458824f, 0.458824f, 0.458824f },
    { "grey46",                 0.458824f, 0.458824f, 0.458824f },
    { "gray47",                 0.470588f, 0.470588f, 0.470588f },
    { "grey47",                 0.470588f, 0.470588f, 0.470588f },
    { "gray48",                 0.478431f, 0.478431f, 0.478431f },
    { "grey48",                 0.478431f, 0.478431f, 0.478431f },
    { "gray49",                 0.490196f, 0.490196f, 0.490196f },
    { "grey49",                 0.490196f, 0.490196f, 0.490196f },
    { "gray50",                 0.498039f, 0.498039f, 0.498039f },
    { "grey50",                 0.498039f, 0.498039f, 0.498039f },
    { "gray51",                 0.509804f, 0.509804f, 0.509804f },
    { "grey51",                 0.509804f, 0.509804f, 0.509804f },
    { "gray52",                 0.521569f, 0.521569f, 0.521569f },
    { "grey52",                 0.521569f, 0.521569f, 0.521569f },
    { "gray53",                 0.529412f, 0.529412f, 0.529412f },
    { "grey53",                 0.529412f, 0.529412f, 0.529412f },
    { "gray54",                 0.541176f, 0.541176f, 0.541176f },
    { "grey54",                 0.541176f, 0.541176f, 0.541176f },
    { "gray55",                 0.549020f, 0.549020f, 0.549020f },
    { "grey55",                 0.549020f, 0.549020f, 0.549020f },
    { "gray56",                 0.560784f, 0.560784f, 0.560784f },
    { "grey56",                 0.560784f, 0.560784f, 0.560784f },
    { "gray57",                 0.568627f, 0.568627f, 0.568627f },
    { "grey57",                 0.568627f, 0.568627f, 0.568627f },
    { "gray58",                 0.580392f, 0.580392f, 0.580392f },
    { "grey58",                 0.580392f, 0.580392f, 0.580392f },
    { "gray59",                 0.588235f, 0.588235f, 0.588235f },
    { "grey59",                 0.588235f, 0.588235f, 0.588235f },
    { "gray60",                 0.600000f, 0.600000f, 0.600000f },
    { "grey60",                 0.600000f, 0.600000f, 0.600000f },
    { "gray61",                 0.611765f, 0.611765f, 0.611765f },
    { "grey61",                 0.611765f, 0.611765f, 0.611765f },
    { "gray62",                 0.619608f, 0.619608f, 0.619608f },
    { "grey62",                 0.619608f, 0.619608f, 0.619608f },
    { "gray63",                 0.631373f, 0.631373f, 0.631373f },
    { "grey63",                 0.631373f, 0.631373f, 0.631373f },
    { "gray64",                 0.639216f, 0.639216f, 0.639216f },
    { "grey64",                 0.639216f, 0.639216f, 0.639216f },
    { "gray65",                 0.650980f, 0.650980f, 0.650980f },
    { "grey65",                 0.650980f, 0.650980f, 0.650980f },
    { "gray66",                 0.658824f, 0.658824f, 0.658824f },
    { "grey66",                 0.658824f, 0.658824f, 0.658824f },
    { "gray67",                 0.670588f, 0.670588f, 0.670588f },
    { "grey67",                 0.670588f, 0.670588f, 0.670588f },
    { "gray68",                 0.678431f, 0.678431f, 0.678431f },
    { "grey68",                 0.678431f, 0.678431f, 0.678431f },
    { "gray69",                 0.690196f, 0.690196f, 0.690196f },
    { "grey69",                 0.690196f, 0.690196f, 0.690196f },
    { "gray70",                 0.701961f, 0.701961f, 0.701961f },
    { "grey70",                 0.701961f, 0.701961f, 0.701961f },
    { "gray71",                 0.709804f, 0.709804f, 0.709804f },
    { "grey71",                 0.709804f, 0.709804f, 0.709804f },
    { "gray72",                 0.721569f, 0.721569f, 0.721569f },
    { "grey72",                 0.721569f, 0.721569f, 0.721569f },
    { "gray73",                 0.729412f, 0.729412f, 0.729412f },
    { "grey73",                 0.729412f, 0.729412f, 0.729412f },
    { "gray74",                 0.741176f, 0.741176f, 0.741176f },
    { "grey74",                 0.741176f, 0.741176f, 0.741176f },
    { "gray75",                 0.749020f, 0.749020f, 0.749020f },
    { "grey75",                 0.749020f, 0.749020f, 0.749020f },
    { "gray76",                 0.760784f, 0.760784f, 0.760784f },
    { "grey76",                 0.760784f, 0.760784f, 0.760784f },
    { "gray77",                 0.768627f, 0.768627f, 0.768627f },
    { "grey77",                 0.768627f, 0.768627f, 0.768627f },
    { "gray78",                 0.780392f, 0.780392f, 0.780392f },
    { "grey78",                 0.780392f, 0.780392f, 0.780392f },
    { "gray79",                 0.788235f, 0.788235f, 0.788235f },
    { "grey79",                 0.788235f, 0.788235f, 0.788235f },
    { "gray80",                 0.800000f, 0.800000f, 0.800000f },
    { "grey80",                 0.800000f, 0.800000f, 0.800000f },
    { "gray81",                 0.811765f, 0.811765f, 0.811765f },
    { "grey81",                 0.811765f, 0.811765f, 0.811765f },
    { "gray82",                 0.819608f, 0.819608f, 0.819608f },
    { "grey82",                 0.819608f, 0.819608f, 0.819608f },
    { "gray83",                 0.831373f, 0.831373f, 0.831373f },
    { "grey83",                 0.831373f, 0.831373f, 0.831373f },
    { "gray84",                 0.839216f, 0.839216f, 0.839216f },
    { "grey84",                 0.839216f, 0.839216f, 0.839216f },
    { "gray85",                 0.850980f, 0.850980f, 0.850980f },
    { "grey85",                 0.850980f, 0.850980f, 0.850980f },
    { "gray86",                 0.858824f, 0.858824f, 0.858824f },
    { "grey86",                 0.858824f, 0.858824f, 0.858824f },
    { "gray87",                 0.870588f, 0.870588f, 0.870588f },
    { "grey87",                 0.870588f, 0.870588f, 0.870588f },
    { "gray88",                 0.878431f, 0.878431f, 0.878431f },
    { "grey88",                 0.878431f, 0.878431f, 0.878431f },
    { "gray89",                 0.890196f, 0.890196f, 0.890196f },
    { "grey89",                 0.890196f, 0.890196f, 0.890196f },
    { "gray90",                 0.898039f, 0.898039f, 0.898039f },
    { "grey90",                 0.898039f, 0.898039f, 0.898039f },
    { "gray91",                 0.909804f, 0.909804f, 0.909804f },
    { "grey91",                 0.909804f, 0.909804f, 0.909804f },
    { "gray92",                 0.921569f, 0.921569f, 0.921569f },
    { "grey92",                 0.921569f, 0.921569f, 0.921569f },
    { "gray93",                 0.929412f, 0.929412f, 0.929412f },
    { "grey93",                 0.929412f, 0.929412f, 0.929412f },
    { "gray94",                 0.941176f, 0.941176f, 0.941176f },
    { "grey94",                 0.941176f, 0.941176f, 0.941176f },
    { "gray95",                 0.949020f, 0.949020f, 0.949020f },
    { "grey95",                 0.949020f, 0.949020f, 0.949020f },
    { "gray96",                 0.960784f, 0.960784f, 0.960784f },
    { "grey96",                 0.960784f, 0.960784f, 0.960784f },
    { "gray97",                 0.968627f, 0.968627f, 0.968627f },
    { "grey97",                 0.968627f, 0.968627f, 0.968627f },
    { "gray98",                 0.980392f, 0.980392f, 0.980392f },
    { "grey98",                 0.980392f, 0.980392f, 0.980392f },
    { "gray99",                 0.988235f, 0.988235f, 0.988235f },
    { "grey99",                 0.988235f, 0.988235f, 0.988235f },
    { "gray100",                1.000000f, 1.000000f, 1.000000f },
    { "grey100",                1.000000f, 1.000000f, 1.000000f },
    { "dark_grey",              0.662745f, 0.662745f, 0.662745f },
    { "DarkGrey",               0.662745f, 0.662745f, 0.662745f },
    { "dark_gray",              0.662745f, 0.662745f, 0.662745f },
    { "DarkGray",               0.662745f, 0.662745f, 0.662745f },
    { "dark_blue",              0.000000f, 0.000000f, 0.545098f },
    { "DarkBlue",               0.000000f, 0.000000f, 0.545098f },
    { "dark_cyan",              0.000000f, 0.545098f, 0.545098f },
    { "DarkCyan",               0.000000f, 0.545098f, 0.545098f },
    { "dark_magenta",           0.545098f, 0.000000f, 0.545098f },
    { "DarkMagenta",            0.545098f, 0.000000f, 0.545098f },
    { "dark_red",               0.545098f, 0.000000f, 0.000000f },
    { "DarkRed",                0.545098f, 0.000000f, 0.000000f },
    { "light_green",            0.564706f, 0.933333f, 0.564706f },
    { "LightGreen",             0.564706f, 0.933333f, 0.564706f }
  };

  for (size_t i = 0; i < countof(colorArray); i++) {
    const ColorArrayData& data = colorArray[i];
    const std::string lower = TextUtils::tolower(data.name);
    colorMap[lower] = ColorMapData(data.r, data.g, data.b);
  }

  return colorMap;
}


//============================================================================//
//============================================================================//


// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
