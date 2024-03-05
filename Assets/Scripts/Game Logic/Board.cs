using System.Collections.Generic;
using UnityEngine;

public class Board
{
    private BoardState _state;
    private List<BoardState> _history;
    private List<BoardState> _editHistory;
    private List<BoardState> _rulesHistory;

    public List<BoardState> History { get { return _history; } }
    public List<Piece> Pieces { get { return _state.Pieces; } }
    public int Turn { get { return _state.Turn; } private set { _state.Turn = value; } }
    public bool IsGoldsTurn { get { return _state.IsGoldsTurn; } private set { _state.IsGoldsTurn = value; } }
    public int StepsLeft { get { return _state.StepsLeft; } private set { _state.StepsLeft = value; } }
    public bool Pushing { get { return _state.Pushing; } private set { _state.Pushing = value; } }
    public bool Pulling { get { return _state.Pulling; } private set { _state.Pulling = value; } }
    public bool EndOfMove { get { return _state.EndOfTurn; } private set { _state.EndOfTurn = value; } }
    public string Moves { get { return _state.Moves; } private set { _state.Moves = value; } }
    public string State { get { return _state.State; } private set { _state.State = value; } }
    public Piece LastPiece { get { return _state.LastPiece; } private set { _state.LastPiece = value; } }
    public Vector2Int LastPosition { get { return _state.LastPosition; } private set { _state.LastPosition = value; } } 
    public bool GameEnded { get { return _state.GameEnded; } set { _state.GameEnded = value; } }
    public bool IsGoldWiner { get { return _state.IsGoldWiner; } set { _state.IsGoldWiner = value; } }
    public bool WinByElimination { get { return _state.WinByElimination; } set { _state.WinByElimination = value; } }
    public bool WinByInmobilization { get { return _state.WinByInmobilization; } set { _state.WinByInmobilization = value; } }
    public bool WinByRepetition { get { return _state.WinByRepetition; } set { _state.WinByRepetition = value; } }
    public bool CanUndo { get { return _undoIndex != _history.Count - 1; } }
    public bool CanRedo { get { return _undoIndex != 0; } }
    public bool CanMakeStep { get { return !(StepsLeft == 4 && Turn > 1); } }
    public bool IsPaused { get { return _pause; } }

    public bool IsEditing { get { return _editing; } }
    public bool CanUndoEdit { get { return _editUndoIndex != _editHistory.Count - 1; } }
    public bool CanRedoEdit { get { return _editUndoIndex != 0; } }

    public bool IsGoldComputer { get; set; }
    public bool IsSilverComputer { get; set; }
    public bool IsCurrentStateRepeting
    {
        get
        {
            string[] states = State.Trim().Split("\n");
            if (states.Length >= 2 && states[0].Trim() == CurrentFullState.Trim()) { return true; }
            if (states.Length >= 4)
            {
                string firstState = states[3].Trim();
                string secondState = states[1].Trim();
                string thirdState = CurrentFullState.Trim();

                if (firstState == secondState && firstState == thirdState) { return true; }
            }
            return false;
        }
    }
    public bool IsStateRepeting
    {
        get
        {
            string[] states = State.Trim().Split("\n");
            if (states.Length >= 5)
            {
                string firstState = states[4].Trim();
                string secondState = states[2].Trim();
                string thirdState = states[0].Trim();

                if (firstState == secondState && firstState == thirdState) { return true; }
            }
            return false;
        }
    }

    public int UndoIndex { get { return _undoIndex; } }
    
