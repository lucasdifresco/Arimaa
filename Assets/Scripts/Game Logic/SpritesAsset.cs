using System.Collections.Generic;
using UnityEngine;

[CreateAssetMenu(fileName = "Sprite Asset", menuName = "Arimaa/Sprites Asset")]
public class SpritesAsset : ScriptableObject
{
    public Sprite VolumeHigh;
    public Sprite VolumeMuted;

    public Color GoldColor;
    public Color SilverColor;

    public List<Sprite> Boards;
    
    public List<Sprite> WhiteSprites;
    public List<Sprite> GoldSprites;
    public List<Sprite> SilverSprites;
}
