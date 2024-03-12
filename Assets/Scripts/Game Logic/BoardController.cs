using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using TMPro;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

public class BoardController : MonoBehaviour
{
    [SerializeField] private SpriteRenderer _renderer;
    [SerializeField] private SpritesAsset _spritesAsset;
    [SerializeField] private SavedGameAsset _savedAsset;
    [SerializeField] private SettingsController _settings;
    [SerializeField] private TMP_InputField _savefileNameInput;
    [SerializeField] private TMP_InputField _saveQuitfileNameInput;
    [SerializeField] private GameObject _piecePrefab;
    [SerializeField] private GameObject _legalMovePrefab;
    [SerializeField] private GameObject _pathMovePrefab;
    [SerializeField] private GameObject _legalMoveMarkerParent;
    [SerializeField] private GameObject _goldDeadPieceParent;
    [SerializeField] private GameObject _silverDeadPieceParent;
    [SerializeField] private GameObject _undoVeil;
    [SerializeField] private EventController _pauseEvent;
    
    [SerializeField] private Button _sendButton;
    [SerializeField] private Button _undoButton;
    [SerializeField] private Button _redoButton;

    [SerializeField] private UnityEvent _onMoveMake;
    [SerializeField] private UnityEvent _onPieceTrapped;
    [SerializeField] private UnityEvent _onTurnUpdate;
    [SerializeField] private UnityEvent _onChangeTurn;
    [SerializeField] private UnityEvent _onGoldTurn;
    [SerializeField] private UnityEvent _onSilverTurn;
    [SerializeField] private UnityEvent _onScreenNormal;
    [SerializeField] private UnityEvent _onScreenFlip;
    [SerializeField] private UnityEvent _onGameEnd;
    [SerializeField] private UnityEvent _onRedoMax;

    private Board _board;
    private List<PieceController> _pieces;
    private List<PieceController> _goldDeadPieces;
    private List<PieceController> _silverDeadPieces;
    private Grid<GameObject> _legalMovesGrid;
    private Grid<GameObject> _pathMovesGrid;

    private List<Vector2Int> _legalMoves;
    private bool _calculatingMove = false;
    private bool _autoMoving = false;
    private bool _loadingRecords = false;
    private int _undoCount = 0;

    private Vector2Int _pathOrigin;
    private Vector2Int _pathLastTarget;
    private List<Vector2Int> _path;

    public Action onPieceFinishUpdate;
    private Action _undoUpdate;
    private Action _autoUpdate;
    private List<string> _moves;
    private CancellationTokenSource _cancelationToken;

    private bool IsGoldComputer { get; set; } = false;
    private bool IsSilverComputer { get; set; } = false;
    private int GoldLevel { get; set; } = 0;
    private int SilverLevel { get; set; } = 0;
    private bool IsComputerPlay { get { return !_board.CanRedo && ((IsGoldsTurn && IsGoldComputer) || (!IsGoldsTurn && IsSilverComputer)); } }

    public int Turn { get { return _board.Turn; } }
    public int StepsLeft { get { return _board.StepsLeft; } }
    public bool IsGoldsTurn { get { return _board.IsGoldsTurn; } }
    public bool EndOfMove { get { return _board.EndOfMove; } }
    public bool MovingBackwards { get; private set; }
    public bool IsEditing { get { return _board.IsEditing; } }
    public bool IsGrowing { get; set; }
    public bool IsShrinking { get; set; }
    public bool IsUpdating { get; set; }
    private string SaveLabel
    {
        get
        {
            //if (_savedAsset.HasLoadedGame && _board.ContainsSavedGame(_savedAsset.Current)) { return _savedAsset.FileName; }
            //else { return $"{DateTime.Now.Year.ToString().Substring(2)}{DateTime.Now.Month:00}{DateTime.Now.Day:00}_{DateTime.Now.Hour:00}{DateTime.Now.Minute:00}_{((IsGoldComputer) ? $"L{GoldLevel}" : "H")}v{((IsSilverComputer) ? $"L{SilverLevel}" : "H")}_{Turn}{((IsGoldsTurn) ? "g" : "s")}"; }
            return $"{DateTime.Now.Year.ToString().Substring(2)}{DateTime.Now.Month:00}{DateTime.Now.Day:00}_{DateTime.Now.Hour:00}{DateTime.Now.Minute:00}_{((IsGoldComputer) ? $"L{GoldLevel}" : "H")}v{((IsSilverComputer) ? $"L{SilverLevel}" : "H")}_{Turn}{((IsGoldsTurn) ? "g" : "s")}";
        }
    }
    