    public string InfoLabel
    {
        get
        {
            if (_pause) { return "Viewing Game"; }
            else if (Turn == 1) { return ((IsGoldsTurn) ? "Gold's Setup" : "Silver's Setup"); }
            else { return ((IsGoldsTurn) ? "Gold's Turn" : "Silver's Turn"); }
        }
    }
    public string TurnLabel
    {
        get
        {
            if (_pause) 
            {
                if (_editing && _editUndoIndex != _editHistory.Count - 1) { return $"Turn: {Turn}{((IsGoldsTurn) ? "g" : "s")}/{_editHistory[0].Turn}{((_editHistory[0].IsGoldsTurn) ? "g" : "s")}"; }
                else { return $"Turn: {Turn}{((IsGoldsTurn) ? "g" : "s")}/{_history[0].Turn}{((_history[0].IsGoldsTurn) ? "g" : "s")}"; }
            }
            else { return $"Turn: {Turn}{((IsGoldsTurn) ? "g" : "s")}"; }
        }
    }
    public string StepsLabel { get { return (Turn == 1) ? $"Drag to swap pieces": $"Steps Left: {StepsLeft}"; } }
    public string GameEndLabel 
    {
        get 
        {
            if (WinByInmobilization) { return $"{((IsGoldWiner) ? "Gold" : "Silver")} wins by immobilization"; }
            else if (WinByElimination) { return $"{((IsGoldWiner) ? "Gold" : "Silver")} wins by elimination"; }
            else if (WinByRepetition) { return $"{((IsGoldWiner) ? "Gold" : "Silver")} wins by repetition"; }
            else { return $"{((IsGoldWiner) ? "Gold" : "Silver")} wins by goal"; }
        } 
    }
    public string UndoLabel 
    {
        get 
        {
            if (_pause) { return $"Forget subsequent moves and play again from here?"; }
            else { return $"Make a different move for {((IsGoldsTurn) ? "Gold" : "Silver")}"; }
        } 
    }
    public string CurrentFullState
    {
        get
        {
            string goldState = "g";
            string silverState = "s";
            _grid.Do((x, y, piece) =>
            {
                if (piece == null) 
                {
                    goldState += "x";
                    silverState += "x"; 
                }
                else if (piece.IsGold) 
                {
                    goldState += piece.StateLabel; 
                    silverState += "x"; 
                }
                else 
                {
                    goldState += "x"; 
                    silverState += piece.StateLabel; 
                }
            });
            return goldState + silverState;
        }
    }

    private Grid<Piece> _grid;
    private PathFinding _pathFinding;

    private int _undoIndex = 0;
    private int _editUndoIndex = 0;
    private bool _editing = false;
    private bool _active = true;
    private bool _pause = false;

    public bool RangeCheck(Vector2Int position) { return _grid.RangeCheck(position); }

