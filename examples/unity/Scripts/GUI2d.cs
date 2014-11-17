using UnityEngine;
using System.Collections;

public class GUI2d : MonoBehaviour {

	private float startTime; 
	private float elapsedTime;
	private bool displaytimer;
	private bool OLDtimerkey;
	
	void Awake(){ 
		startTime = Time.time; 
		displaytimer = true;
		
	}
	
	void Update () {
		
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
		
		
		if(displaytimer){
			// Make a background box
			GUI.Box(new Rect(Screen.width / 2 - 75, Screen.height / 2 - 25, 150, 50), "SECONDS ELAPSED");
			
			// Display elapsed time 
			GUI.Label(new Rect(Screen.width/2, Screen.height/2, 100, 50), (elapsedTime.ToString("N0")));
		}
	} 
}

