using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class UpdateTimer : MonoBehaviour {

	Text elapsedTimeText;						
	private float startTime; 
	private float elapsedTime;
	
	void Awake(){ 
		startTime = Time.time; 
		elapsedTimeText = gameObject.GetComponent<Text>();		
	}
	
	void Update () {
		elapsedTime = Time.time - startTime;
		elapsedTimeText.text = "Elapsed Time " + elapsedTime.ToString("N0");
	}

}
