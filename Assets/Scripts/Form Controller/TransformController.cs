using UnityEngine;

public class TransformController : MonoBehaviour
{
    public void SetRotarionZ(float angle) 
    {
        transform.localEulerAngles = new Vector3(transform.localEulerAngles.x, transform.localEulerAngles.y, angle);
    }
}
