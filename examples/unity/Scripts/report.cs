using UnityEngine;
using System.Collections;
using Ovr;

public class report : MonoBehaviour {

	// Use this for initialization
	void Start () {

		Debug.Log (OVRManager.capiHmd.GetString(Hmd.OVR_KEY_USER, "test"));
	
	}
	
}
