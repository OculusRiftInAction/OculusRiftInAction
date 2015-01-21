using UnityEngine;
using System.Collections;

public class Cubecollision : MonoBehaviour {

	void OnCollisionEnter (Collision col){
		renderer.material.color = Color.white;
		transform.parent = null;
		transform.rotation = Quaternion.Euler(0, 0, 0);
		if(col.gameObject.name == "Beach"){
			Vector3 newPos = transform.position;
			newPos.y = col.gameObject.transform.position.y + transform.localScale.y/2.0f + .1f;
			transform.position = newPos;

			rigidbody.isKinematic = true;
			rigidbody.useGravity = true;
		}else {
			
			rigidbody.useGravity = true;
		} 
	}
	
	void OnCollisionStay() {
		// if the cubes land in a colliding position, apply force until they move apart.
		rigidbody.AddForce(transform.forward * 20);
		
	}
}
