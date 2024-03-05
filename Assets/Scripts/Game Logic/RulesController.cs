using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;
using System.IO;

public class RulesController : MonoBehaviour
{
    [SerializeField] private SpriteRenderer _renderer;
    [SerializeField] private SpritesAsset _spritesAsset;
    [SerializeField] private GameObject _piecePrefab;
    [SerializeField] private GameObject _legalMovePrefab;
    [SerializeField] private GameObject _pathMovePrefab;
    [SerializeField] private GameObject _legalMoveMarkerParent;
    [SerializeField] private GameObject _goldDeadPieceParent;
    [SerializeField] private GameObject _silverDeadPieceParent;
    [SerializeField] private TextAsset _rules;
    
    [SerializeField] private Button _undoButton;
    [SerializeField] private Button _redoButton;

    [SerializeField] private UnityEvent _onMoveMake;
    [SerializeField] private UnityEvent _onPieceTrapped;
    [SerializeField] private UnityEvent _onChangeTurn;

    private Board _board;
    private List<RulesPieceController> _pieces;
    private List<RulesPieceController> _goldDeadPieces;
    private List<RulesPieceController> _silverDeadPieces;
    private Grid<GameObject> _legalMovesGrid;
    private Grid<GameObject> _pathMovesGrid;

    private bool _redoing = false;
    private bool _undoing = false;

    public Action onPieceFinishUpdate;
    private Action _undoUpdate;

    private int _undoCount = 0;

    public int Turn { get { return _board.Turn; } }
    public bool EndOfMove { get { return _board.EndOfMove; } }
    public bool Pulling { get { return _board.Pulling; } }
    public bool MovingBackwards { get; private set; }
    public bool IsGrowing { get; set; }
    public bool IsShrinking { get; set; }
    public bool IsUpdating { get; set; }
    
    private void Awake()
    {
        SavedGameAsset.SavedGame _savedGame = new SavedGameAsset.SavedGame(_rules.text);

        _board = new Board(_savedGame.History, true);
        _pieces = new List<RulesPieceController>();
        _goldDeadPieces = new List<RulesPieceController>();
        _silverDeadPieces = new List<RulesPieceController>();

        GameObject current;
        
        _legalMovesGrid = new Grid<GameObject>(8, 8, (x, y, grid) => 
        {
            current = Instantiate(_legalMovePrefab, _legalMoveMarkerParent.transform);
            current.SetActive(false);
            current.transform.localPosition = (Vector3.right * x) + (Vector3.up * y);
            return current; 
        });
        _pathMovesGrid = new Grid<GameObject>(8, 8, (x, y, grid) =>
        {
            current = Instantiate(_pathMovePrefab, _legalMoveMarkerParent.transform);
            current.SetActive(false);
            current.transform.localPosition = (Vector3.right * x) + (Vector3.up * y);
            return current;
        });

        foreach (Piece piece in _board.Pieces) { InsanciateNewPiece(piece); }

        onPieceFinishUpdate = () =>
        {
            foreach (RulesPieceController piece in _pieces) { if (piece.IsUpdating) { return; } }
            _undoUpdate?.Invoke();
            IsUpdating = false;
        };
    }
    private void Start()
    {
        UpdateTurn();
        foreach (RulesPieceController piece in _pieces) { piece.UpdateSprite(); piece.Animate = true; }
        UpdateTurn();
    }
    private void InsanciateNewPiece(Piece piece) 
    {
        RulesPieceController pieceController = Instantiate(_piecePrefab, transform).GetComponent<RulesPieceController>();
        pieceController.Init(
            piece, 
            this, 
            ((piece.IsGold) ? _goldDeadPieceParent : _silverDeadPieceParent), 
            ((piece.IsGold) ? _goldDeadPieces : _silverDeadPieces), 
            () => { _onMoveMake?.Invoke(); }, 
            () => { _onPieceTrapped?.Invoke(); }
            );
        _pieces.Add(pieceController);
    }

