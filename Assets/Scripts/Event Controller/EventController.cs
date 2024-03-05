using UnityEngine;
using UnityEngine.Events;

public class EventController : MonoBehaviour
{
    [SerializeField] private UnityEvent Event;
    public void Play() { Event?.Invoke(); }
}