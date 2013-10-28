#version 330
// This vertex shader is specifically designed to assist in working with
// fragment shaders intended to distort the incoming image for the
// Oculus Rift.

// This shader is suitable either for determining the coordinates for
// a real-time modification of the pixels for every frame rendered
// or, more likely, for pre-generating an offset lookup map that can be
// used by a much simpler shader to provide the distortion with less
// memory overhead
//
// Mechanism of operation
//
// The Rift distortion is based on the distance of the pixel
// from the intersection of the lens axis and the screen.  This
// position has a linear relationship with the vertex positions
// represented in viewport space, where the viewport is for only
// one eye.
//
// If we know the aspect of the viewport, and the offset of the
// lens axis intersection in terms of the viewport coordinates, we
// can calculate the correct value for each incoming vertex and
// allow linear interpolation to provide the correct per pixel
// coordinate to the fragment shader.

layout(location = 0) in vec3 Position;

// A default aspect value based on the 16:10 panel used in the
// DK1 Rift
uniform vec2 Aspect;

uniform bool Mirror;

// The lens center offset will allow us to translate the coordinates
// so that they are centered over the Rift.
// This can't have a default value since the X value will either be
// positive or negative depending on whether we're rendering for the
// right or left eye
uniform vec2 LensCenter;

// In addition to setting the gl_Position output, we also have a varying value
// which will convey the Rift oriented coordinates to the fragment shader.
out vec2 vRiftTexCoord;


void main() {
    vRiftTexCoord = (Position.xy / Aspect) -
            ( LensCenter * (Mirror  ? -1.0 : 1.0));
    gl_Position = vec4(Position.xy, 0, 1);
}