    public Board(List<BoardState> history, bool setFirstState = false)
    {
        Initialize();
        _history.Clear();
        _history.AddRange(history);
        if (setFirstState) { SetFirstState(); } 
        else { GoBackToHistory(); }
        UpdatePiecesActivation();
    }
    public Board() 
    {
        Initialize();
        SetPieces();
        EndOfMove = true;
        UpdatePiecesActivation();
        UpdateUndoHistory();
    }
    private void Initialize()
    {
        _state = new BoardState();
        _history = new List<BoardState>();
        _editHistory = new List<BoardState>();
        _rulesHistory = new List<BoardState>();
        _grid = new Grid<Piece>(8, 8, (x, y, grid) => { return null; });
        _pathFinding = new PathFinding(8, 8);

        int strength = 1;
        bool isGold = true;

        for (int i = 0; i < 32; i++)
        {
            if (i == 0) { strength = 1; }
            else if (i == 8) { strength = 2; }
            else if (i == 10) { strength = 3; }
            else if (i == 12) { strength = 4; }
            else if (i == 14) { strength = 5; }
            else if (i == 15) { strength = 6; }
            else if (i == 16) { strength = 1; isGold = false; }
            else if (i == 24) { strength = 2; }
            else if (i == 26) { strength = 3; }
            else if (i == 28) { strength = 4; }
            else if (i == 30) { strength = 5; }
            else if (i == 31) { strength = 6; }

            _state.Pieces.Add(new Piece(strength, isGold, true));
        }
    }
    private void SetPieces()
    {
        Pieces[0].MoveTo(new Vector2Int(0, 0));
        Pieces[1].MoveTo(new Vector2Int(1, 0));
        Pieces[2].MoveTo(new Vector2Int(2, 0));

        Pieces[3].MoveTo(new Vector2Int(0, 1));
        Pieces[4].MoveTo(new Vector2Int(7, 1));

        Pieces[5].MoveTo(new Vector2Int(5, 0));
        Pieces[6].MoveTo(new Vector2Int(6, 0));
        Pieces[7].MoveTo(new Vector2Int(7, 0));

        Pieces[8].MoveTo(new Vector2Int(2, 1));
        Pieces[9].MoveTo(new Vector2Int(5, 1));

        Pieces[10].MoveTo(new Vector2Int(3, 0));
        Pieces[11].MoveTo(new Vector2Int(4, 0));

        Pieces[12].MoveTo(new Vector2Int(1, 1));
        Pieces[13].MoveTo(new Vector2Int(6, 1));

        Pieces[14].MoveTo(new Vector2Int(3, 1));
        Pieces[15].MoveTo(new Vector2Int(4, 1));

        Pieces[16].MoveTo(new Vector2Int(0, 7));
        Pieces[17].MoveTo(new Vector2Int(1, 7));
        Pieces[18].MoveTo(new Vector2Int(2, 7));

        Pieces[19].MoveTo(new Vector2Int(0, 6));
        Pieces[20].MoveTo(new Vector2Int(7, 6));

        Pieces[21].MoveTo(new Vector2Int(5, 7));
        Pieces[22].MoveTo(new Vector2Int(6, 7));
        Pieces[23].MoveTo(new Vector2Int(7, 7));

        Pieces[24].MoveTo(new Vector2Int(2, 6));
        Pieces[25].MoveTo(new Vector2Int(5, 6));

        Pieces[26].MoveTo(new Vector2Int(3, 7));
        Pieces[27].MoveTo(new Vector2Int(4, 7));

        Pieces[28].MoveTo(new Vector2Int(1, 6));
        Pieces[29].MoveTo(new Vector2Int(6, 6));

        Pieces[30].MoveTo(new Vector2Int(4, 6));
        Pieces[31].MoveTo(new Vector2Int(3, 6));

        foreach (Piece piece in _state.Pieces) 
        {
            piece.MoveTo(piece.Position);
            piece.UpdateTrapPosition();
            piece.IsTrapped = true;
            _grid.SetItem(piece.Position, null);
        }
    }
    public void SetGoldPieces() 
    {
        foreach (Piece piece in _state.Pieces) 
        {
            if (!piece.IsGold) { continue; }
            piece.IsTrapped = false;
            _grid.SetItem(piece.Position, piece);
            piece.MoveTo(piece.Position);
        }
        EndOfMove = true;
        UpdatePiecesActivation();
        UpdateUndoHistory();
    }
    private void SetSilverPieces()
    {
        foreach (Piece piece in _state.Pieces)
        {
            if (piece.IsGold) { continue; }
            piece.IsTrapped = false;
            _grid.SetItem(piece.Position, piece);
            piece.MoveTo(piece.Position);
        }
        EndOfMove = true;
        UpdatePiecesActivation();
        UpdateUndoHistory();
    }
    public void SetPiecesFromString(List<string> initialPlacements, bool isGold)
    {
        int offset = (isGold) ? 0 : 16;
        int rabbitCount = 0;
        int catCount = 0;
        int dogCount = 0;
        int horseCount = 0;
        int camelCount = 0;
        int elephantCount = 0;

        foreach (Piece boardPiece in _state.Pieces)
        {
            if(boardPiece.IsGold != isGold) { continue; }
            boardPiece.IsTrapped = true;
            _grid.SetItem(boardPiece.Position, null);
        }

        Piece piece = null;
        Vector2Int position;

        foreach (string placement in initialPlacements)
        {
            if (placement.Length == 0) {continue; }

            if (placement.ToLower()[0] == 'r') { if (rabbitCount >= 8) { continue; } piece = Pieces[rabbitCount + offset]; rabbitCount++; }
            if (placement.ToLower()[0] == 'c') { if (catCount >= 2) { continue; } piece = Pieces[catCount + 8 + offset]; catCount++; }
            if (placement.ToLower()[0] == 'd') { if (dogCount >= 2) { continue; } piece = piece = Pieces[dogCount + 10 + offset]; dogCount++; }
            if (placement.ToLower()[0] == 'h') { if (horseCount >= 2) { continue; } piece = piece = Pieces[horseCount + 12 + offset]; horseCount++; }
            if (placement.ToLower()[0] == 'm') { if (camelCount >= 1) { continue; } piece = piece = Pieces[camelCount + 14 + offset]; camelCount++; }
            if (placement.ToLower()[0] == 'e') { if (elephantCount >= 1) { continue; } piece = piece = Pieces[elephantCount + 15 + offset]; elephantCount++; }

            if(piece == null) { continue; }
            position = GetPosition(placement.Substring(1,2));
            piece.IsTrapped = false;
            piece.MoveTo(position);
            _grid.SetItem(piece.Position, piece);
            piece = null;
        }

        EndOfMove = true;
        UpdatePiecesActivation();
        UpdateUndoHistory();
    }

    public void Pause() { _pause = true; }
    public void Unpause() { _pause = false; }