    public void UpdateTurn() 
    {
        _board.UpdatePiecesActivation();

        foreach (RulesPieceController piece in _pieces) 
        {
            if (Turn == 1 && piece.IsTrapped) { piece.Animate = false; }
            piece.UpdatePieceState();
            if (Turn == 1) { piece.Animate = true; }
        }
        PositionDeadPieces();

        UpdateButtons();
        CheckPositionBoxes();

        _onChangeTurn?.Invoke();

        IsGrowing = false;
        IsShrinking = false;
    }
    private void UpdateButtons() 
    {
        _undoButton.interactable = _board.CanUndo;
        _redoButton.interactable = _board.CanRedo;            
    }
    public void PositionDeadPieces() 
    {        
        _goldDeadPieceParent.gameObject.SetActive(false);
        _silverDeadPieceParent.gameObject.SetActive(false);
    }
    public void Undo() 
    {
        if (_redoing) { return; }
        if (!_board.CanUndo) { return; }

        _undoing = true;
        _board.Undo();

        //print($"{_board.History.Count - _board.UndoIndex} - {_undoCount}");

        if (EndOfMove) 
        {
            _undoing = false;
            _undoCount--;
            _undoUpdate = null;
            foreach (RulesPieceController piece in _pieces) { piece.Animate = false; }
            MovingBackwards = true;
            UpdateTurn();
            MovingBackwards = false;
            foreach (RulesPieceController piece in _pieces) { piece.Animate = true; }
        }
        else if (IsUpdating) { _undoUpdate = Undo; }
        else { Undo(); }
    }
    public void Redo() 
    {
        if (_undoing) { return; }
        if (!_board.CanRedo) { return; }

        _redoing = true;
        _board.Redo();
        if (Pulling) { _undoCount++; }
        UpdateTurn();

        //print($"{_board.History.Count - _board.UndoIndex} - {_undoCount}");
        if (EndOfMove) 
        {
            _redoing = false;
            _undoUpdate = null; 
        }
        else if (IsUpdating) { _undoUpdate = Redo; }
        else { Redo(); }
    }

