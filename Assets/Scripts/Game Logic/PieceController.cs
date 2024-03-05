using System;
using System.Collections.Generic;
using UnityEngine;

public class PieceController : MonoBehaviour
{
    [SerializeField][Range(0f,2f)] private float _enlargeAmount = 1.2f;
    [SerializeField] private SpriteRenderer _renderer;
    [SerializeField] private SpriteRenderer _inactiveRenderer;
    [SerializeField] private SpriteRenderer _frozenRenderer;
    [SerializeField] private TouchController _touchController;
    [SerializeField] private ScaleController _shrinkAnimation;
    [SerializeField] private ScaleController _growAnimation;
    [SerializeField] private PositionController _moveAnimation;
    [SerializeField] private SpritesAsset _spritesAsset;
    
    private BoardController _board;
    private SettingsController _settings;
    private Piece _piece;
    private GameObject _trappedParent;
    private List<PieceController> _trappedList;

    private Vector2Int _trappedPosition;
    private Action _onMove;
    private Action _onTrapped;
    private bool _moving = false;
    private bool _shrinking = false;
    private bool _growing = false;
    private bool _dropping = false;
    private bool _movingBackwards = false;
    private bool _isUpdating = false;
    private float _animationSpeed = 0.5f;

    public bool Animate { get; set; } = false;
    public int SpriteIndex { get { return _piece.Strength - 1; } }
    public bool IsGold { get { return _piece.IsGold; } }
    public bool IsTrapped { get; set; }
    public bool MovingBackwards { get { return _board.MovingBackwards; } }
    public Piece Piece { get { return _piece; } }
    public Vector2Int Position { get; set; }
    public Vector2Int BoardPosition { get { return new Vector2Int(Mathf.FloorToInt(transform.position.x + 0.5f), Mathf.FloorToInt(transform.position.y + 0.5f)); } }
    public bool IsUpdating 
    {
        get { return _isUpdating; }
        private set 
        {
            _isUpdating = value;
            if (value) { _board.IsUpdating = true; }
            else { _board.onPieceFinishUpdate?.Invoke(); }
        }
    }

    public PieceController Init(Piece piece, SettingsController settings, BoardController board, GameObject trappedParent, List<PieceController> trappedList, Action onMove, Action onTrapped)
    {
        _piece = piece;
        _trappedParent = trappedParent;
        _trappedList = trappedList;
        _onMove = onMove;
        _onTrapped = onTrapped;
        Position = piece.Position;
        transform.position = new Vector2(piece.Position.x, piece.Position.y);
        _board = board;
        _settings = settings;
        _settings.OnSave.AddListener(UpdateSprite);
        _settings.OnLoad.AddListener(UpdateSprite);

        gameObject.name = _piece.NameLabel;

        return this;
    }


    private void OnEnable()
    {
        if (_settings == null) { return; }
        _settings.OnSave.AddListener(UpdateSprite);
        _settings.OnLoad.AddListener(UpdateSprite);
    }
    private void OnDisable()
    {
        _settings.OnSave.RemoveListener(UpdateSprite);
        _settings.OnLoad.RemoveListener(UpdateSprite);
    }