    private void UpdatePiecesToGrid() 
    {
        _grid.Do((x, y, piece) => { _grid.SetItem(x, y, null); });

        foreach (Piece piece in _state.Pieces) 
        {
            if (piece.IsTrapped) { continue; }
            _grid.SetItem(piece.Position, piece);
        }
    } 
    private void UpdateUndoHistory() { _history.Insert(0, _state.Copy()); }
    public void ForgetHistory() 
    {
        if (_undoIndex == 0) { return; }
        while(_undoIndex > 0) 
        {
            _history.RemoveAt(0);
            _undoIndex--;
        }
        if(EndOfMove) 
        {
            EndOfMove = false;
            UpdateUndoHistory();
        }
    }
    public void GoBackToHistory() 
    {
        _undoIndex = 0;
        _state.SetState(_history[_undoIndex]);
        UpdatePiecesToGrid();
    }
    public void Undo()
    {
        if (_editing) { UndoEdit(); return; }

        _undoIndex++;
        if (_undoIndex > _history.Count - 1) { _undoIndex = _history.Count - 1; return; }

        _state.SetState(_history[_undoIndex]);
        UpdatePiecesToGrid();
    }
    public void Redo()
    {
        if (_editing) { RedoEdit(); return; }

        _undoIndex--;
        if (_undoIndex < 0) { _undoIndex = 0; return; }

        _state.SetState(_history[_undoIndex]);
        UpdatePiecesToGrid();
        if (_undoIndex == 0) { EndOfMove = false; }
    }
    public void SetFirstLoadMove() 
    {
        int undoIndex = _undoIndex;
        string[] moves;

        for (int i = _history.Count - 1; i >= 0; i--)
        {
            moves = _history[i].Moves.Trim().Split("\n");
            if (moves.Length >= 2) 
            {
                undoIndex = i;
                break;
            }            
        }

        _undoIndex = undoIndex;
        _state.SetState(_history[undoIndex]);
        UpdatePiecesToGrid();
    }
    public void SetFirstState() 
    {
        _undoIndex = _history.Count - 1;
        _state.SetState(_history[_undoIndex]);
        UpdatePiecesToGrid();
    }

    public void UpdatePiecesActivation()
    {
        if (_editing)
        {
            foreach (Piece piece in Pieces) { piece.IsActive = true; }
            return;
        }
        else if (!_active) 
        {
            foreach (Piece piece in Pieces) { piece.IsActive = false; }
            return; 
        }

        if(GameEnded) 
        {
            foreach (Piece piece in Pieces) { piece.IsActive = false; }
            return; 
        }
        
        foreach (Piece piece in Pieces)
        {
            if (_editing) { piece.IsActive = true; }
            else if (_pause) { piece.IsActive = false; }
            else if ((IsGoldsTurn && IsGoldComputer) || (!IsGoldsTurn && IsSilverComputer)) { piece.IsActive = false; }
            else if (piece.IsTrapped) { piece.IsActive = false; }
            else if (Pushing)
            {
                if (IsPushing(piece)) { piece.IsActive = true; }
                else { piece.IsActive = false; }
            }
            else if (IsMovable(piece) || IsPushable(piece) || IsPullable(piece)) { piece.IsActive = true; }
            else { piece.IsActive = false; }
        }
    }
    public void ActivatePieces() 
    {
        _active = true;
        UpdatePiecesActivation();
    }
    public void DeactivatePieces() 
    {
        _active = false;
        foreach (Piece piece in Pieces) { piece.IsActive = false; } 
    }
    public List<Vector2Int> GetLegalMoves(Vector2Int origin) { return GenerateLegalMoves(origin.x, origin.y, StepsLeft); }
    public List<Vector2Int> GetPath(Vector2Int origin, Vector2Int target, List<Vector2Int> legalMoves) 
    {
        Piece piece = _grid[origin];
        if (piece == null) { return null; }

        List<Vector2Int> moves = new List<Vector2Int>(legalMoves);

        for (int i = moves.Count - 1; i >= 0; i--)
        {
            if (target != moves[i] && IsStopPosition(moves[i], piece)) 
            {
                moves.RemoveAt(i); 
            } 
        }

        _pathFinding.SetLegalMoves(origin, moves);
        return _pathFinding.FindPath(origin, target);
    }

