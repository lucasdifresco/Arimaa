using System;
using System.Collections.Generic;
using UnityEngine;

[Serializable] public class BoardState
{
    public List<Piece> Pieces;
    public int Turn = 1;
    public bool IsGoldsTurn = true;
    public int StepsLeft = 4;
    public bool Pushing = false;
    public bool Pulling = false;
    public bool EndOfTurn = false;
    public string Moves = "1g ";
    public string State = "";
    public Piece LastPiece;
    public Vector2Int LastPosition;

    public bool GameEnded = false;
    public bool IsGoldWiner = false;
    public bool WinByElimination = false;
    public bool WinByInmobilization = false;
    public bool WinByRepetition = false;

    public BoardState() { Pieces = new List<Piece>(); }
    public BoardState(BoardState state)
    {
        Pieces = new List<Piece>();
        foreach (Piece piece in state.Pieces) { Pieces.Add(piece.Copy()); }
        UpdateState(state);
    }
    public BoardState Copy() { return new BoardState(this); }

    public void SetState(BoardState state)
    {
        for (int i = 0; i < Pieces.Count; i++) { Pieces[i].SetPiece(state.Pieces[i]); }
        UpdateState(state);
    }

    public string ToJSON() { return JsonUtility.ToJson(this); }

    private void UpdateState(BoardState state) 
    {
        Turn = state.Turn;
        IsGoldsTurn = state.IsGoldsTurn;
        StepsLeft = state.StepsLeft;
        Pushing = state.Pushing;
        Pulling = state.Pulling;
        EndOfTurn = state.EndOfTurn;
        Moves = state.Moves;
        State = state.State;
        LastPiece = state.LastPiece;
        LastPosition = state.LastPosition;

        GameEnded = state.GameEnded;
        IsGoldWiner = state.IsGoldWiner;
        WinByElimination = state.WinByElimination;
        WinByInmobilization = state.WinByInmobilization;
        WinByRepetition = state.WinByRepetition;
    }
}
