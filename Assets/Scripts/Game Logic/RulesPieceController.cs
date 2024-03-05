using System;
using System.Collections.Generic;
using UnityEngine;
using static UnityEngine.GraphicsBuffer;

public class RulesPieceController : MonoBehaviour
{
    [SerializeField] private SpriteRenderer _renderer;
    [SerializeField] private SpriteRenderer _frozenRenderer;
    [SerializeField] private ScaleController _shrinkAnimation;
    [SerializeField] private ScaleController _growAnimation;
    [SerializeField] private PositionController _moveAnimation;
    [SerializeField] private SpritesAsset _spritesAsset;
    
    private RulesController _board;
    private Piece _piece;
    private GameObject _trappedParent;
    private List<RulesPieceController> _trappedList;

    private Action _onMove;
    private Action _onTrapped;
    private bool _moving = false;
    private bool _shrinking = false;
    private bool _growing = false;
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

    public RulesPieceController Init(Piece piece, RulesController board, GameObject trappedParent, List<RulesPieceController> trappedList, Action onMove, Action onTrapped)
    {
        _piece = piece;
        _trappedParent = trappedParent;
        _trappedList = trappedList;
        _onMove = onMove;
        _onTrapped = onTrapped;
        Position = piece.Position;
        transform.position = new Vector2(piece.Position.x, piece.Position.y);
        _board = board;

        gameObject.name = _piece.NameLabel;

        return this;
    }

    private void Freeze() { _frozenRenderer.gameObject.SetActive(true); }
    public void Unfreeze() { _frozenRenderer.gameObject.SetActive(false); }
    private void Trap() 
    {
        IsTrapped = true;
        _frozenRenderer.gameObject.SetActive(false);

        if (Animate) 
        {
            if (!MovingBackwards) { _onTrapped?.Invoke(); }
            _shrinkAnimation.Play(() =>
            {
                transform.parent = _trappedParent.transform;
                if (!_trappedList.Contains(this)) { _trappedList.Add(this); }
                _board.PositionDeadPieces();
                IsUpdating = false;
            });
            IsUpdating = true;
        }
        else 
        {
            transform.parent = _trappedParent.transform;
            if (!_trappedList.Contains(this)) { _trappedList.Add(this); }
            _board.PositionDeadPieces();
            IsUpdating = false;
            return;
        }
    }
    private void Untrap() 
    {
        IsTrapped = false;

        transform.parent = _board.transform;

        _trappedList.Remove(this);
        _board.PositionDeadPieces();
        transform.localScale = new Vector3(0f, 0f, 1f);

        gameObject.SetActive(true);

        if (Animate)
        {
            _growAnimation.Play();
            _growAnimation.OnEnd(() => { IsUpdating = false; });
        }
        else 
        {
            transform.localScale = new Vector3(1f, 1f, 1f);
            IsUpdating = false; 
        }
    }
    private void Move(Vector2Int target) 
    {
        Position = target;
        if (!_shrinking && !_growing && !_movingBackwards && !_board.IsGrowing && !_board.IsShrinking) { _onMove?.Invoke(); }
        if (!Animate) 
        {
            transform.position = new Vector3(_piece.Position.x, _piece.Position.y); 
            if(!_shrinking && !_growing) { IsUpdating = false; }
        }
        else 
        {
            _moveAnimation.Play(new Vector3(_piece.Position.x, _piece.Position.y));
            _moveAnimation.OnEnd(() => { IsUpdating = false; });
        }
        _movingBackwards = false;
    }

    public void UpdatePieceState()
    {
        if (IsUpdating)
        {
            _moveAnimation.Stop();
            
            _growAnimation.Stop();
            _shrinkAnimation.Stop();

            if (!IsTrapped) { transform.localScale = new Vector3(1f, 1f, 1f); }

            transform.position = new Vector3(_piece.Position.x, _piece.Position.y);
        }

        if (_piece.IsFrozen) { Freeze(); }
        else { Unfreeze(); }

        if (IsTrapped && !_piece.IsTrapped) { _growing = true; _board.IsGrowing = true; }
        else if (!IsTrapped && _piece.IsTrapped) { _shrinking = true; _board.IsShrinking = true; }

        if (Position != _piece.Position) { _moving = true; }
        else if (_shrinking && Position != BoardPosition) { _moving = true; }
        _movingBackwards = MovingBackwards;

        if (!_moving && !_growing && !_shrinking) { return; }

        if (IsTrapped && _moving && !_growing)
        {
            Position = _piece.Position;
            transform.position = new Vector3(_piece.Position.x, _piece.Position.y);
            return;
        }

        IsUpdating = true;

        if (_growing && _moving) 
        {
            Animate = false;
            transform.localScale = new Vector3(0f, 0f, 1f);
            Move(_piece.Position);
            Animate = true;
            Untrap();
        }
        else if (_growing) { Untrap(); } 
        else if (_shrinking && _board.IsTrapPosition(_piece.Position))
        {
            if (!MovingBackwards) 
            {
                if (_moving) { Move(_piece.Position); }

                if (_moving && _moveAnimation.IsPlaying) { _moveAnimation.OnEnd(() => { Trap(); }); }
                else { Trap(); }
            }
            else { Trap(); }
        }
        else if (_shrinking)
        {
            Animate = false;
            Trap();
            Animate = true;
        }
        else if(_moving) { Move(_piece.Position); }


        _moving = false;
        _growing = false;
        _shrinking = false;
    }
    public void UpdateSprite()
    {
        _animationSpeed = 3;

        _moveAnimation.Speed = _animationSpeed;
        _shrinkAnimation.Speed = _animationSpeed;
        _growAnimation.Speed = _animationSpeed;

        if (IsGold)
        {
            _renderer.color = Color.white;
            _renderer.sprite = _spritesAsset.GoldSprites[SpriteIndex];
            _frozenRenderer.sprite = _spritesAsset.GoldSprites[SpriteIndex];
        }
        else
        {
            _renderer.flipX = false;
            _renderer.color = Color.white;
            _renderer.sprite = _spritesAsset.SilverSprites[SpriteIndex];

            _frozenRenderer.flipX = false;
            _frozenRenderer.sprite = _spritesAsset.SilverSprites[SpriteIndex];
        }
    }
   
    public void UpdatePiece() 
    {
        Move(_piece.Position);
    }
}