    private void Activate()
    {
        _inactiveRenderer.gameObject.SetActive(false);
        _touchController.Active = true;
    }
    private void Deactivate() 
    {
        _inactiveRenderer.gameObject.SetActive(true);
        _touchController.Active = false;
    }
    private void Freeze() { _frozenRenderer.gameObject.SetActive(true); }
    public void Unfreeze() { _frozenRenderer.gameObject.SetActive(false); }
    private void Trap() 
    {
        _trappedPosition = _piece.TrapPosition;
        IsTrapped = true;
        _frozenRenderer.gameObject.SetActive(false);

        if (Animate) 
        {
            if (!MovingBackwards) { _onTrapped?.Invoke(); }
            _shrinkAnimation.Play(() =>
            {
                transform.parent = _trappedParent.transform;
                transform.localScale = new Vector3(0.5f, 0.5f, 1f);
                if (!_trappedList.Contains(this)) { _trappedList.Add(this); }
                _board.PositionDeadPieces();
                IsUpdating = false;
            });
            IsUpdating = true;
        }
        else 
        {
            transform.parent = _trappedParent.transform;
            transform.localScale = new Vector3(0.5f, 0.5f, 1f);
            if (!_trappedList.Contains(this)) { _trappedList.Add(this); }
            _board.PositionDeadPieces();
            return;
        }
    }
    private void Untrap() 
    {
        IsTrapped = false;

        transform.parent = _board.transform;

        if (_board.Turn == 1) { transform.position = new Vector2(_piece.Position.x, _piece.Position.y); }
        else { transform.position = new Vector2(_trappedPosition.x, _trappedPosition.y); }
        transform.localScale = new Vector3(1f, 1f, 1f);

        _trappedList.Remove(this);
        _board.PositionDeadPieces();

        gameObject.SetActive(true);

        if (Animate) 
        {
            _growAnimation.Play();
            _growAnimation.OnEnd(()=> { IsUpdating = false; });
            IsUpdating = true;
        }
    }
    private void Move(Vector2Int target) 
    {
        Position = target;
        if (!_shrinking && !_growing && !_movingBackwards && !_board.IsGrowing && !_board.IsShrinking) { _onMove?.Invoke(); }
        if (!Animate) { transform.position = new Vector3(_piece.Position.x, _piece.Position.y); }
        else 
        {
            _moveAnimation.Play(new Vector3(_piece.Position.x, _piece.Position.y));
            _moveAnimation.OnEnd(() => { IsUpdating = false; });
            IsUpdating = true;
        }
        _movingBackwards = false;
    }

    public void UpdatePieceState()
    {
        if (_piece.IsActive) { Activate(); }
        else { Deactivate(); }

        if (_piece.IsFrozen) { Freeze(); }
        else { Unfreeze(); }

        if (IsTrapped && !_piece.IsTrapped) { _growing = true; _board.IsGrowing = true; }
        else if (!IsTrapped && _piece.IsTrapped) { _shrinking = true; _board.IsShrinking = true; }

        if (Position != _piece.Position) { _moving = true; }
        else if (_shrinking && Position != BoardPosition) { _moving = true; }
        _movingBackwards = MovingBackwards;

        if (_growing && _moving && !_movingBackwards) 
        {
            Animate = false;
            Untrap();
            Move(_piece.Position);
            _onMove?.Invoke();
            Animate = true;
        }
        else if (_growing)
        {
            Untrap();
            if (_moving) 
            {
                if (_growAnimation.IsPlaying) { _growAnimation.OnEnd(() => { Move(_trappedPosition); }); }
                else { Move(_trappedPosition); }            
            }
        } 
        else if (_shrinking)
        {
            if (!MovingBackwards) 
            {
                if (_moving) { Move(_piece.Position); }

                if (_moving && _moveAnimation.IsPlaying)
                {
                    _moveAnimation.OnEnd(() => { Trap(); });
                    if (_dropping) { _moveAnimation.Stop(); }
                }
                else { Trap(); }
            }
            else { Trap(); }
        } 
        else if(_moving) { Move(_piece.Position); }


        _moving = false;
        _growing = false;
        _shrinking = false;
        _dropping = false;
    }
    public void UpdateSpriteFlip()
    {
        if (_board.IsEditing) { SetStateNormal(); }
        else if (_board.IsHvH)
        {
            if (_settings.Current.Flip && !_board.IsGoldsTurn) { SetStateFliped(); }
            else { SetStateNormal(); }
        }
        else if (_board.IsCvH) { SetStateFliped(); }
        else { SetStateNormal(); }
    }
    public void UpdateSprite()
    {
        _animationSpeed = _settings.Current.AnimationSpeed + 1;

        _moveAnimation.Speed = _animationSpeed;
        _shrinkAnimation.Speed = _animationSpeed;
        _growAnimation.Speed = _animationSpeed;

        if (_settings.Current.Piece == 0) 
        {
            if (IsGold)
            {
                _renderer.color = Color.white;
                _renderer.sprite = _spritesAsset.GoldSprites[SpriteIndex];
                _frozenRenderer.sprite = _spritesAsset.GoldSprites[SpriteIndex];
                _inactiveRenderer.sprite = _spritesAsset.GoldSprites[SpriteIndex];
            }
            else
            {
                _renderer.flipX = false;
                _renderer.color = Color.white;
                _renderer.sprite = _spritesAsset.SilverSprites[SpriteIndex];

                _frozenRenderer.flipX = false;
                _frozenRenderer.sprite = _spritesAsset.SilverSprites[SpriteIndex];

                _inactiveRenderer.flipX = false;
                _inactiveRenderer.sprite = _spritesAsset.SilverSprites[SpriteIndex];
            }
        }
        else if (_settings.Current.Piece == 1) 
        {
            if (IsGold) 
            {
                _renderer.color = _spritesAsset.GoldColor;
                _renderer.sprite = _spritesAsset.WhiteSprites[SpriteIndex];
                _frozenRenderer.sprite = _spritesAsset.WhiteSprites[SpriteIndex];
                _inactiveRenderer.sprite = _spritesAsset.WhiteSprites[SpriteIndex]; 
            }
            else 
            {
                _renderer.flipX = true;
                _renderer.color = _spritesAsset.SilverColor;
                _renderer.sprite = _spritesAsset.WhiteSprites[SpriteIndex];

                _frozenRenderer.flipX = true;
                _frozenRenderer.sprite = _spritesAsset.WhiteSprites[SpriteIndex];

                _inactiveRenderer.flipX = true;
                _inactiveRenderer.sprite = _spritesAsset.WhiteSprites[SpriteIndex];
            }
        }
    }
   
