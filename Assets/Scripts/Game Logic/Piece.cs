using System;
using UnityEngine;

[Serializable] public class Piece
{
    [SerializeField] private bool _isGold;
    [SerializeField] private bool _isTrapped = false;
    [SerializeField] private bool _isFrozen = false;
    [SerializeField] private bool _active = true;
    [SerializeField] private int _strength = 1;
    [SerializeField] private Vector2Int _position;
    [SerializeField] private Vector2Int _trapPosition;

    public bool IsGold { get { return _isGold; } private set { _isGold = value; } }
    public bool IsFrozen { get { return _isFrozen; } set { _isFrozen = value; } }
    public bool IsTrapped { get { return _isTrapped; } set { _isTrapped = value; } }
    public bool IsActive { get { return _active; } set { _active = value; } }
    public int Strength { get { return _strength; } set { _strength = value; } }
    public bool IsRabbit { get { return Strength == 1; } }

    public bool IsFriend(Piece piece) { return piece.IsGold == IsGold && piece != this; }
    public bool IsStronger(Piece piece) { return (piece.IsGold != IsGold && piece.Strength < Strength); }

    public Vector2Int Position { get { return _position; } private set { _position = value; } }
    public Vector2Int TrapPosition { get { return _trapPosition; } private set { _trapPosition = value; } }
    public bool CanMoveNorth { get { return !(IsRabbit && !IsGold); } }
    public bool CanMoveSouth { get { return !(IsRabbit && IsGold); } }
    public string PositionLabel
    {
        get
        {
            string label = "";

            if (Strength == 1) { label += (IsGold)?"R":"r"; }
            else if (Strength == 2) { label += (IsGold) ? "C" : "c"; }
            else if (Strength == 3) { label += (IsGold) ? "D" : "d"; }
            else if (Strength == 4) { label += (IsGold) ? "H" : "h"; }
            else if (Strength == 5) { label += (IsGold) ? "M" : "m"; }
            else if (Strength == 6) { label += (IsGold) ? "E" : "e"; }

            if (Position.x == 0) { label += "a"; }
            else if (Position.x == 1) { label += "b"; }
            else if (Position.x == 2) { label += "c"; }
            else if (Position.x == 3) { label += "d"; }
            else if (Position.x == 4) { label += "e"; }
            else if (Position.x == 5) { label += "f"; }
            else if (Position.x == 6) { label += "g"; }
            else if (Position.x == 7) { label += "h"; }

            label += (Position.y + 1);
            return label;
        }
    }
    public string NameLabel
    {
        get
        {
            string label = "";

            if (Strength == 1) { label += ((IsGold) ? "Gold" : "Silver") + "Rabbit"; }
            else if (Strength == 2) { label += ((IsGold) ? "Gold" : "Silver") + "Cat"; }
            else if (Strength == 3) { label += ((IsGold) ? "Gold" : "Silver") + "Dog"; }
            else if (Strength == 4) { label += ((IsGold) ? "Gold" : "Silver") + "Horse"; }
            else if (Strength == 5) { label += ((IsGold) ? "Gold" : "Silver") + "Camel"; }
            else if (Strength == 6) { label += ((IsGold) ? "Gold" : "Silver") + "Elephant"; }

            return label;
        }
    }
    public string StateLabel
    {
        get
        {
            string label = "";

            if (Strength == 1) { label += "R"; }
            else if (Strength == 2) { label += "C"; }
            else if (Strength == 3) { label += "D"; }
            else if (Strength == 4) { label += "H"; }
            else if (Strength == 5) { label += "M"; }
            else if (Strength == 6) { label += "E"; }

            return label;
        }
    }

    public Piece(Piece piece) { SetPiece(piece); }
    public Piece(int strength, bool isGold)
    {
        Strength = strength;
        IsGold = isGold;
    }
    public Piece(int strength, bool isGold, Vector2Int position)
    {
        Strength = strength;
        IsGold = isGold;
        Position = position;
    }
    public Piece(int strength, bool isGold, bool isTrapped)
    {
        Strength = strength;
        IsGold = isGold;
        IsTrapped = isTrapped;
    }

    public Piece Copy() { return new Piece(this); }
    public void SetPiece(Piece piece) 
    {
        _isGold = piece._isGold;
        _isTrapped = piece._isTrapped;
        _isFrozen = piece._isFrozen;
        _active = piece._active;
        _strength = piece._strength;
        _position = piece._position;
        _trapPosition = piece._trapPosition;
    }

    public void MoveTo(Vector2Int position) { Position = position; }
    public void UpdateTrapPosition() { _trapPosition = Position; }
}