    public bool IsTrapPosition(Vector2Int position) { return _board.IsTrapPosition(position); }
    public void SetInfoLabel(LabelController label) 
    {
        string text = "";
        
        if (_undoCount == 0) { text = "How to play Arimaa"; }
        else if (_undoCount == 1) { text = "In Arimaa, there are 6 kinds of pieces. From strongest to weakest:"; }
        else if (_undoCount == 2) { text = "In Arimaa, there are 6 kinds of pieces. From strongest to weakest:\nElephant,"; }
        else if (_undoCount == 3) { text = "In Arimaa, there are 6 kinds of pieces. From strongest to weakest:\nElephant, camel,"; }
        else if (_undoCount == 4) { text = "In Arimaa, there are 6 kinds of pieces. From strongest to weakest:\nElephant, camel, horse,"; }
        else if (_undoCount == 5) { text = "In Arimaa, there are 6 kinds of pieces. From strongest to weakest:\nElephant, camel, horse, dog,"; }
        else if (_undoCount == 6) { text = "In Arimaa, there are 6 kinds of pieces. From strongest to weakest:\nElephant, camel, horse, dog, cat,"; }
        else if (_undoCount == 7) { text = "In Arimaa, there are 6 kinds of pieces. From strongest to weakest:\nElephant, camel, horse, dog, cat, rabbit."; }
        else if (_undoCount == 8) { text = "The goal is to get a rabbit to the opposite end of the board."; }
        else if (_undoCount == 9) { text = "The goal is to get a rabbit to the opposite end of the board."; }
        else if (_undoCount == 10) { text = "Regrdless of strength, pieces move only one step at a time, either left, right, forward or backward."; }
        else if (_undoCount == 11) { text = "Regrdless of strength, pieces move only one step at a time, either left, right, forward or backward."; }
        else if (_undoCount == 12) { text = "Except for rabbits, which cannot step backward."; }
        else if (_undoCount == 13) { text = "Except for rabbits, which cannot step backward."; }
        else if (_undoCount == 14) { text = "Stronger pieces can push weaker opposing pieces."; }
        else if (_undoCount == 15) { text = "Stronger pieces can push weaker opposing pieces."; }
        else if (_undoCount == 16) { text = "Both steps of a push need not to be in the same direction."; }
        else if (_undoCount == 17) { text = "Both steps of a push need not to be in the same direction."; }
        else if (_undoCount == 18) { text = "Both steps of a push need not to be in the same direction."; }
        else if (_undoCount == 19) { text = "Similarly, stronger pieces can pull weaker opposing pieces."; }
        else if (_undoCount == 20) { text = "Similarly, stronger pieces can pull weaker opposing pieces."; }
        else if (_undoCount == 21) { text = "Similarly, stronger pieces can pull weaker opposing pieces."; }
        else if (_undoCount == 22) { text = "Similarly, stronger pieces can pull weaker opposing pieces."; }
        else if (_undoCount == 23) { text = "To begin the game, players set up their pieces on the first two rows in any desired arrangement."; }
        else if (_undoCount == 24) { text = "To begin the game, players set up their pieces on the first two rows in any desired arrangement."; }
        else if (_undoCount == 25) { text = "Each turn, players can make up to 4 steps in any desired combination."; }
        else if (_undoCount == 26) { text = "Each turn, players can make up to 4 steps in any desired combination."; }
        else if (_undoCount == 27) { text = "Each turn, players can make up to 4 steps in any desired combination."; }
        else if (_undoCount == 28) { text = "Fewer than 4 steps is fine, as long as the board position changes."; }
        else if (_undoCount == 29) { text = "Pushes and pulls take two steps each."; }
        else if (_undoCount == 30) { text = "Pushes and pulls take two steps each."; }
        else if (_undoCount == 31) { text = "Pushes and pulls take two steps each."; }
        else if (_undoCount == 32) { text = "There are 4 trap squares on the board."; }
        else if (_undoCount == 33) { text = "There are 4 trap squares on the board."; }
        else if (_undoCount == 34) { text = "If a piece steps on a trap square with no friendly adjacent pieces, it gets captured."; }
        else if (_undoCount == 35) { text = "If a piece steps on a trap square with no friendly adjacent pieces, it gets captured."; }
        else if (_undoCount == 36) { text = "As long as there are other friendly pieces defending the trap, the piece is fine."; }
        else if (_undoCount == 37) { text = "But if they all step away, the piece on the trap is captured."; }
        else if (_undoCount == 38) { text = "You can use this to capture opposing pieces by pushing or pulling."; }
        else if (_undoCount == 39) { text = "You can use this to capture opposing pieces by pushing or pulling."; }
        else if (_undoCount == 40) { text = "You can use this to capture opposing pieces by pushing or pulling."; }
        else if (_undoCount == 41) { text = "Pieces are \"frozen\" when next to stronger opposing piece. Frozen pieces cannot move."; }
        else if (_undoCount == 42) { text = "Pieces are \"frozen\" when next to stronger opposing piece. Frozen pieces cannot move."; }
        else if (_undoCount == 43) { text = "Pieces are \"frozen\" when next to stronger opposing piece. Frozen pieces cannot move."; }
        else if (_undoCount == 44) { text = "But pieces are only frozen when there are no friendly adjacent pieces."; }
        else if (_undoCount == 45) { text = "But pieces are only frozen when there are no friendly adjacent pieces."; }
        else if (_undoCount == 46) { text = "So adding another friendly piece allows a frozen piece to move again."; }
        else if (_undoCount == 47) { text = "As mentioned earlier, you win if one of your rabbits reaches the opposite end of the board."; }
        else if (_undoCount == 48) { text = "As mentioned earlier, you win if one of your rabbits reaches the opposite end of the board."; }
        else if (_undoCount == 49) { text = "This is called a \"goal\"."; }
        else if (_undoCount == 50) { text = "Much less commonly, you can win by capturing all of the opponent's rabbits. (\"elimination\")"; }
        else if (_undoCount == 51) { text = "Or if the opponent has no legal moves. (\"immobilization\")"; }
        else if (_undoCount == 52) { text = "Lastly, to prevent endless repetition in certain board positions..."; }
        else if (_undoCount == 53) { text = "Lastly, to prevent endless repetition in certain board positions..."; }
        else if (_undoCount == 54) { text = "It is illegal to make any move that repeats the same board position plus player-to-move a third time."; }
        else if (_undoCount == 55) { text = "That's it!"; }
        
        label.SetLabel(text);
    }
    public void CheckPositionBoxes() 
    {
        if (_undoCount == 33)
        {
            _pathMovesGrid[2, 2].SetActive(true);
            _pathMovesGrid[2, 5].SetActive(true);
            _pathMovesGrid[5, 2].SetActive(true);
            _pathMovesGrid[5, 5].SetActive(true);
        }
        else if (_undoCount == 42 || _undoCount == 45 || (_undoCount == 46 && _board.UndoIndex == 89)) 
        {
            _pathMovesGrid[3, 4].SetActive(true);
            _pathMovesGrid[4, 3].SetActive(false);
        }
        else if (_undoCount == 43 || _undoCount == 44)
        {
            _pathMovesGrid[3, 4].SetActive(true);
            _pathMovesGrid[4, 3].SetActive(true);
        }
        else
        {
            _pathMovesGrid[2, 2].SetActive(false);
            _pathMovesGrid[2, 5].SetActive(false);
            _pathMovesGrid[5, 2].SetActive(false);
            _pathMovesGrid[5, 5].SetActive(false);
            _pathMovesGrid[3, 4].SetActive(false);
            _pathMovesGrid[4, 3].SetActive(false);
        }
    }
}