    public bool IsHvH { get { return _settings.Current.GoldMode == 0 && _settings.Current.SilverMode == 0; } }
    public bool IsHvC { get { return _settings.Current.GoldMode == 0 && _settings.Current.SilverMode != 0; } }
    public bool IsCvH { get { return _settings.Current.GoldMode != 0 && _settings.Current.SilverMode == 0; } }
    public bool IsCvC { get { return _settings.Current.GoldMode != 0 && _settings.Current.SilverMode != 0; } }

    private void Awake()
    {
        _board = (_savedAsset.HasLoadedGame) ? new Board(_savedAsset.Current) : new Board();
        _pieces = new List<PieceController>();
        _goldDeadPieces = new List<PieceController>();
        _silverDeadPieces = new List<PieceController>();
        _legalMoves = new List<Vector2Int>();
        _moves = new List<string>();

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
            foreach (PieceController piece in _pieces) { if (piece.IsUpdating) { return; } }
            _undoUpdate?.Invoke();
            IsUpdating = false;
            _autoUpdate?.Invoke();
        };
    }
    private void Start()
    {
        _settings.UpdateFlipSetting();

        if (_savedAsset.HasRecordedGame || _savedAsset.HasVariantGame) { LoadRecordedGame(); }

        UpdateTurn();
        ActivatePieces();
        foreach (PieceController piece in _pieces) { piece.Animate = true; }

        if (_savedAsset.HasVariantGame) 
        {
            if (IsUpdating) { _autoUpdate = UpdateTurn; }
            else { UpdateTurn(); }
        }
        else if (_savedAsset.HasRecordedGame) 
        {
            _settings.SetHvH();
            LoadGameInit();
            return;
        }
        else if (_savedAsset.HasLoadedGame) 
        {
            _settings.SetHvH();
            if (IsUpdating) { _autoUpdate = LoadGameInit; }
            else { LoadGameInit(); }
            return;
        }


        if (IsComputerPlay) { CheckAutoMove(); }
        else
        {
            if (Turn == 1 && IsGoldsTurn) { _board.SetGoldPieces(); }
            UpdateTurn();
        }
    }
    private void OnEnable()
    {
        _settings.OnSave.AddListener(UpdateSprite);
        _settings.OnLoad.AddListener(UpdateSprite);
    }
    private void OnDisable()
    {
        _settings.OnSave.RemoveListener(UpdateSprite);
        _settings.OnLoad.RemoveListener(UpdateSprite);
    }

    private void LoadGameInit() 
    {
        _board.SetFirstLoadMove();

        UpdateTurn();
        if (IsUpdating) { _autoUpdate = _pauseEvent.Play; }
        else { _pauseEvent.Play(); }
    }
    public void SaveGame() 
    {
        string filename = _savefileNameInput.text.Trim();
        if (filename.Length == 0 ) { filename = $"{DateTime.Now.Year}{DateTime.Now.Month}{DateTime.Now.Day}_GoldVsSilver"; }
        _savedAsset.Save(_board.History, filename); 
    }
    public void SaveQuitGame()
    {
        string filename = _saveQuitfileNameInput.text.Trim();
        if (filename.Length == 0) { filename = $"{DateTime.Now.Year}{DateTime.Now.Month}{DateTime.Now.Day}_GoldVsSilver"; }
        _savedAsset.Save(_board.History, filename);
    }
    
    public void SaveRules() { _savedAsset.Save(_board.History, "Rules"); }
    public void SetRules() { _board.UpdateRulesHistory(); }

    public void Pause() 
    {
        _board.Pause();
        _autoMoving = false;
        InterruptBot();
        _cancelationToken?.Cancel();
        _onChangeTurn?.Invoke();

        if (!_board.CanRedo && StepsLeft != 4) 
        {
            foreach (PieceController piece in _pieces) { piece.Animate = false; }
            Undo();
            ForgetHistory();
            foreach (PieceController piece in _pieces) { piece.Animate = true; }
        }

        UpdateTurn();
    }
    public void Unpause()
    {
        CheckWinState(false);
        if (_board.GameEnded) { return; }

        _board.Unpause();
        _autoMoving = true;
        UpdateTurn();
        CheckAutoMove();
    }

    public void UpdateSprite() 
    {
        _renderer.sprite = _spritesAsset.Boards[_settings.Current.Board];
        IsGoldComputer = _settings.Current.GoldMode == 1;
        IsSilverComputer = _settings.Current.SilverMode == 1;
        GoldLevel = _settings.Current.GoldLevel;
        SilverLevel = _settings.Current.SilverLevel;
        _board.IsGoldComputer = IsGoldComputer;
        _board.IsSilverComputer = IsSilverComputer;

        UpdateTurn();
    }
    public void UpdateTurn() 
    {
        if (_loadingRecords) { return; }
        _board.UpdatePiecesActivation();

        foreach (PieceController piece in _pieces) 
        {
            if (Turn == 1 && piece.IsTrapped) { piece.Animate = false; }
            piece.UpdatePieceState();
            if (Turn == 1) { piece.Animate = true; }
        }
        PositionDeadPieces();

        _undoButton.interactable = (IsEditing) ? _board.CanUndoEdit : _board.CanUndo;
        _redoButton.interactable = (IsEditing) ? _board.CanRedoEdit : _board.CanRedo;
        _sendButton.interactable = _board.CanMakeStep && !IsComputerPlay && !_board.GameEnded && !_board.Pushing && !_board.IsCurrentStateRepeting;
        _undoVeil.SetActive(_undoCount > 1);

        if (StepsLeft == 3 && _board.IsCurrentStateRepeting && _board.CannotMoveAnyPiece()) { CheckWinState(true); return; }

        _onTurnUpdate?.Invoke();

        if (_settings.Current == null) { return; }
        _onChangeTurn?.Invoke();

        if (IsGoldsTurn) { _onGoldTurn?.Invoke(); }
        else { _onSilverTurn?.Invoke(); }

        if (IsEditing) 
        {
            SetStateNormal();
            foreach (PieceController piece in _pieces) { piece.UpdateSpriteFlip(); }
        }
        else if (!_board.CanRedo) 
        {
            if (IsHvH)
            {
                if (_settings.Current.Flip && !IsGoldsTurn) { SetStateFliped(); }
                else { SetStateNormal(); }
            }
            else if (IsCvH) { SetStateFliped(); }
            else { SetStateNormal(); }

            foreach (PieceController piece in _pieces) { piece.UpdateSpriteFlip(); }
        }    

        IsGrowing = false;
        IsShrinking = false;
    }
    private void SetStateNormal() 
    {
        _renderer.flipX = false;
        _renderer.flipY = false;
        _onScreenNormal?.Invoke();
    }
    private void SetStateFliped() 
    {
        _renderer.flipX = true;
        _renderer.flipY = true;
        _onScreenFlip?.Invoke();
    }
    private void CheckWinState(bool isReapiting)
    {
        string moves = "";
#if UNITY_EDITOR
        StringBuilder move = new StringBuilder(255);
        MoveBot(_board.Moves, 0, move, move.Capacity);
        moves = move.ToString();
#elif PLATFORM_ANDROID
        moves = Marshal.PtrToStringAnsi(MoveBot2(_board.Moves, 0));
#elif PLATFORM_IOS
        moves = Marshal.PtrToStringAnsi(MoveBot2(_board.Moves, 0));
#endif
        _board.CheckWinState(moves.Trim() != "", isReapiting);
        if (_board.GameEnded) 
        {
            _onGameEnd?.Invoke();
            _pauseEvent.Play();
            UpdateTurn();
        }
    }
    private void InsanciateNewPiece(Piece piece) 
    {
        PieceController pieceController = Instantiate(_piecePrefab, transform).GetComponent<PieceController>();
        pieceController.Init(
            piece, 
            _settings, 
            this, 
            ((piece.IsGold) ? _goldDeadPieceParent : _silverDeadPieceParent), 
            ((piece.IsGold) ? _goldDeadPieces : _silverDeadPieces), 
            () => { _onMoveMake?.Invoke(); }, 
            () => { _onPieceTrapped?.Invoke(); }
            );
        _pieces.Add(pieceController);
    }

    public void PositionDeadPieces() 
    {
        for (int i = 0; i < _goldDeadPieces.Count; i++) 
        {
            _goldDeadPieces[i].Unfreeze();
            _goldDeadPieces[i].transform.localPosition = Vector3.right * 0.5f * i;
        }
        for (int i = 0; i < _silverDeadPieces.Count; i++) 
        {
            _silverDeadPieces[i].Unfreeze();
            _silverDeadPieces[i].transform.localPosition = Vector3.right * 0.5f * i;
        }
    }
    public void ActivatePieces()
    {
        _board.ActivatePieces();
        UpdateTurn();
    }
    public void DeactivatePieces()
    {
        _board.DeactivatePieces();
        UpdateTurn();
    }

    public bool IsPositionLegal(Vector2 position)
    {
        Vector2Int intPosition = new Vector2Int(Mathf.FloorToInt(position.x), Mathf.FloorToInt(position.y));
        if (Turn == 1 || _board.IsEditing) { return _legalMoves.Contains(intPosition) || !_board.RangeCheck(intPosition); }
        else { return _legalMoves.Contains(intPosition); }
    }
    public void ShowLegalMoves(Vector2Int origin)
    {
        _pathOrigin = origin;
        _legalMoves = _board.GetLegalMoves(origin);

        if (IsEditing) { return; }
        foreach (Vector2Int move in _legalMoves) { _legalMovesGrid[move].SetActive(true); }
    }
    public void UpdatePath(Vector2Int target) 
    {
        if(_pathLastTarget != target && _board.RangeCheck(target)) 
        {
            foreach (Vector2Int move in _legalMoves) { _pathMovesGrid[move].SetActive(false);  }
            _pathLastTarget = target;
            _path = _board.GetPath(_pathOrigin, target, _legalMoves);
            if (IsEditing) { return; }
            if (_path != null) { foreach (Vector2Int move in _path) { _pathMovesGrid[move].SetActive(true); } }
        }
    }
    public void HideLegalMoves() 
    {
        _pathLastTarget = Vector2Int.one * -1;
        _legalMovesGrid.Do((x, y, marker) => { marker.SetActive(false); });
        _pathMovesGrid.Do((x, y, marker) => { marker.SetActive(false); });

        _pathMovesGrid[_pathOrigin].SetActive(false);
    }
    public void MakeMove(PieceController piece, Vector2 target)
    {
        Vector2Int intTarget = new Vector2Int(Mathf.FloorToInt(target.x), Mathf.FloorToInt(target.y));
        if (!IsEditing) { _board.MakeMove(piece.Position, intTarget); }
        else { _board.MakeEditMove(piece.Piece, intTarget); }
        _undoCount = 0;
        UpdateTurn();
    }
    public void EndTurn() 
    {
        _board.EndTurn();
        UpdateTurn();
        CheckAutoMove();
        CheckWinState(false);
    }
    public void ForgetHistory() 
    {
        _board.ForgetHistory();
        _undoCount = 0;
        UpdateTurn();
        CheckAutoMove();
    }
    public void GoBack()
    {
        _board.GoBackToHistory();
        _undoCount = 0;
        UpdateTurn();
        CheckAutoMove();
    }
    public void Undo() 
    {
        if (IsEditing) 
        {
            _board.Undo();
            MovingBackwards = true;
            UpdateTurn();
            MovingBackwards = false;
            return; 
        }

        InterruptBot();
        if (!_board.CanUndo) { return; }

        _board.Undo();

        if (EndOfMove) { _undoCount++; }

        if (_undoCount > 1) { _board.DeactivatePieces(); }

        MovingBackwards = true;
        UpdateTurn();
        MovingBackwards = false;

        if (EndOfMove) { _undoUpdate = null; }
        else if (IsUpdating) { _undoUpdate = Undo; }
        else { Undo(); }
    }
    public void Redo() 
    {
        if (IsEditing)
        {
            _board.Redo();
            UpdateTurn();
            return;
        }

        if (EndOfMove) { _undoCount--; }

        if (!_board.CanRedo) { return; }
        _board.Redo();

        UpdateTurn();

        if (!_board.CanRedo) { _onRedoMax?.Invoke(); }

        if (EndOfMove) { _undoUpdate = (!_board.CanRedo) ? CheckAutoMove : null; }
        else if (IsUpdating) { _undoUpdate = Redo; }
        else { Redo(); }
    }
    public void FlipTurn() 
    {
        _board.FlipTurn();
        UpdateTurn();
    }
    public void EnableEditing() 
    {
        _board.EnableEditing();
        UpdateTurn();
    }
    public void DisableEditing()
    {
        _board.DisableEditing();
        UpdateTurn();
    }

    public void SetRecordsLabel(LabelController label) { label.SetLabel(_board.Moves); }
    public void SetInfoLabel(LabelController label) { label.SetLabel(_board.InfoLabel); }
    public void SetTurnLabel(LabelController label) { label.SetLabel(_board.TurnLabel); }
    public void SetStepsLabel(LabelController label) { label.SetLabel((IsComputerPlay) ? ((_calculatingMove) ? "Thinking..." : "Moving") : _board.StepsLabel); }
    public void SetUndoLabel(LabelController label) { label.SetLabel(_board.UndoLabel); }
    public void SetGameEndLabel(LabelController label) { label.SetLabel(_board.GameEndLabel); }
    public void SetDefaultSaveLabel() { _savefileNameInput.text = SaveLabel; }
    public void SetDefaultSaveLabel(TMP_InputField label) { label.text = SaveLabel; }
    public void SetGoldLabel(LabelController label) { label.SetLabel(((IsGoldComputer) ? $"Computer (L{GoldLevel})" : "Gold")); }
    public void SetSilverLabel(LabelController label) { label.SetLabel(((IsSilverComputer) ? $"Computer (L{SilverLevel})" : "Silver")); }
    public void CopyRecordsToClipboard() { GUIUtility.systemCopyBuffer = _board.Moves; }

    private Vector2Int GetPosition(string record)
    {
        int x = 0;
        int y = int.Parse(record[1].ToString()) - 1;

        if (record[0] == 'a') { x = 0; }
        else if (record[0] == 'b') { x = 1; }
        else if (record[0] == 'c') { x = 2; }
        else if (record[0] == 'd') { x = 3; }
        else if (record[0] == 'e') { x = 4; }
        else if (record[0] == 'f') { x = 5; }
        else if (record[0] == 'g') { x = 6; }
        else if (record[0] == 'h') { x = 7; }

        return new Vector2Int(x, y);
    }

