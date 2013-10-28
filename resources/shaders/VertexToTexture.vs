#version 330
// This vertex shader is designed to allow a user to apply an effect based
// on a lookup texture.  The incoming vertex coordinates are interpreted in
// OpenGL viewport coordinates, and the assumption is that the lookup texture
// Will have been calculated for the full viewport dimensions.

// Therefore, all that's required for us to do is to conver the position
// coordinate from it's [-1,1] range to the texture convention of [0, 1]

// The incoming vertex coordinate in 2D viewport space
layout(location = 0) in vec3 Position;

// The outgoing texture coordinate in 2D texture space
out vec2 vTexCoord;

void main() {
    vTexCoord = (Position.xy / 2.0) + 0.5;
    gl_Position = vec4(Position.xy, 0, 1);
}
