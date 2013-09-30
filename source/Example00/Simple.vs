uniform mat4 Projection;
uniform mat4 ModelView;
attribute vec3 Position;
attribute vec3 TexCoord;

void main() {
   gl_Position = Projection * ModelView * vec4(Position, 1);
}