    private void UpdateBoardStatus()
    {
        foreach (var piece in Pieces)
        {
            if (piece.IsTrapped) { continue; }
            if (!IsFriendlyPositionForPiece(piece.Position, piece) && IsPieceTrapped(piece)) { TrapPiece(piece); }
        }

        foreach (var piece in Pieces)
        {
            if (piece.IsTrapped) { continue; }
            piece.IsFrozen = false;
            if (!IsFriendlyPositionForPiece(piece.Position, piece) && IsPieceFrozen(piece)) { piece.IsFrozen = true; }
        }

        UpdatePiecesActivation();
    }
    public void CheckWinState(bool hasPosibleMove, bool isRepeating)
    {
        if (GameEnded) { return; }
        if (Turn == 1) { return; }

        bool lastPlayer = !IsGoldsTurn;
        WinByElimination = false;
        WinByInmobilization = false;
        WinByRepetition = false;

        // Check if a rabbit of player A reached goal. If so player A wins.
        foreach (Piece piece in Pieces)
        {
            if (!piece.IsRabbit) { continue; }
            if (piece.IsGold != lastPlayer) { continue; }

            if (piece.IsGold && piece.Position.y == 7)
            {
                GameEnded = true;
                IsGoldWiner = true;
            }
            else if (!piece.IsGold && piece.Position.y == 0)
            {
                GameEnded = true;
                IsGoldWiner = false;
            }
        }
        if (GameEnded) { UpdatePiecesActivation(); return; }

        // Check if a rabbit of player B reached goal. If so player B wins.
        foreach (Piece piece in Pieces)
        {
            if (!piece.IsRabbit) { continue; }
            if (piece.IsGold == lastPlayer) { continue; }

            if (piece.IsGold && piece.Position.y == 7)
            {
                GameEnded = true;
                IsGoldWiner = true;
            }
            else if (!piece.IsGold && piece.Position.y == 0)
            {
                GameEnded = true;
                IsGoldWiner = false;
            }
        }
        if (GameEnded) { UpdatePiecesActivation(); return; }


        // Check if player B lost all rabbits. If so player A wins.
        bool winner = true;
        foreach (Piece piece in Pieces)
        {
            if (!piece.IsRabbit) { continue; }
            if (piece.IsGold == lastPlayer) { continue; }

            if (piece.IsTrapped) { continue; }

            winner = false;
        }
        if (winner) 
        {
            GameEnded = true;
            IsGoldWiner = lastPlayer;
            WinByElimination = true;
        }
        if (GameEnded) { UpdatePiecesActivation(); return; }

        // Check if player A lost all rabbits. If so player B wins.
        winner = true;
        foreach (Piece piece in Pieces)
        {
            if (!piece.IsRabbit) { continue; }
            if (piece.IsGold != lastPlayer) { continue; }
            if (piece.IsTrapped) { continue; }
            
            winner = false;
        }
        if (winner)
        {
            GameEnded = true;
            IsGoldWiner = !lastPlayer;
            WinByElimination = true;
        }
        if (GameEnded) { UpdatePiecesActivation(); return; }

        // Check if player B has no possible move (all pieces are frozen or have no place to move). If so player A wins.
        winner = true;
        foreach (Piece piece in Pieces)
        {
            if (piece.IsGold == lastPlayer) { continue; }
            if (CannotPieceMove(piece)) { continue; }

            winner = false;
        }
        if (winner)
        {
            GameEnded = true;
            IsGoldWiner = lastPlayer;
            WinByInmobilization = true;
        }
        if (GameEnded) { UpdatePiecesActivation(); return; }

        if (!hasPosibleMove) 
        {
            GameEnded = true;
            IsGoldWiner = lastPlayer;
            WinByInmobilization = true;
        }
        if (GameEnded) { UpdatePiecesActivation(); return; }
        
        // Check if the only moves player B has are 3rd time repetitions. If so player A wins.
        if (isRepeating || IsStateRepeting)
        {
            GameEnded = true;
            IsGoldWiner = lastPlayer;
            WinByRepetition = true;
        }
        if (GameEnded) { UpdatePiecesActivation(); return; }

        UpdatePiecesActivation();
    }

