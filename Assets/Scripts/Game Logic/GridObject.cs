using System;
using System.Collections.Generic;
using UnityEngine;

public class Grid<T>
{
    private int _columns;
    private int _rows;
    private T[,] _grid;
    private event Action<int, int, T> _onValueChanged;
    private event Action<int, int, T> _onPrint;

    public int Columns { get { return _columns; } }
    public int Rows { get { return _rows; } }
    public int Count { get { return _columns * _rows; } }

    public T this[int i, int j] { get { return GetItem(i, j); } set { SetItem(i, j, value); } }
    public T this[Vector2Int position] { get { return GetItem(position); } set { SetItem(position, value); } }

    public Grid(int columns, int rows, Func<int, int, Grid<T>, T> defaultObject)
    {
        _columns = columns;
        _rows = rows;
        _grid = new T[columns, rows];
        Do((x, y, obj) => { SetItem(x, y, defaultObject(x, y, this)); });
    }

    public void Print(Color color = default(Color), bool drawBorderLines = true, bool drawInsideLines = true, float duration = 0f)
    {
        Vector3 bottomLeft = Vector3.zero;
        Vector3 topLeft = Vector3.zero;
        Vector3 bottomRight = Vector3.zero;
        Vector3 topRight = Vector3.zero;

        Do((x, y, obj) =>
        {
            bottomLeft = Vector3.zero;
            topLeft = bottomLeft + Vector3.up;
            topRight = topLeft + Vector3.right;
            bottomRight = topRight - Vector3.up;

            Debug.DrawLine(bottomLeft, topLeft, color, duration);
            Debug.DrawLine(topLeft, topRight, color, duration);
            Debug.DrawLine(topRight, bottomRight, color, duration);
            Debug.DrawLine(bottomRight, bottomLeft, color, duration);
            _onPrint?.Invoke(x, y, obj);
        });
    }
    public Grid<T> Do(Action<int, int, T> action)
    {
        for (int x = 0; x < _grid.GetLength(0); x++) { for (int y = 0; y < _grid.GetLength(1); y++) { action?.Invoke(x, y, _grid[x, y]); } }
        return this;
    }

    public void SetItem(int x, int y, T value)
    {
        if (RangeCheck(x, y))
        {
            _grid[x, y] = value;
            _onValueChanged?.Invoke(x, y, GetItem(x, y));
        }
    }
    public void SetItem(Vector2Int position, T value) { SetItem(position.x, position.y, value); }

    public T GetItem(int x, int y)
    {
        if (RangeCheck(x, y)) { return _grid[x, y]; }
        else { return default(T); }
    }
    public T GetItem(Vector2Int position) { return GetItem(position.x, position.y); }

    public List<T> GetAdjacent(int x, int y)
    {
        List<T> neighbours = new List<T>();

        if (RightCheck(x + 1)) { neighbours.Add(GetItem(x + 1, y)); }
        if (LeftCheck(x - 1)) { neighbours.Add(GetItem(x - 1, y)); }
        if (UpCheck(y + 1)) { neighbours.Add(GetItem(x, y + 1)); }
        if (DownCheck(y - 1)) { neighbours.Add(GetItem(x, y - 1)); }

        return neighbours;
    }
    public List<T> GetAdjacent(Vector2Int position) { return GetAdjacent(position.x, position.y); }
    public List<Vector2Int> GetAdjacentPositions(int x, int y)
    {
        List<Vector2Int> neighbours = new List<Vector2Int>();

        if (RightCheck(x + 1)) { neighbours.Add(new Vector2Int(x + 1, y)); }
        if (LeftCheck(x - 1)) { neighbours.Add(new Vector2Int(x - 1, y)); }
        if (UpCheck(y + 1)) { neighbours.Add(new Vector2Int(x, y + 1)); }
        if (DownCheck(y - 1)) { neighbours.Add(new Vector2Int(x, y - 1)); }

        return neighbours;
    }
    public List<Vector2Int> GetAdjacentPositions(Vector2Int position) { return GetAdjacentPositions(position.x, position.y); }

    public List<T> GetNeighbours(int x, int y)
    {
        List<T> neighbours = new List<T>();

        if (RightCheck(x + 1)) { neighbours.Add(GetItem(x + 1, y)); }
        if (LeftCheck(x - 1)) { neighbours.Add(GetItem(x - 1, y)); }
        if (UpCheck(y + 1)) { neighbours.Add(GetItem(x, y + 1)); }
        if (DownCheck(y - 1)) { neighbours.Add(GetItem(x, y - 1)); }
        if (RightCheck(x + 1) && UpCheck(y + 1)) { neighbours.Add(GetItem(x + 1, y + 1)); }
        if (RightCheck(x + 1) && DownCheck(y - 1)) { neighbours.Add(GetItem(x + 1, y - 1)); }
        if (LeftCheck(x - 1) && UpCheck(y + 1)) { neighbours.Add(GetItem(x - 1, y + 1)); }
        if (LeftCheck(x - 1) && DownCheck(y - 1)) { neighbours.Add(GetItem(x - 1, y - 1)); }

        return neighbours;
    }
    public List<T> GetNeighbours(Vector2Int position) { return GetNeighbours(position.x, position.y); }

    public Vector2Int GetRelativePosition(int originX, int originY, int targetX, int targetY) { return new Vector2Int(targetX - originX, targetY - originY); }
    public Vector2Int GetRelativePosition(Vector2Int origin, Vector2Int target) { return GetRelativePosition(origin.x, origin.y, target.x, target.y); }

    public int GetNumberPosition(int x, int y)
    {
        if (RangeCheck(x, y)) { return (_rows * x) + y; }
        else { return -1; }
    }
    public int GetNumberPosition(Vector2Int position) { return GetNumberPosition(position.x, position.y); }

    public Vector2Int BoundToGrid(int x, int y)
    {
        while (x >= _columns) { x -= _columns; }
        while (x < 0) { x += _columns; }
        while (y >= _rows) { y -= _rows; }
        while (y < 0) { y += _rows; }
        return new Vector2Int(x, y);
    }
    public Vector2Int BoundToGrid(Vector2Int position) { return BoundToGrid(position.x, position.y); }

    public Grid<T> OnValueChange(Action<int, int, T> onValueChanged) { _onValueChanged += onValueChanged; return this; }
    public Grid<T> OnPrint(Action<int, int, T> onPrint) { _onPrint += onPrint; return this; }

    public bool RangeCheck(Vector2Int position) { return (HCheck(position.x) && VCheck(position.y)); }
    public bool RangeCheck(int x, int y) { return (HCheck(x) && VCheck(y)); }
    public bool HCheck(int x) { return (LeftCheck(x) && RightCheck(x)); }
    public bool RightCheck(int x) { return (x < _columns); }
    public bool LeftCheck(int x) { return (x >= 0); }
    public bool VCheck(int y) { return (DownCheck(y) && UpCheck(y)); }
    public bool UpCheck(int y) { return (y < _rows); }
    public bool DownCheck(int y) { return (y >= 0); }
}