    public void PickUp() 
    {
        _board.ShowLegalMoves(Position);
        if (_piece.IsTrapped && _board.IsEditing) { transform.localScale = Vector3.one; }
        if (_settings.Current.Enlarge) 
        {
            _renderer.transform.localScale = Vector3.one * _enlargeAmount; 
            _frozenRenderer.transform.localScale = Vector3.one * _enlargeAmount; 
        } 
    }
    public void Drag(Vector3 position) 
    {
        position += Vector3.one * 0.5f;
        _board.UpdatePath(new Vector2Int(Mathf.FloorToInt(position.x), Mathf.FloorToInt(position.y)));
    }
    public void Drop(Vector3 position) 
    {
        _dropping = true;
        if (_board.IsPositionLegal(position + (Vector3.one * 0.5f))) { _board.MakeMove(this, position + (Vector3.one * 0.5f)); }
        else 
        {
            _dropping = false;
            Move(_piece.Position);
        }
        
        if (_piece.IsTrapped && _board.IsEditing) { transform.localScale = new Vector3(0.5f, 0.5f, 1f); }
        if (_settings.Current.Enlarge) 
        {
            _renderer.transform.localScale = Vector3.one;
            _frozenRenderer.transform.localScale = Vector3.one; 
        }

        _board.HideLegalMoves();
    }

    public void UpdatePiece() 
    {
        Move(_piece.Position);
    }

    private void SetStateNormal() 
    {
        _renderer.flipY = false;
        _frozenRenderer.flipY = false;
        _inactiveRenderer.flipY = false;

        _renderer.flipX = false;
        _frozenRenderer.flipX = false;
        _inactiveRenderer.flipX = false;
    }
    private void SetStateFliped() 
    {
        _renderer.flipY = true;
        _frozenRenderer.flipY = true;
        _inactiveRenderer.flipY = true;

        _renderer.flipX = true;
        _frozenRenderer.flipX = true;
        _inactiveRenderer.flipX = true;
    }
}