    public void MakeMove(Vector2Int origin, Vector2Int target)
    {
        Piece piece = _grid[origin];
        if (piece == null) { return; }

        ForgetHistory();
        if (Turn == 1 && !_grid.RangeCheck(target))
        {
            piece.IsTrapped = true;
            UpdateBoardStatus();
            return;
        }

        List<Vector2Int> legalMoves = GetLegalMoves(origin);
        if (!legalMoves.Contains(target) || !_grid.RangeCheck(target)) { return; }

        List<Vector2Int> path = GetPath(origin, target, legalMoves);
        if (path == null || path.Count < 2) { return; }

        if (Turn == 1)
        {
            Move(piece, target);
            return;
        }

        if (piece.IsGold != IsGoldsTurn)
        {
            if (Pulling) 
            {
                Pulling = false;
                if (LastPosition != target) { Pushing = true; }
            }
            else { Pushing = true; }
        }
        else 
        {
            Pulling = CanPull(piece);
            Pushing = false;
        }

        LastPosition = piece.Position;
        LastPiece = piece;

        Vector2Int direction;
        for (int i = 1; i < path.Count; i++)
        {
            direction = path[i] - path[i - 1];
            if (direction == Vector2Int.up) { Moves += piece.PositionLabel + "n "; }
            else if (direction == Vector2Int.down) { Moves += piece.PositionLabel + "s "; }
            else if (direction == Vector2Int.right) { Moves += piece.PositionLabel + "e "; }
            else if (direction == Vector2Int.left) { Moves += piece.PositionLabel + "w "; }
            StepsLeft--;
            Move(piece, path[i]);
        }

        if (piece.IsTrapped) { Moves += piece.PositionLabel + "x "; }

        return;
    }
    public void EndTurn(bool setSilverPieces = true)
    {
        ForgetHistory();

        if (Turn > 1 && (StepsLeft == 4 || Pushing)) { return; }

        if (Turn == 1) 
        {
            foreach (Piece piece in Pieces) 
            {
                if (piece.IsGold == IsGoldsTurn && !piece.IsTrapped) 
                {
                    Moves += piece.PositionLabel + " "; 
                } 
            }  
        }
        else if(StepsLeft != 0) { Moves += "pass "; }

        State = CurrentFullState + "\n" + State;

        StepsLeft = 4;
        if (!IsGoldsTurn) { Turn++; }
        IsGoldsTurn = !IsGoldsTurn;
        EndOfMove = true;

        if (Turn == 1 && !IsGoldsTurn && setSilverPieces) { SetSilverPieces(); }
        Moves += $"\n{Turn}{((IsGoldsTurn) ? "g" : "s")} ";

        UpdatePiecesActivation();
        UpdateUndoHistory();
        EndOfMove = false;
    }

    public void Move(Piece piece, Vector2Int target)
    {
        _grid.SetItem(piece.Position, _grid[target]);
        _grid.SetItem(target, piece);

        _grid[piece.Position]?.MoveTo(piece.Position);
        piece.MoveTo(target);

        UpdateBoardStatus();        
        UpdateUndoHistory();
    }
    public void TrapPiece(Piece piece)
    {
        piece.IsTrapped = true;
        piece.UpdateTrapPosition();
        _grid.SetItem(piece.Position, null);
        UpdateBoardStatus();
    }

    public void EnableEditing()
    {
        _editing = true;
        ForgetHistory();
        UpdateEditUndoHistory();
        UpdatePiecesActivation();
    }
    public void DisableEditing()
    {
        _editing = false;
        if (_editUndoIndex != _editHistory.Count - 1) { UpdateUndoHistory(); }
        _editHistory.Clear();
        DeactivatePieces();
    }    
    public void FlipTurn() 
    {
        IsGoldsTurn = !IsGoldsTurn;

        GameEnded = false;
        WinByElimination = false;
        WinByInmobilization = false;
        WinByRepetition = false;

        UpdateEditMoves();
        UpdateEditUndoHistory();
    }
    public void MakeEditMove(Piece piece, Vector2Int target) 
    {
        bool isIllegal = IsTrapPositionForPiece(target, piece) || !_grid.RangeCheck(target);

        if (isIllegal && piece.IsTrapped) { return; }

        if (!piece.IsTrapped)
        {
            if (isIllegal) { TrapPiece(piece); }
            else { Move(piece, target); }
        }
        else 
        {
            Piece targetPiece = _grid[target];
            if (targetPiece != null) { TrapPiece(targetPiece); }
            piece.IsTrapped = false;
            Move(piece, target);
        }
        
        GameEnded = false;
        WinByElimination = false;
        WinByInmobilization = false;
        WinByRepetition = false;

        UpdateEditMoves();
        UpdateEditUndoHistory();
    }
    public void UpdateEditMoves()
    {
        Turn = 2;
        string goldSetup = "1g";
        string silverSetup = "1s";
        _grid.Do((x, y, piece) => 
        {
            if (piece != null) 
            {
                if (piece.IsGold) { goldSetup += $" {piece.PositionLabel}"; }
                else { silverSetup += $" {piece.PositionLabel}"; }
            }
        });
        if (goldSetup != "1g") { goldSetup += " "; }
        if (silverSetup != "1s") { silverSetup += " "; }

        Moves = goldSetup + "\n" + silverSetup + "\n" + ((IsGoldsTurn)? "2g ": "2g pass\n2s ");
    }
    private void UpdateEditUndoHistory() { _editHistory.Insert(0, _state.Copy()); }
    public void UndoEdit()
    {
        _editUndoIndex++;
        if (_editUndoIndex > _editHistory.Count - 1) { _editUndoIndex = _editHistory.Count - 1; return; }

        _state.SetState(_editHistory[_editUndoIndex]);
        UpdatePiecesToGrid();
    }
    public void RedoEdit()
    {
        _editUndoIndex--;
        if (_editUndoIndex < 0) { _editUndoIndex = 0; return; }

        _state.SetState(_editHistory[_editUndoIndex]);
        UpdatePiecesToGrid();
        if (_editUndoIndex == 0) { EndOfMove = false; }
    }

