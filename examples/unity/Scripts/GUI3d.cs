using UnityEngine;
using System.Collections;
using Ovr;

public class GUI3d : MonoBehaviour {
		
		private GameObject      GUIRenderObject  = null;     		
		private RenderTexture    GUIRenderTexture = null;		
		public Vector3 guiPosition      = new Vector3(0f, 0f, 1f);	
		private float startTime; 					
		private float elapsedTime;
		private bool displaytimer;
		private bool OLDtimerkey;
			
			
		void Start () 
		{
			GUIRenderObject = Instantiate(Resources.Load("OVRGuiObjectMain"))as GameObject; 	
				
			GUIRenderObject.transform.parent        = this.transform;  	
			GUIRenderObject.transform.localPosition = guiPosition;		
				
			GUIRenderTexture = new RenderTexture(Screen.width, Screen.height, 24);						
				
			GUIRenderObject.renderer.material.mainTexture = GUIRenderTexture;
				
			startTime = Time.time; 		
			displaytimer = true;
		}
		
		void Update () {

			if (OVRManager.isHSWDisplayed){  			
				startTime = Time.time;
			}
			elapsedTime = Time.time - startTime;

			bool timerkey = Input.GetKey(KeyCode.T);
			if ((OLDtimerkey == false) && (timerkey == true)){
				if(displaytimer){
					displaytimer = false;
				}else{
					displaytimer = true;
				}
			}
			OLDtimerkey = timerkey;
		}
		void OnGUI(){
			
			if (Event.current.type != EventType.Repaint) 		
				return;
			
			GUIRenderObject.SetActive(true);				
				
				Vector3 scale = Vector3.one; 					
				Matrix4x4 svMat = GUI.matrix; 				
				GUI.matrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity,scale); 									 	
				
				
				RenderTexture previousActive = RenderTexture.active;		
				
				
				if(GUIRenderTexture != null && GUIRenderObject.activeSelf)	
			{
				RenderTexture.active = GUIRenderTexture;			
					GL.Clear (false, true, new Color (0.0f, 0.0f, 0.0f, 0.0f));  
			}
			
			if(displaytimer == true){
				GUI.Box(new Rect(Screen.width / 2 - 75, Screen.height / 2 - 25, 150, 50), "SECONDS ELAPSED");		
				GUI.Label(new Rect(Screen.width/2, Screen.height/2, 100, 50),(elapsedTime.ToString("N0")));;					
			}	
				
				if (GUIRenderObject.activeSelf)		
			{
				RenderTexture.active = previousActive;
			}
			
			
			GUI.matrix = svMat;				
		} 
	}
