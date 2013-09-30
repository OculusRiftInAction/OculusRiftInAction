attribute vec2 Position;
attribute vec2 TexCoord;
varying vec2 oTexCoord;
void main()
{
   oTexCoord = TexCoord;
   gl_Position = vec4(Position, 0, 1);
}