    public void UpdateRulesHistory() { _rulesHistory.Insert(0, _state.Copy()); }

    private List<Vector2Int> GenerateLegalMoves(int x, int y, int stepsLeft)
    {
        List<Vector2Int> legalMoves = new List<Vector2Int>();

        if (_editing)
        {
            Vector2Int position;
            _grid.Do((x, y, piece) => 
            {
                position = new Vector2Int(x, y);
                if (!legalMoves.Contains(position)) { legalMoves.Add(position); }
            });
            return legalMoves;
        }

        if (Turn == 1)
        {
            foreach (Piece p in Pieces) 
            {
                if (p.IsGold == IsGoldsTurn)
                {
                    if (!legalMoves.Contains(p.Position)) { legalMoves.Add(p.Position); }
                } 
            }
            return legalMoves;
        }

        if (StepsLeft <= 0) { return legalMoves; }

        Piece piece = _grid[x, y];
        if (piece == null) { return legalMoves; }

        if (Pushing) 
        {
            if (!legalMoves.Contains(LastPosition)) { legalMoves.Add(LastPosition); }
            return legalMoves; 
        }
        
        if (piece.IsGold != IsGoldsTurn)
        {
            if (Pulling && IsPullable(piece))
            {
                if (!legalMoves.Contains(LastPosition)) { legalMoves.Add(LastPosition); }
                return legalMoves; 
            }

            foreach (Vector2Int neightbour in _grid.GetAdjacentPositions(piece.Position))
            {
                if (_grid[neightbour] != null) { continue; }
                if (!legalMoves.Contains(neightbour)) { legalMoves.Add(neightbour); }
            }
            return legalMoves;
        }

        if (piece.IsFrozen || piece.IsTrapped) { return legalMoves; }

        MapLegalMoves(piece, piece.Position, Vector2Int.zero, stepsLeft, ref legalMoves);

        return legalMoves;
    }
    private void MapLegalMoves(Piece piece, Vector2Int position, Vector2Int direction, int stepsLeft, ref List<Vector2Int> legalMoves)
    {
        if (direction != Vector2Int.zero)
        {
            position += direction;
            if (!_grid.RangeCheck(position)) { return; }
            if (!IsEmptyPosition(position)) { return; }
            if (!legalMoves.Contains(position)) { legalMoves.Add(position); }
            if (IsStopPosition(position, piece)) { return; }
            stepsLeft--;
            if (stepsLeft == 0) { return; }
        }

        if (direction != Vector2Int.down && piece.CanMoveNorth) { MapLegalMoves(piece, position, Vector2Int.up, stepsLeft, ref legalMoves); }
        if (direction != Vector2Int.up && piece.CanMoveSouth) { MapLegalMoves(piece, position, Vector2Int.down, stepsLeft, ref legalMoves); }
        if (direction != Vector2Int.left) { MapLegalMoves(piece, position, Vector2Int.right, stepsLeft, ref legalMoves); }
        if (direction != Vector2Int.right) { MapLegalMoves(piece, position, Vector2Int.left, stepsLeft, ref legalMoves); }
        return;
    }

