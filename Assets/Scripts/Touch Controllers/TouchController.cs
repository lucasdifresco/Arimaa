using UnityEngine;
using UnityEngine.Events;

public class TouchController : MonoBehaviour
{
#if UNITY_EDITOR
    [SerializeField] private bool _debug;
#endif

    public bool Active = true;

    [SerializeField] private GameObject _selectionSquare;
    [SerializeField] private UnityEvent<Vector3> _onPickUp;
    [SerializeField] private UnityEvent<Vector3> _onDrag;
    [SerializeField] private UnityEvent<Vector3> _onDrop;
    
    private Camera _camera;
    private bool _isDragging = false;

    private Vector3 TouchPosition 
    {
        get 
        {
            if (_camera == null) { _camera = Camera.main; }
            return (Vector2)_camera.ScreenToWorldPoint(Input.GetTouch(0).position); 
        } 
    }
    private Vector2 BoardPosition { get { return new Vector3(Mathf.FloorToInt(TouchPosition.x + 0.5f), Mathf.FloorToInt(TouchPosition.y + 0.5f)); } }
    private Vector2 TrappedPosition 
    {
        get
        {
            float baseX = Mathf.FloorToInt(TouchPosition.x + 0.5f);
            float touchX = TouchPosition.x + 0.5f - baseX;
            touchX = baseX + ((touchX < 0.5f) ? 0f : 0.5f) - 0.25f;
            return new Vector3(touchX, Mathf.FloorToInt(TouchPosition.y + 0.5f)); 
        } 
    }
    private bool IsInside { get { return (Vector2)transform.position == ((BoardPosition.y == -1 || BoardPosition.y == 8) ? TrappedPosition : BoardPosition); } }

    public void Activate() { Active = true; }
    public void Deactivate() { Active = false; }

    private void Update()
    {
        if(!Active) { return; }
        
        if (Input.touchCount == 0) { return; }

        if(Input.GetTouch(0).phase == TouchPhase.Began && IsInside) { HandlePickUp(); }


        if (!_isDragging) { return; }
        if (Input.GetTouch(0).phase == TouchPhase.Moved) { HandleDrag(); }
        if(Input.GetTouch(0).phase == TouchPhase.Ended) { HandleDrop(); }
    }

    private void HandlePickUp() 
    {
        _isDragging = true;
        _selectionSquare.gameObject.SetActive(true);
        _onPickUp?.Invoke(TouchPosition);

#if UNITY_EDITOR
        if (_debug) { print($"{name} has been picked up at {TouchPosition}"); }
#endif
    }
    private void HandleDrag() 
    { 
        transform.position = TouchPosition;
        _selectionSquare.transform.position = BoardPosition;
        _onDrag?.Invoke(TouchPosition);

#if UNITY_EDITOR
        if (_debug) { print($"{name} is being dragged to {TouchPosition}"); }
#endif
    }
    private void HandleDrop() 
    {
        _isDragging = false;
        transform.position = BoardPosition;
        _selectionSquare.transform.position = BoardPosition;
        _selectionSquare.gameObject.SetActive(false);

        _onDrop?.Invoke(TouchPosition);

#if UNITY_EDITOR
        if (_debug) { print($"{name} has been dropped at {TouchPosition}"); }
#endif
    }
}
