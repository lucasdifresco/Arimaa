using System;
using System.Collections.Generic;
using UnityEngine;

public class PathFinding
{
    private const int MOVE_COST = 10;

    private Grid<PathNode> _grid;
    private List<PathNode> _openList;
    private List<PathNode> _closeList;

    public PathFinding(int width, int height) { _grid = new Grid<PathNode>(width, height, (x, y, grid) => { return new PathNode(x, y, false, grid); }); }

    public void SetLegalMoves(List<Vector2Int> legalMoves)
    {
        _grid.Do((x, y, node) => { node.IsEnabled = false; });
        foreach (Vector2Int move in legalMoves) { _grid[move].IsEnabled = true; }
    }
    public void SetLegalMoves(Vector2Int origin, List<Vector2Int> legalMoves) 
    {
        _grid.Do((x, y, node) => { node.IsEnabled = false; });
        _grid[origin].IsEnabled = true;
        foreach(Vector2Int move in legalMoves) { _grid[move].IsEnabled = true; } 
    }

    public void DoGrid(Action<int, int> action) { _grid.Do((x, y, node) => { if (node.IsEnabled) { action?.Invoke(x, y); } }); }

    public List<Vector2Int> FindPath(Vector2Int origin, Vector2Int target) 
    {
        PathNode startNode = _grid.GetItem(origin);
        PathNode endNode = _grid.GetItem(target);

        _openList = new List<PathNode>() { startNode };
        _closeList = new List<PathNode>();

        _grid.Do((x, y, node) => { node.ResetNode(); });

        startNode.gCost = 0;
        startNode.hCost = CalculateDistanceCost(startNode, endNode);

        int count = 0;
        while (_openList.Count > 0) 
        {
            count++;
            PathNode currentNode = GetLowestFCostNode(_openList);
            if (currentNode == endNode) { return CalculatePath(endNode); }

            _openList.Remove(currentNode);
            _closeList.Add(currentNode);

            foreach (PathNode neightbour in _grid.GetAdjacent(currentNode.Position)) 
            {
                if (_closeList.Contains(neightbour)) { continue; }
                if (!neightbour.IsEnabled) { _closeList.Add(neightbour); continue; }

                int tentativeGCost = currentNode.gCost + CalculateDistanceCost(currentNode, neightbour);
                if(tentativeGCost < neightbour.gCost) 
                {
                    neightbour.cameFromNode = currentNode;
                    neightbour.gCost = tentativeGCost;
                    neightbour.hCost = CalculateDistanceCost(neightbour, endNode);
                    if (!_openList.Contains(neightbour)) { _openList.Add(neightbour); }                    
                }
            }
        }

        return null;
    }

    private List<Vector2Int> CalculatePath(PathNode endNode) 
    {
        List<Vector2Int> path = new List<Vector2Int>() { endNode.Position };
        PathNode current = endNode;
        while (current.cameFromNode != null) 
        {
            path.Add(current.cameFromNode.Position);
            current = current.cameFromNode;
        }
        path.Reverse();
        return path; 
    }
    private int CalculateDistanceCost(PathNode origin, PathNode target) 
    {
        int xDistance = Mathf.Abs(origin.X - target.X);
        int yDistance = Mathf.Abs(origin.Y - target.Y);
        return MOVE_COST * Mathf.Abs(xDistance - yDistance);
    }
    private PathNode GetLowestFCostNode(List<PathNode> pathNodeList) 
    {
        PathNode lowestFCostNode = pathNodeList[0];
        for (int i = 1; i < pathNodeList.Count; i++) { if (pathNodeList[i].fCost < lowestFCostNode.fCost) { lowestFCostNode = pathNodeList[i]; } }
        return lowestFCostNode;
    } 
}
