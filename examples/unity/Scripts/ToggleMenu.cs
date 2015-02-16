using UnityEngine;
using System.Collections;

public class ToggleMenu: MonoBehaviour {
	
	private bool displayTimer;
	private bool oldTimerKey;
	public GameObject timerMenu;
	
	void Start () {
		displayTimer = true;
		timerMenu = GameObject.Find("TimerMenu");
	}
	
	void Update () {
		
		bool timerKey = Input.GetKey(KeyCode.T);        
		if (timerKey && !oldTimerKey){            
			displayTimer = !displayTimer;             
		}
		oldTimerKey= timerKey;
		
		if (displayTimer){
			timerMenu.SetActive (true);			
				
		}else{
			timerMenu.SetActive (false);			
		}
		
	}
}

