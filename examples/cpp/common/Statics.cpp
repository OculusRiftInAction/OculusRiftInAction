/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ************************************************************************************/

#include "Common.h"


// Adapted (totally different from 'stolen') from OpenFrameworks
const glm::vec3 Colors::gray(1.0f / 2, 1.0f / 2, 1.0f / 2);
const glm::vec3 Colors::white(1.0f, 1.0f, 1.0f);
const glm::vec3 Colors::red(1.0f, 0, 0);
const glm::vec3 Colors::green(0, 1.0f, 0);
const glm::vec3 Colors::blue(0, 0, 1.0f);
const glm::vec3 Colors::cyan(0, 1.0f, 1.0f);
const glm::vec3 Colors::magenta(1.0f, 0, 1.0f);
const glm::vec3 Colors::yellow(1.0f, 1.0f, 0);
const glm::vec3 Colors::black(0, 0, 0);
const glm::vec3 Colors::aliceBlue(0.941176, 0.972549, 1);
const glm::vec3 Colors::antiqueWhite(0.980392, 0.921569, 0.843137);
const glm::vec3 Colors::aqua(0, 1, 1);
const glm::vec3 Colors::aquamarine(0.498039, 1, 0.831373);
const glm::vec3 Colors::azure(0.941176, 1, 1);
const glm::vec3 Colors::beige(0.960784, 0.960784, 0.862745);
const glm::vec3 Colors::bisque(1, 0.894118, 0.768627);
const glm::vec3 Colors::blanchedAlmond(1, 0.921569, 0.803922);
const glm::vec3 Colors::blueViolet(0.541176, 0.168627, 0.886275);
const glm::vec3 Colors::brown(0.647059, 0.164706, 0.164706);
const glm::vec3 Colors::burlyWood(0.870588, 0.721569, 0.529412);
const glm::vec3 Colors::cadetBlue(0.372549, 0.619608, 0.627451);
const glm::vec3 Colors::chartreuse(0.498039, 1, 0);
const glm::vec3 Colors::chocolate(0.823529, 0.411765, 0.117647);
const glm::vec3 Colors::coral(1, 0.498039, 0.313726);
const glm::vec3 Colors::cornflowerBlue(0.392157, 0.584314, 0.929412);
const glm::vec3 Colors::cornsilk(1, 0.972549, 0.862745);
const glm::vec3 Colors::crimson(0.862745, 0.0784314, 0.235294);
const glm::vec3 Colors::darkBlue(0, 0, 0.545098);
const glm::vec3 Colors::darkCyan(0, 0.545098, 0.545098);
const glm::vec3 Colors::darkGoldenRod(0.721569, 0.52549, 0.0431373);
const glm::vec3 Colors::darkGray(0.662745, 0.662745, 0.662745);
const glm::vec3 Colors::darkGrey(0.662745, 0.662745, 0.662745);
const glm::vec3 Colors::darkGreen(0, 0.392157, 0);
const glm::vec3 Colors::darkKhaki(0.741176, 0.717647, 0.419608);
const glm::vec3 Colors::darkMagenta(0.545098, 0, 0.545098);
const glm::vec3 Colors::darkOliveGreen(0.333333, 0.419608, 0.184314);
const glm::vec3 Colors::darkorange(1, 0.54902, 0);
const glm::vec3 Colors::darkOrchid(0.6, 0.196078, 0.8);
const glm::vec3 Colors::darkRed(0.545098, 0, 0);
const glm::vec3 Colors::darkSalmon(0.913725, 0.588235, 0.478431);
const glm::vec3 Colors::darkSeaGreen(0.560784, 0.737255, 0.560784);
const glm::vec3 Colors::darkSlateBlue(0.282353, 0.239216, 0.545098);
const glm::vec3 Colors::darkSlateGray(0.184314, 0.309804, 0.309804);
const glm::vec3 Colors::darkSlateGrey(0.184314, 0.309804, 0.309804);
const glm::vec3 Colors::darkTurquoise(0, 0.807843, 0.819608);
const glm::vec3 Colors::darkViolet(0.580392, 0, 0.827451);
const glm::vec3 Colors::deepPink(1, 0.0784314, 0.576471);
const glm::vec3 Colors::deepSkyBlue(0, 0.74902, 1);
const glm::vec3 Colors::dimGray(0.411765, 0.411765, 0.411765);
const glm::vec3 Colors::dimGrey(0.411765, 0.411765, 0.411765);
const glm::vec3 Colors::dodgerBlue(0.117647, 0.564706, 1);
const glm::vec3 Colors::fireBrick(0.698039, 0.133333, 0.133333);
const glm::vec3 Colors::floralWhite(1, 0.980392, 0.941176);
const glm::vec3 Colors::forestGreen(0.133333, 0.545098, 0.133333);
const glm::vec3 Colors::fuchsia(1, 0, 1);
const glm::vec3 Colors::gainsboro(0.862745, 0.862745, 0.862745);
const glm::vec3 Colors::ghostWhite(0.972549, 0.972549, 1);
const glm::vec3 Colors::gold(1, 0.843137, 0);
const glm::vec3 Colors::goldenRod(0.854902, 0.647059, 0.12549);
const glm::vec3 Colors::grey(0.501961, 0.501961, 0.501961);
const glm::vec3 Colors::greenYellow(0.678431, 1, 0.184314);
const glm::vec3 Colors::honeyDew(0.941176, 1, 0.941176);
const glm::vec3 Colors::hotPink(1, 0.411765, 0.705882);
const glm::vec3 Colors::indianRed(0.803922, 0.360784, 0.360784);
const glm::vec3 Colors::indigo(0.294118, 0, 0.509804);
const glm::vec3 Colors::ivory(1, 1, 0.941176);
const glm::vec3 Colors::khaki(0.941176, 0.901961, 0.54902);
const glm::vec3 Colors::lavender(0.901961, 0.901961, 0.980392);
const glm::vec3 Colors::lavenderBlush(1, 0.941176, 0.960784);
const glm::vec3 Colors::lawnGreen(0.486275, 0.988235, 0);
const glm::vec3 Colors::lemonChiffon(1, 0.980392, 0.803922);
const glm::vec3 Colors::lightBlue(0.678431, 0.847059, 0.901961);
const glm::vec3 Colors::lightCoral(0.941176, 0.501961, 0.501961);
const glm::vec3 Colors::lightCyan(0.878431, 1, 1);
const glm::vec3 Colors::lightGoldenRodYellow(0.980392, 0.980392, 0.823529);
const glm::vec3 Colors::lightGray(0.827451, 0.827451, 0.827451);
const glm::vec3 Colors::lightGrey(0.827451, 0.827451, 0.827451);
const glm::vec3 Colors::lightGreen(0.564706, 0.933333, 0.564706);
const glm::vec3 Colors::lightPink(1, 0.713726, 0.756863);
const glm::vec3 Colors::lightSalmon(1, 0.627451, 0.478431);
const glm::vec3 Colors::lightSeaGreen(0.12549, 0.698039, 0.666667);
const glm::vec3 Colors::lightSkyBlue(0.529412, 0.807843, 0.980392);
const glm::vec3 Colors::lightSlateGray(0.466667, 0.533333, 0.6);
const glm::vec3 Colors::lightSlateGrey(0.466667, 0.533333, 0.6);
const glm::vec3 Colors::lightSteelBlue(0.690196, 0.768627, 0.870588);
const glm::vec3 Colors::lightYellow(1, 1, 0.878431);
const glm::vec3 Colors::lime(0, 1, 0);
const glm::vec3 Colors::limeGreen(0.196078, 0.803922, 0.196078);
const glm::vec3 Colors::linen(0.980392, 0.941176, 0.901961);
const glm::vec3 Colors::maroon(0.501961, 0, 0);
const glm::vec3 Colors::mediumAquaMarine(0.4, 0.803922, 0.666667);
const glm::vec3 Colors::mediumBlue(0, 0, 0.803922);
const glm::vec3 Colors::mediumOrchid(0.729412, 0.333333, 0.827451);
const glm::vec3 Colors::mediumPurple(0.576471, 0.439216, 0.858824);
const glm::vec3 Colors::mediumSeaGreen(0.235294, 0.701961, 0.443137);
const glm::vec3 Colors::mediumSlateBlue(0.482353, 0.407843, 0.933333);
const glm::vec3 Colors::mediumSpringGreen(0, 0.980392, 0.603922);
const glm::vec3 Colors::mediumTurquoise(0.282353, 0.819608, 0.8);
const glm::vec3 Colors::mediumVioletRed(0.780392, 0.0823529, 0.521569);
const glm::vec3 Colors::midnightBlue(0.0980392, 0.0980392, 0.439216);
const glm::vec3 Colors::mintCream(0.960784, 1, 0.980392);
const glm::vec3 Colors::mistyRose(1, 0.894118, 0.882353);
const glm::vec3 Colors::moccasin(1, 0.894118, 0.709804);
const glm::vec3 Colors::navajoWhite(1, 0.870588, 0.678431);
const glm::vec3 Colors::navy(0, 0, 0.501961);
const glm::vec3 Colors::oldLace(0.992157, 0.960784, 0.901961);
const glm::vec3 Colors::olive(0.501961, 0.501961, 0);
const glm::vec3 Colors::oliveDrab(0.419608, 0.556863, 0.137255);
const glm::vec3 Colors::orange(1, 0.647059, 0);
const glm::vec3 Colors::orangeRed(1, 0.270588, 0);
const glm::vec3 Colors::orchid(0.854902, 0.439216, 0.839216);
const glm::vec3 Colors::paleGoldenRod(0.933333, 0.909804, 0.666667);
const glm::vec3 Colors::paleGreen(0.596078, 0.984314, 0.596078);
const glm::vec3 Colors::paleTurquoise(0.686275, 0.933333, 0.933333);
const glm::vec3 Colors::paleVioletRed(0.858824, 0.439216, 0.576471);
const glm::vec3 Colors::papayaWhip(1, 0.937255, 0.835294);
const glm::vec3 Colors::peachPuff(1, 0.854902, 0.72549);
const glm::vec3 Colors::peru(0.803922, 0.521569, 0.247059);
const glm::vec3 Colors::pink(1, 0.752941, 0.796078);
const glm::vec3 Colors::plum(0.866667, 0.627451, 0.866667);
const glm::vec3 Colors::powderBlue(0.690196, 0.878431, 0.901961);
const glm::vec3 Colors::purple(0.501961, 0, 0.501961);
const glm::vec3 Colors::rosyBrown(0.737255, 0.560784, 0.560784);
const glm::vec3 Colors::royalBlue(0.254902, 0.411765, 0.882353);
const glm::vec3 Colors::saddleBrown(0.545098, 0.270588, 0.0745098);
const glm::vec3 Colors::salmon(0.980392, 0.501961, 0.447059);
const glm::vec3 Colors::sandyBrown(0.956863, 0.643137, 0.376471);
const glm::vec3 Colors::seaGreen(0.180392, 0.545098, 0.341176);
const glm::vec3 Colors::seaShell(1, 0.960784, 0.933333);
const glm::vec3 Colors::sienna(0.627451, 0.321569, 0.176471);
const glm::vec3 Colors::silver(0.752941, 0.752941, 0.752941);
const glm::vec3 Colors::skyBlue(0.529412, 0.807843, 0.921569);
const glm::vec3 Colors::slateBlue(0.415686, 0.352941, 0.803922);
const glm::vec3 Colors::slateGray(0.439216, 0.501961, 0.564706);
const glm::vec3 Colors::slateGrey(0.439216, 0.501961, 0.564706);
const glm::vec3 Colors::snow(1, 0.980392, 0.980392);
const glm::vec3 Colors::springGreen(0, 1, 0.498039);
const glm::vec3 Colors::steelBlue(0.27451, 0.509804, 0.705882);
const glm::vec3 Colors::blueSteel(0.27451, 0.509804, 0.705882);
const glm::vec3 Colors::tan(0.823529, 0.705882, 0.54902);
const glm::vec3 Colors::teal(0, 0.501961, 0.501961);
const glm::vec3 Colors::thistle(0.847059, 0.74902, 0.847059);
const glm::vec3 Colors::tomato(1, 0.388235, 0.278431);
const glm::vec3 Colors::turquoise(0.25098, 0.878431, 0.815686);
const glm::vec3 Colors::violet(0.933333, 0.509804, 0.933333);
const glm::vec3 Colors::wheat(0.960784, 0.870588, 0.701961);
const glm::vec3 Colors::whiteSmoke(0.960784, 0.960784, 0.960784);
const glm::vec3 Colors::yellowGreen(0.603922, 0.803922, 0.196078);


const glm::vec3 Vectors::X_AXIS = glm::vec3(1.0f, 0.0f, 0.0f);
const glm::vec3 Vectors::Y_AXIS = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 Vectors::Z_AXIS = glm::vec3(0.0f, 0.0f, 1.0f);
const glm::vec3 Vectors::ZERO = glm::vec3(0);
const glm::vec3 Vectors::ONE = glm::vec3(1);
const glm::vec3 & Vectors::ORIGIN = Vectors::ZERO;
const glm::vec3 & Vectors::UP = Vectors::Y_AXIS;

