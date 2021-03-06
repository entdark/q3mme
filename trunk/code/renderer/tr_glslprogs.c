/*
 *      tr_glslprogs.c
 *      
 *      Copyright 2007 Gord Allott <gordallott@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include "tr_glslprogs.h"
//for all the glsl source its best if we keep them in the mainline code unless
//someone (not me!) wants to impliment them into the q3 mainline pak code
//and of course provide a 'standard library' for paks that don't have the glsl
//sources in them. this would allow mod authors to code their own effects but 
//meh, they might as well just hack them into the source and send the changes
//upstream.

//when implimenting glsl code try and stick to the carmark q3 coding style ie, 
//variables go myVariable and stuff. also end lines with \n\ instead of \ as it 
//makes debugging glsl code 15,823x easier. (looks nasty either way...)


//this vertex shader is basically complete, we shouldn't really need anything 
//else(?). it just maps the vertex position to a screen position. 
const char *glslBase_vert = "\n\
void main() {\n\
  gl_TexCoord[0] = gl_MultiTexCoord0;\n\
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
}\
";

const char *glslGauss9 = "\n\
#define NK9_0 0.17857142857142855\n\
#define NK9_1 0.1607142857142857\n\
#define NK9_2 0.14285714285714285\n\
#define NK9_3 0.071428571428571425\n\
#define NK9_4 0.035714285714285712\n\
\
vec4 GaussPass(sampler2D src, vec2 coord, vec2 blrsize) {\n\
  //blrsize is the size (in texture coordinates) of the blur kernel\n\
\n\
  vec4 accum;\n\
  vec2 step, pos;\n\
  step = blrsize / 9.0;\n\
  pos = coord - (step * 4.0);\n\
\n\
  accum  = texture2D(src, pos) * NK9_4; pos += step;\n\
  accum += texture2D(src, pos) * NK9_3; pos += step;\n\
  accum += texture2D(src, pos) * NK9_2; pos += step;\n\
  accum += texture2D(src, pos) * NK9_1; pos += step;\n\
  accum += texture2D(src, pos) * NK9_0; pos += step;\n\
  accum += texture2D(src, pos) * NK9_1; pos += step;\n\
  accum += texture2D(src, pos) * NK9_2; pos += step;\n\
  accum += texture2D(src, pos) * NK9_3; pos += step;\n\
  accum += texture2D(src, pos) * NK9_4; pos += step;\n\
\n\
  return accum;\n\
\n\
}\n\
";


const char *glslGauss7 = "\n\
#define NK7_0 0.19230769230769229\n\
#define NK7_1 0.18269230769230768\n\
#define NK7_2 0.15384615384615385\n\
#define NK7_3 0.067307692307692304\n\
\n\
vec4 GaussPass(sampler2D src, vec2 coord, vec2 blrsize) {\n\
  //blrsize is the size (in texture coordinates) of the blur kernel\n\
\n\
  vec4 accum;\n\
  vec2 step, pos;\n\
  step = blrsize / 7.0;\n\
  pos = coord - (step * 3.0);\n\
\n\
  accum  = texture2D(src, pos) * NK7_3; pos += step;\n\
  accum += texture2D(src, pos) * NK7_2; pos += step;\n\
  accum += texture2D(src, pos) * NK7_1; pos += step;\n\
  accum += texture2D(src, pos) * NK7_0; pos += step;\n\
  accum += texture2D(src, pos) * NK7_1; pos += step;\n\
  accum += texture2D(src, pos) * NK7_2; pos += step;\n\
  accum += texture2D(src, pos) * NK7_3; pos += step;\n\
  \n\
  return accum;\n\
\n\
}\n\
";

const char *glslGauss5 = "\n\
#define NK5_0 0.33333333\n\
#define NK5_1 0.26666666\n\
#define NK5_2 0.06666666\n\
\n\
vec4 GaussPass(sampler2D src, vec2 coord, vec2 blrsize) {\n\
  //blrsize is the size (in texture coordinates) of the blur kernel\n\
  \n\
  vec4 accum;\n\
  vec2 step, pos;\n\
  step = blrsize / 5.0;\n\
  pos = coord - (step * 2.0);\n\
  \n\
  accum  = texture2D(src, pos) * NK5_2; pos += step;\n\
  accum += texture2D(src, pos) * NK5_1; pos += step;\n\
  accum += texture2D(src, pos) * NK5_0; pos += step;\n\
  accum += texture2D(src, pos) * NK5_1; pos += step;\n\
  accum += texture2D(src, pos) * NK5_2; pos += step;\n\
  \n\
  return accum;\n\
  \n\
}\n\
";

const char *glslBlurMain = "\n\
uniform sampler2D srcSampler;\n\
uniform vec2 blurSize;\n\
void main()\n\
{\n\
  gl_FragColor = GaussPass(srcSampler, gl_TexCoord[0].xy, blurSize);\n\
}\n\
";

const char *glslSigScreen = "\n\
uniform sampler2D srcSampler;\n\
uniform sampler2D blurSampler;\n\
//#define sharpness 0.75 \n\
uniform float     sharpness;\n\
//#define brightness 0.85\n\
uniform float     brightness;\n\
#define SIGMOIDAL_BASE          2.0\n\
#define SIGMOIDAL_RANGE         20.0\n\
\n\
void main()\n\
{\n\
	\n\
  vec4 blurcolor 	= texture2D( blurSampler, gl_TexCoord[0].xy);\n\
  vec4 basecolor 	= texture2D( srcSampler, gl_TexCoord[0].xy);\n\
  \n\
  //vec4 val = 1.0 / (1.0 + exp (-(SIGMOIDAL_BASE + (sharpness * SIGMOIDAL_RANGE)) * (blurcolor - 0.5)));\n\
  vec4 val;\n\
  val = -(SIGMOIDAL_BASE + (sharpness * SIGMOIDAL_RANGE)) * (blurcolor - 0.5);\n\
  val = 1.0 + pow(vec4(2.718281828459045), val);\n\
  val = 1.0 / val;\n\
  val = val * brightness;\n\
  \n\
  gl_FragColor = 1.0 - ((1.0 - basecolor) * (1.0 - val));\n\
}\n\
";

const char *glslSobel = "\n\
float sobel(sampler2D tex, vec2 basecoord, vec2 texel_size) {\n\
  /* computes a sobel value from the surrounding pixels */\n\
  vec4 hori, vert;\n\
  //vec2 basecoord = coord;\n\
  float stepw, steph;\n\
  stepw = texel_size.x;\n\
  steph = texel_size.y;\n\
  \n\
  vert  = texture2D(tex, basecoord + vec2(-stepw, -steph)) * -1.0;\n\
  vert += texture2D(tex, basecoord + vec2(-stepw,  0.0  )) * -2.0;\n\
  vert += texture2D(tex, basecoord + vec2(-stepw, +steph)) * -1.0;\n\
  \n\
  vert += texture2D(tex, basecoord + vec2( stepw, -steph)) * 1.0;\n\
  vert += texture2D(tex, basecoord + vec2( stepw,  0.0  )) * 2.0;\n\
  vert += texture2D(tex, basecoord + vec2( stepw, +steph)) * 1.0;\n\
  \n\
  hori  = texture2D(tex, basecoord + vec2(-stepw, -steph)) * -1.0;\n\
  hori += texture2D(tex, basecoord + vec2( 0.0  , -steph)) * -2.0;\n\
  hori += texture2D(tex, basecoord + vec2(+stepw, -steph)) * -1.0;\n\
