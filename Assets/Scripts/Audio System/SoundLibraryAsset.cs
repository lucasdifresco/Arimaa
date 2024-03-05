using System;
using System.Collections.Generic;
using UnityEngine;

[CreateAssetMenu(menuName = "Arimaa/Sound Library Asset")]
public class SoundLibraryAsset : ScriptableObject
{
    [Range(0f, 2f)] public float Volume = 1f;
    [Range(0.1f, 3f)] public float Pitch = 1f;
    public List<Sound> Library;
}

