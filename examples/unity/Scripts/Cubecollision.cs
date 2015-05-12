using UnityEngine;
using System.Collections;

public class Cubecollision : MonoBehaviour {

	void OnCollisionEnter (Collision col){
		GetComponent<Renderer>().material.color = Color.white;
		transform.parent = null;
		transform.rotation = Quaternion.Euler(0, 0, 0);
		if(col.gameObject.name == "Beach"){
			Vector3 newPos = transform.position;
			newPos.y = col.gameObject.transform.position.y + transform.localScale.y/2.0f + .1f;
			transform.position = newPos;

			GetComponent<Rigidbody>().isKinematic = true;
			GetComponent<Rigidbody>().useGravity = true;
		}else {
			
			GetComponent<Rigidbody>().useGravity = true;
		} 
	}
	
	void OnCollisionStay() {
		// if the cubes land in a colliding position, apply force until they move apart.
		GetComponent<Rigidbody>().AddForce(transform.forward * 20);
		
	}
}