\n\
  hori += texture2D(tex, basecoord + vec2(-stepw,  steph)) * 1.0;\n\
  hori += texture2D(tex, basecoord + vec2( 0.0  ,  steph)) * 2.0;\n\
  hori += texture2D(tex, basecoord + vec2(+stepw,  steph)) * 1.0;\n\
\n\
  /* could use dist() but this is more compatible */\n\
  return sqrt(float((vert * vert) + (hori * hori)));\n\
  \n\
}\n\
";

const char *glslSobelZ = "\n\
float sobel(sampler2D tex, vec2 basecoord, vec2 texel_size) {\n\
  /* computes a sobel value from the surrounding pixels */\n\
  vec4 hori, vert;\n\
  //vec2 basecoord = coord;\n\
  float stepw, steph;\n\
  stepw = texel_size.x;\n\
  steph = texel_size.y;\n\
  \n\
  //when used with a zbuffer we first need to figure out how deep the texel is\n\
  float depth = texture2D(tex, basecoord).r;\n\
  depth = ((exp(depth * 5.0) /  2.7182818284590451) - 0.36787944117144233);\n\
  depth = depth * 1.5819767068693265;\n\
  \n\
  vert  = texture2D(tex, basecoord + vec2(-stepw, -steph)) * -1.0;\n\
  vert += texture2D(tex, basecoord + vec2(-stepw,  0.0  )) * -2.0;\n\
  vert += texture2D(tex, basecoord + vec2(-stepw, +steph)) * -1.0;\n\
  \n\
  vert += texture2D(tex, basecoord + vec2( stepw, -steph)) * 1.0;\n\
  vert += texture2D(tex, basecoord + vec2( stepw,  0.0  )) * 2.0;\n\
  vert += texture2D(tex, basecoord + vec2( stepw, +steph)) * 1.0;\n\
  \n\
  hori  = texture2D(tex, basecoord + vec2(-stepw, -steph)) * -1.0;\n\
  hori += texture2D(tex, basecoord + vec2( 0.0  , -steph)) * -2.0;\n\
  hori += texture2D(tex, basecoord + vec2(+stepw, -steph)) * -1.0;\n\
\n\
  hori += texture2D(tex, basecoord + vec2(-stepw,  steph)) * 1.0;\n\
  hori += texture2D(tex, basecoord + vec2( 0.0  ,  steph)) * 2.0;\n\
  hori += texture2D(tex, basecoord + vec2(+stepw,  steph)) * 1.0;\n\
\n\
  /* could use dist() but this is more compatible */\n\
  return sqrt(float((vert * vert) + (hori * hori))) * depth;\n\
  \n\
}\n\
";

