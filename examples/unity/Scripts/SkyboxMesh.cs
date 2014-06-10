// coded by Nora
// http://stereoarts.jp
using UnityEngine;
using System.Collections;

public class SkyboxMesh : MonoBehaviour
{
	public enum Shape {
		Sphere,
		Cube,
	}
	
	private static readonly string _shaderText =
		"Shader \"SkyboxMesh Background\" {" +
			"Properties {" +
			"    _Color (\"Main Color\", Color) = (1,1,1,1)" +
			"    _MainTex (\"Texture\", 2D) = \"white\" {}" +
			"}" +
			"Category {" +
			"   Tags { \"Queue\"=\"Background\" }" +
			"    ZWrite Off" +
			"    Lighting Off" +
			"    Fog {Mode Off}" +
			"    BindChannels {" +
			"        Bind \"Color\", color" +
			"        Bind \"Vertex\", vertex" +
			"        Bind \"TexCoord\", texcoord" +
			"    }" +
			"    SubShader {" +
			"        Pass {" +
			"            SetTexture [_MainTex] {" +
			"                Combine texture" +
			"            }" +
			"            SetTexture [_MainTex] {" +
			"               constantColor [_Color]" +
			"            combine previous * constant" +
			"            }" +
			"        }" +
			"    }" +
			"}" +
			"}";
	
	public Material		material;
	public float		radius		= 800.0f;
	public int		 	segments	= 32;
	public Shape		shape		= Shape.Sphere;
	public Material		skybox;
	public GameObject	follow;
	
	void Awake()
	{
		if( this.material == null ) {
			this.material = new Material( _shaderText );
		}
		
		Mesh mesh = _CreateMesh();
		_CreatePlane( mesh, "_FrontTex", Quaternion.identity );
		_CreatePlane( mesh, "_LeftTex",  Quaternion.Euler( 0.0f, 90.0f, 0.0f ) );
		_CreatePlane( mesh, "_BackTex",  Quaternion.Euler( 0.0f, 180.0f, 0.0f ) );
		_CreatePlane( mesh, "_RightTex", Quaternion.Euler( 0.0f, 270.0f, 0.0f ) );
		_CreatePlane( mesh, "_UpTex",    Quaternion.Euler( -90.0f, 0.0f, 0.0f ) );
		_CreatePlane( mesh, "_DownTex",  Quaternion.Euler( 90.0f, 0.0f, 0.0f ) );
	}
	
	void LateUpdate()
	{
		if( this.follow != null ) {
			this.transform.position = this.follow.transform.position;
		}
	}
	
	Mesh _CreateMesh()
	{
		Mesh mesh = new Mesh();
		
		int hvCount2 = this.segments + 1;
		int hvCount2Half = hvCount2 / 2;
		int numVertices = hvCount2 * hvCount2;
		int numTriangles = this.segments * this.segments * 6;
		Vector3[] vertices = new Vector3[numVertices];
		Vector2[] uvs = new Vector2[numVertices];
		int[] triangles = new int[numTriangles];
		
		float scaleFactor = 2.0f / (float)this.segments;
		float uvFactor = 1.0f / (float)this.segments;
		
		if( this.segments <= 1 || this.shape == Shape.Cube ) {
			float ty = 0.0f, py = -1.0f;
			int index = 0;
			for( int y = 0; y < hvCount2; ++y, ty += uvFactor, py += scaleFactor ) {
				float tx = 0.0f, px = -1.0f;
				for( int x = 0; x < hvCount2; ++x, ++index, tx += uvFactor, px += scaleFactor ) {
					vertices[index] = new Vector3( px, py, 1.0f );
					uvs[index] = new Vector2( tx, ty );
				}
			}
		} else {
			float ty = 0.0f, py = -1.0f;
			int index = 0, indexY = 0;
			for( int y = 0; y <= hvCount2Half; ++y, indexY += hvCount2, ty += uvFactor, py += scaleFactor ) {
				float tx = 0.0f, px = -1.0f, py2 = py * py;
				int x = 0;
				for( ; x <= hvCount2Half; ++x, ++index, tx += uvFactor, px += scaleFactor ) {
					float d = Mathf.Sqrt( px * px + py2 + 1.0f );
					float theta = Mathf.Acos( 1.0f / d );
					float phi = Mathf.Atan2( py, px );
					float sinTheta = Mathf.Sin( theta );
					vertices[index] = new Vector3(
						sinTheta * Mathf.Cos( phi ),
						sinTheta * Mathf.Sin( phi ),
						Mathf.Cos( theta ) );
					uvs[index] = new Vector2( tx, ty );
				}
				int indexX = hvCount2Half - 1;
				for( ; x < hvCount2; ++x, ++index, --indexX, tx += uvFactor, px += scaleFactor ) {
					Vector3 v = vertices[indexY + indexX];
					vertices[index] = new Vector3( -v.x, v.y, v.z );
					uvs[index] = new Vector2( tx, ty );
				}
			}
			indexY = (hvCount2Half - 1) * hvCount2;
			for( int y = hvCount2Half + 1; y < hvCount2; ++y, indexY -= hvCount2, ty += uvFactor, py += scaleFactor ) {
				float tx = 0.0f, px = -1.0f;
				int x = 0;
				for( ; x <= hvCount2Half; ++x, ++index, tx += uvFactor, px += scaleFactor ) {
					Vector3 v = vertices[indexY + x];
					vertices[index] = new Vector3( v.x, -v.y, v.z );
					uvs[index] = new Vector2( tx, ty );
				}
				int indexX = hvCount2Half - 1;
				for( ; x < hvCount2; ++x, ++index, --indexX, tx += uvFactor, px += scaleFactor ) {
					Vector3 v = vertices[indexY + indexX];
					vertices[index] = new Vector3( -v.x, -v.y, v.z );
					uvs[index] = new Vector2( tx, ty );
				}
			}
		}
		
		{
			for( int y = 0, index = 0, ofst = 0; y < this.segments; ++y, ofst += hvCount2 ) {
				int y0 = ofst, y1 = ofst + hvCount2;
				for( int x = 0; x < this.segments; ++x, index += 6 ) {
					triangles[index+0] = y0 + x;
					triangles[index+1] = y1 + x;
					triangles[index+2] = y0 + x + 1;
					triangles[index+3] = y1 + x;
					triangles[index+4] = y1 + x + 1;
					triangles[index+5] = y0 + x + 1;
				}
			}
		}
		
		mesh.vertices = vertices;
		mesh.uv = uvs;
		mesh.triangles = triangles;
		return mesh;
	}
	
	void _CreatePlane( Mesh mesh, string textureName, Quaternion rotation )
	{
		GameObject go = new GameObject();
		go.name = textureName;
		go.transform.parent = this.transform;
		go.transform.localPosition = Vector3.zero;
		go.transform.localScale = new Vector3( this.radius, this.radius, this.radius );
		go.transform.localRotation = rotation;
		Material material = new Material( this.material );
		material.mainTexture = skybox.GetTexture( textureName );
		MeshRenderer meshRenderer = go.AddComponent< MeshRenderer >();
		meshRenderer.material = material;
		meshRenderer.castShadows = false;
		meshRenderer.receiveShadows = false;
		MeshFilter meshFilter = go.AddComponent< MeshFilter >();
		meshFilter.mesh = mesh;
	}
}