    public bool IsPieceTrapped(Piece piece) { return IsTrapPosition(piece.Position); }
    public bool IsPieceFrozen(Piece piece) { return IsFrozenPositionForPiece(piece.Position, piece); }
    private bool IsPushing(Piece piece)
    {
        if (piece.IsGold != IsGoldsTurn) { return false; }
        if (!piece.IsStronger(LastPiece)) { return false; }
        foreach (Vector2Int neightbour in _grid.GetAdjacentPositions(piece.Position)) { if (neightbour == LastPosition) { return true; } }
        return false;
    }
    private bool IsMovable(Piece piece) { return piece.IsGold == IsGoldsTurn && !piece.IsFrozen && StepsLeft > 0; }
    private bool IsPushable(Piece piece)
    {
        if (piece.IsGold == IsGoldsTurn) { return false; }
        if (StepsLeft <= 1) { return false; }

        foreach (Piece neightbour in _grid.GetAdjacent(piece.Position))
        {
            if (neightbour == null) { continue; }
            if (neightbour.IsStronger(piece) && !neightbour.IsFrozen) { return true; }
        }
        return false;
    }
    private bool IsPullable(Piece piece)
    {
        if (!Pulling) { return false; }
        if (piece.IsGold == IsGoldsTurn) { return false; }
        if (StepsLeft <= 0) { return false; }
        if (!LastPiece.IsStronger(piece)) { return false; }

        foreach (Vector2Int neightbour in _grid.GetAdjacentPositions(piece.Position)) { if (neightbour == LastPosition) { return true; } }

        return false;
    }
    private bool CanPull(Piece piece)
    {
        if(Pushing) { return false; }
        foreach (Piece neightbour in _grid.GetAdjacent(piece.Position))
        {
            if (neightbour == null) { continue; }
            if (piece.IsStronger(neightbour) && piece.IsGold != neightbour.IsGold) { return true; }
        }
        return false;
    }
    private bool IsEmptyPosition(Vector2Int position) { return _grid[position] == null; }
    public bool IsTrapPosition(Vector2Int position)
    {
        if (position == new Vector2Int(2, 2)) { return true; }
        if (position == new Vector2Int(5, 5)) { return true; }
        if (position == new Vector2Int(2, 5)) { return true; }
        if (position == new Vector2Int(5, 2)) { return true; }

        return false;
    }
    private bool IsStopPosition(Vector2Int position, Piece piece) { return !IsFriendlyPositionForPiece(position, piece) && (IsTrapPosition(position) || IsFrozenPositionForPiece(position, piece)); }
    private bool IsFrozenPositionForPiece(Vector2Int position, Piece piece)
    {
        Piece current;

        current = _grid[position + Vector2Int.up];
        if (current != null && current.IsStronger(piece) && !current.IsTrapped) { return true; }
        current = _grid[position + Vector2Int.down];
        if (current != null && current.IsStronger(piece) && !current.IsTrapped) { return true; }
        current = _grid[position + Vector2Int.left];
        if (current != null && current.IsStronger(piece) && !current.IsTrapped) { return true; }
        current = _grid[position + Vector2Int.right];
        if (current != null && current.IsStronger(piece) && !current.IsTrapped) { return true; }

        return false;
    }
    private bool IsFriendlyPositionForPiece(Vector2Int position, Piece piece)
    {
        Piece current;

        current = _grid[position + Vector2Int.up];
        if (current != null && current.IsFriend(piece)) { return true; }
        current = _grid[position + Vector2Int.down];
        if (current != null && current.IsFriend(piece)) { return true; }
        current = _grid[position + Vector2Int.left];
        if (current != null && current.IsFriend(piece)) { return true; }
        current = _grid[position + Vector2Int.right];
        if (current != null && current.IsFriend(piece)) { return true; }

        return false;
    }
    private bool IsTrapPositionForPiece(Vector2Int position, Piece piece) { return !IsFriendlyPositionForPiece(position, piece) && IsTrapPosition(position); }

    private bool CannotPieceMove(Piece piece) 
    {
        if (piece.IsFrozen) { return true; }
        foreach (Piece neightbour in _grid.GetAdjacent(piece.Position)) 
        {
            if (neightbour == null) { return false; } 
            else if(piece.IsStronger(neightbour))
            {
                foreach (Piece pushPosition in _grid.GetAdjacent(neightbour.Position))
                {
                    if (pushPosition == null) { return false; }
                }
            }
        }
        return true;
    }
    public bool CannotMoveAnyPiece() 
    {
        bool result = true;
        foreach (Piece piece in Pieces)
        {
            if (piece.IsGold != IsGoldsTurn) { continue; }
            if (CannotPieceMove(piece)) { continue; }

            result = false;
        }
        return result;
    }

    private int GetTurn(char record) { return int.Parse(record.ToString()); }
    private bool GetPlayer(char record)
    {
        bool player = true;

        if (record == 'g') { player = true; }
        else if (record == 's') { player = false; }

        return player;
    }
    private int GetStrength(char record)
    {
        int strength = 0;

        if (record == 'r') { strength = 1; }
        else if (record == 'c') { strength = 2; }
        else if (record == 'd') { strength = 3; }
        else if (record == 'h') { strength = 4; }
        else if (record == 'm') { strength = 5; }
        else if (record == 'e') { strength = 6; }

        return strength;
    }
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

    public void Print(object target) { MonoBehaviour.print(target); }

    public bool ContainsSavedGame(List<BoardState> savedGame) 
    {
        if (_history.Count < savedGame.Count) { return false; }
        for (int i = 0; i < _history.Count; i++) { if (_history[i].Moves != savedGame[i].Moves) { return false; } }
        return true;
    }
}