#if UNITY_EDITOR
    [DllImport("ArimaEngine")]
    private static extern void InitBot();
    [DllImport("ArimaEngine")]
    private static extern void InterruptBot();
    [DllImport("ArimaEngine")]
    private static extern string MoveBot(string state, int difficulty, StringBuilder move, int length);
    [DllImport("libArimaaEngine")]
    private static extern IntPtr MoveBot2(string state, int difficulty);

#elif PLATFORM_ANDROID
    [DllImport("libArimaaEngine")]
    private static extern void InitBot();
    [DllImport("libArimaaEngine")]
    private static extern void InterruptBot();
    [DllImport("libArimaaEngine")]
    private static extern string MoveBot(string state, int difficulty, StringBuilder move, int length);
    [DllImport("libArimaaEngine")]
    private static extern IntPtr MoveBot2(string state, int difficulty);
#elif PLATFORM_IOS
    [DllImport("__Internal")]
    private static extern void InitBot();
    [DllImport("__Internal")]
    private static extern void InterruptBot();
    [DllImport("__Internal")]
    private static extern string MoveBot(string state, int difficulty, StringBuilder move, int length);
    [DllImport("__Internal")]
    private static extern IntPtr MoveBot2(string state, int difficulty);
#endif

    private void CheckAutoMove() 
    {
        if (_loadingRecords) { return; }

        if (IsComputerPlay && !_board.IsPaused && !_board.GameEnded)
        {
            if (_moves.Count > 0 || _autoUpdate != null) { AutoMove(); }
            else if (IsUpdating) { _autoUpdate = () => { CalculateMove(); }; }
            else { CalculateMove(); }
        }
        else { _autoUpdate = null; }
    }
    private async void CalculateMove()
    {
        _calculatingMove = true;
        _onChangeTurn?.Invoke();
        await Task.Delay(0);
        int difficulty = (IsGoldsTurn) ? GoldLevel : SilverLevel;
        _cancelationToken = new CancellationTokenSource();

#if UNITY_EDITOR
        StringBuilder move = new StringBuilder(255);
        await Task.Run(() => { InterruptBot(); }, _cancelationToken.Token);
        await Task.Run(() => { MoveBot(_board.Moves, difficulty, move, move.Capacity); }, _cancelationToken.Token);
        _moves = move.ToString().Split(" ").ToList();
#elif PLATFORM_ANDROID
        string moves = "";
        await Task.Run(() => { InterruptBot(); }, _cancelationToken.Token);
        await Task.Run(() => { moves = Marshal.PtrToStringAnsi(MoveBot2(_board.Moves, difficulty)); }, _cancelationToken.Token);
        _moves = moves.ToString().Split(" ").ToList();
#elif PLATFORM_IOS
        string moves = "";
        await Task.Run(() => { InterruptBot(); }, _cancelationToken.Token);
        await Task.Run(() => { moves = Marshal.PtrToStringAnsi(MoveBot2(_board.Moves, difficulty)); }, _cancelationToken.Token);
        _moves = moves.ToString().Split(" ").ToList();
#endif

        _calculatingMove = false;
        _onChangeTurn?.Invoke();
        await Task.Delay(0);

        if (!_cancelationToken.Token.IsCancellationRequested && !_board.GameEnded) { AutoMove(); }
        else { _moves.Clear(); }
    }
    private void AutoMove() 
    {    
        _autoMoving = true;
        if (Turn == 1) { MakeFirstAutoMove(); }
        else { MakeAutoMove(); }
    }
    private void MakeAutoMove()
    {
        if (IsUpdating) { _autoUpdate = MakeAutoMove; return; }
        if (!_autoMoving) { return; }
        if (_moves.Count == 0) { EndAutoMove(); return; }

        string move;
        string action;
        Vector2Int direction = Vector2Int.zero;
        Vector2Int position;

        move = _moves[0].Trim();
        _moves.RemoveAt(0);

        if (move == "pass" || move == "" || move.Length < 4) { EndAutoMove(); return; }

        action = move[3].ToString();
        if (action == "x") { MakeAutoMove(); return; }

        position = GetPosition(move.Substring(1, 2));
        if (action == "n") { direction = Vector2Int.up; }
        else if (action == "s") { direction = Vector2Int.down; }
        else if (action == "e") { direction = Vector2Int.right; }
        else if (action == "w") { direction = Vector2Int.left; }
        else { print($"Move not recognized: {move}"); }

        _board.MakeMove(position, position + direction);
        UpdateTurn();

        if (IsUpdating) { _autoUpdate = MakeAutoMove; }
        else { MakeAutoMove(); }
    }
    private void MakeFirstAutoMove()
    {
        if(!_autoMoving) { return; }
        _board.SetPiecesFromString(_moves, IsGoldsTurn); 
        UpdateTurn();
        if (IsUpdating) { _autoUpdate = EndAutoMove; }
        else { EndAutoMove(); }        
    }
    private void EndAutoMove() 
    {
        _autoMoving = false;
        _autoUpdate = null; 
        _moves.Clear(); 
        EndTurn();
    }

    private void StaticAutoMove()
    {
        if (Turn != 1) { StaticMakeAutoMove(); }
        else { StaticMakeFirstAutoMove(); }
    }
    private void StaticMakeAutoMove()
    {
        if (_moves.Count == 0) { StaticEndAutoMove(); return; }

        string move;
        string action;
        Vector2Int direction = Vector2Int.zero;
        Vector2Int position;

        move = _moves[0].Trim();
        _moves.RemoveAt(0);

        if (move == "pass" || move == "" || move.Length < 4) { StaticEndAutoMove(); return; }

        action = move[3].ToString();
        if (action == "x") { StaticMakeAutoMove(); return; }

        position = GetPosition(move.Substring(1, 2));
        if (action == "n") { direction = Vector2Int.up; }
        else if (action == "s") { direction = Vector2Int.down; }
        else if (action == "e") { direction = Vector2Int.right; }
        else if (action == "w") { direction = Vector2Int.left; }
        else { print($"Move not recognized: {move}"); }

        _board.MakeMove(position, position + direction);
        StaticMakeAutoMove();
    }
    private void StaticMakeFirstAutoMove()
    {
        _board.SetPiecesFromString(_moves, IsGoldsTurn);
        StaticEndAutoMove();
    }
    private void StaticEndAutoMove()
    {
        _moves.Clear();
        _board.EndTurn(false);
    }
    private void LoadRecordedGame() 
    {
        _loadingRecords = true;
        string[] turns = _savedAsset.GetRecordedGame().Split("\n");
        for (int i = 0; i < turns.Length; i++) 
        {
            _moves = turns[i].Split(" ").ToList();
            _moves.RemoveAt(0);
            StaticAutoMove();
        }

        //for (int i = 0; i < _board.History.Count; i++) { print($"{i}: {_board.History[i].Moves}"); }
        
        if(_board.History.Count > 2) { _board.History.RemoveAt(_board.History.Count - 3); }
        
        foreach (PieceController piece in _pieces) { piece.UpdatePiece(); }
        _loadingRecords = false;
    }
}
