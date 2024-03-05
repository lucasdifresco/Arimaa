using UnityEngine;

public class PathNode
{
    private bool _isEnabled;
    private int _x;
    private int _y;
    private Grid<PathNode> _grid;

    public PathNode(int x, int y, bool isEnabled, Grid<PathNode> grid)
    {
        _x = x;
        _y = y;
        _grid = grid;
        _isEnabled = true;
    }
    
    public bool IsEnabled { get { return _isEnabled; } set { _isEnabled = value; } }
    public int X { get { return _x; } }
    public int Y { get { return _y; } }
    public Vector2Int Position { get { return new Vector2Int(X, Y); } }
    
    public int gCost;
    public int hCost;
    public PathNode cameFromNode;
    public int fCost { get { return gCost + hCost; } }

    public void ResetNode() 
    {
        cameFromNode = null;
        gCost = int.MaxValue;
    }
}
