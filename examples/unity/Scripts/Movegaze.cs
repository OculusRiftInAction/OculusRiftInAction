using UnityEngine;
using System.Collections;

public class Movegaze : MonoBehaviour {

	private  float startTime;
	private  GameObject attachedObject = null;
	private  GameObject lookedatObject  = null;

	void Start (){

		startTime = Time.time;
	}

	// Update is called once per frame
	void Update () {
		Ray ray = new Ray(transform.position, transform.forward);
		RaycastHit hit;
		float currentTime = Time.time;
		
		if (attachedObject == null) { 
			// Check to see if the ray cast from the CenterEyeObject hit anything
			if(Physics.Raycast(ray, out hit, 100)) {
				// If the ray cast has hit something check to see if it has been for longer than 2 seconds
				if((currentTime - startTime) > 2){
					attachedObject = lookedatObject;
					attachedObject.rigidbody.useGravity = false;
					attachedObject.rigidbody.isKinematic = false;
					attachedObject.transform.parent = this.transform;
					attachedObject.renderer.material.color = Color.red;
				} else {
					// If less than 2 seconds make it the looked at object and turn it blue
					if (lookedatObject == null){
						lookedatObject = hit.collider.gameObject;
						lookedatObject.renderer.material.color = Color.blue;
					}
				}
			} else {
				// If the ray isn't hitting anything
				if (lookedatObject != null) {
					lookedatObject.renderer.material.color = Color.white;
				}
				startTime = currentTime;
				lookedatObject = null;
			} 
		} else { 
			// If the attached object is not null, check to see if it has been put down.
			if (attachedObject.renderer.material.color == Color.white){	
				startTime = currentTime;
				attachedObject = null;
			}	
		}	
	}
}
