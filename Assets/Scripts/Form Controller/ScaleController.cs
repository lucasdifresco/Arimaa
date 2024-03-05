using System;
using UnityEngine;
using UnityEngine.Events;

public class ScaleController : MonoBehaviour
{
    [SerializeField] private Transform _target;
    [SerializeField] private float _speed;
    [SerializeField][Range(0f, 1f)] private float _startScale = 0f;
    [SerializeField][Range(0f, 1f)] private float _endScale = 1f;
    [SerializeField] private bool _playBack;
    [SerializeField] private bool _isLoop;
    [SerializeField] private bool _playOnAwake;
    [SerializeField] private bool _useUnscaledTime = true;
    [SerializeField] private UnityEvent _onFinish;

    private Vector3 Scale { get { return _target.localScale; } set { _target.localScale = value; } }

    private float _timer = 0f;
    private bool _playing = false;
    private bool _playingForward = true;
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
        UpdateScale();
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
    private void UpdateScale() { Scale = new Vector3(Mathf.Lerp(_startScale, _endScale, _timer), Mathf.Lerp(_startScale, _endScale, _timer), Scale.z); }
    private void PlayOnEnd() { if (_onEnd != null) { _onEnd?.Invoke(); _onEnd = null; } }

    public void Play() { _playing = true; }
    public void Play(Action onEnd) { _playing = true; _onEnd = onEnd; }
    public void OnEnd(Action onEnd) { _onEnd = onEnd; }
    public void OnFinish(Action onFinish) { _onFinish.AddListener(() => { onFinish?.Invoke(); }); }
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
        UpdateScale();
        PlayOnEnd();
        _timer = 0f;
        _playingForward = true;
    }
}
