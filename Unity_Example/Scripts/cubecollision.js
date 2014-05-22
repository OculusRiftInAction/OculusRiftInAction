#pragma strict


function OnCollisionEnter (col : Collision){
  renderer.material.color = Color.white;
  transform.parent = null;
  transform.rotation = Quaternion.Euler(0, 0, 0);
    if(col.gameObject.name == "Beach"){
      transform.position.y = col.gameObject.transform.position.y + transform.localScale.y/2 + .1;
      rigidbody.isKinematic = true;
      rigidbody.useGravity = true;
    }else {
    
    rigidbody.useGravity = true;
    } 
}

function OnCollisionStay() {
		// if the cubes land in a colliding position, apply force until they move apart.
	   rigidbody.AddForce(transform.forward * 20);
	
	}