using System;
using UnityEngine;
using UnityEngine.Events;

public class PositionController : MonoBehaviour
{
    [SerializeField] private Transform _target;
    [SerializeField] private float _speed;
    [SerializeField] private bool _playBack;
    [SerializeField] private bool _isLoop;
    [SerializeField] private bool _playOnAwake;
    [SerializeField] private bool _useUnscaledTime = true;
    [SerializeField] private UnityEvent _onFinish;

    private Vector3 Position { get { return _target.position; } set { _target.position = value; } }

    private float _timer = 0f;
    private bool _playing = false;
    private bool _playingForward = true;

    private Vector3 _origin;
    private Vector3 _targetPosition = Vector3.zero;
    private Action _onEnd;

    public float Speed { get { return _speed; } set { _speed = value; } }
    public bool IsPlaying { get { return _playing; } }

    private void Awake() 
    {
        if (_playOnAwake) { Play(); }
    }
    private void Update()
    {
        if (!_playing) { return; }
        UpdateValue();
        UpdateTimer();
    }

    private void UpdateTimer() 
    {
        if (_playingForward)
        {
            _timer += ((_useUnscaledTime) ? Time.unscaledDeltaTime : Time.deltaTime) * _speed;
            if (_timer >= 1f)
            {
                if (_playBack)
                {
                    _playingForward = false;
                    _timer = 1f;
                }
                else
                {
                    if (!_isLoop) 
                    {
                        Stop();
                        _onFinish?.Invoke();
                        return;
                    }
                    _timer = 0f;
                }
            }
        }
        else
        {
            _timer -= ((_useUnscaledTime) ? Time.unscaledDeltaTime : Time.deltaTime) * _speed;
            if (_timer <= 0f)
            {
                _playingForward = true;
                _timer = 0f;
                if (!_isLoop) 
                {
                    Stop(); 
                    _onFinish?.Invoke();
                }
            }
        }
    }
    private void UpdateValue() 
    {
        Position = Vector3.Lerp(_origin, _targetPosition, _timer);
    }

    public void Play()
    {
        _playing = true;
        _origin = Position;
    }
    public void Play(Vector3 target) 
    {
        _playing = true;
        _timer = 0f;
        _playingForward = true;
        _origin = Position;
        _targetPosition = target;
    }
    public void Pause() { _playing = false; }
    public void Restart() 
    {
        _timer = 0f;
        _playingForward = true;
        Play();
    }
    public void Stop()
    {
        Pause();
        _timer = 1f;
        UpdateValue();
        _onEnd?.Invoke();
        _timer = 0f;
        _playingForward = true;
    }
    public void OnEnd(Action action) { _onEnd = action; }
    public void OnFinish(Action onFinish) { _onFinish.AddListener(() => { onFinish?.Invoke(); }); }
}