const char *glslToonColour = "\n\
vec4 ToonColour(vec4 incolour) {\n\
\n\
  vec3 huetemp;\n\
  huetemp.x = 0.0;\n\
  huetemp.y = 0.0;\n\
  huetemp.z = 0.0;\n\
\n\
  huetemp.x = incolour.x + incolour.y + incolour.z;\n\
  huetemp.y = 1.0 / huetemp.x;\n\
  \n\
  /* multiply the pixel colourby 1 / sumrgb */\n\
  incolour = incolour * huetemp.y;\n\
  /* get the  tones */\n\
  \n\
  if (huetemp.x > 0.2) {\n\
    huetemp.z = 0.4;\n\
   \n\
  } else {\n\
    huetemp.z = 0.0;\n\
  }\n\
  \n\
  if (huetemp.x > 0.4) {\n\
    huetemp.y = 1.0;\n\
  } else {\n\
    huetemp.y = 0.0;\n\
  }\n\
  \n\
  if (huetemp.x > 1.0) {\n\
    huetemp.x = 1.5;\n\
  } else {\n\
    huetemp.x = 0.0;\n\
  }\n\
\n\
  \n\
  /* sum the huetones */\n\
  \n\
  huetemp.x = huetemp.x + huetemp.y + huetemp.z;\n\
  \n\
  /* multiply the pixel colour with the resulting intensity */\n\
  \n\
  incolour = incolour * huetemp.x;\n\
\n\
  return vec4(incolour);\n\
}\n\
";

const char *glslRotoscope = "\n\
uniform vec2 texelSize;\n\
uniform sampler2D srcSampler;\n\
void main()\n\
{\n\
\n\
  float fragsobel = sobel(srcSampler, gl_TexCoord[0].xy, texelSize);\n\
  vec4 final_color = ToonColour(texture2D(srcSampler, gl_TexCoord[0].xy));\n\
\n\
  fragsobel = 1.0 - clamp(fragsobel - 0.2, 0.0, 1.0);\n\
  gl_FragColor = final_color * fragsobel;\n\
\n\
}\n\
";

const char *glslRotoscopeZ = "\n\
uniform vec2 texelSize;\n\
uniform sampler2D srcSampler;\n\
uniform sampler2D depthSampler;\n\
\n\
void main()\n\
{\n\
\n\
  float fragsobel = sobel(depthSampler, gl_TexCoord[0].xy, texelSize);\n\
  vec4 final_color = ToonColour(texture2D(srcSampler, gl_TexCoord[0].xy));\n\
\n\
  fragsobel = clamp(fragsobel, 0.0, 1.0);//1.0 - clamp(fragsobel, 0.0, 1.0);\n\
  //gl_FragColor = vec4(texture2D(depthSampler, gl_TexCoord[0].xy).r) / 2.0;\n\
  //final_color = 1.0;\n\
  gl_FragColor = final_color * (1.0 - fragsobel);\n\
  //gl_FragColor = fragsobel;\n\
\n\
}\n\
";