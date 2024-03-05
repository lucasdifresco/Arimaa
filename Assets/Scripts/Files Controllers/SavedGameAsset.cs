using System.IO;
using UnityEngine;
using System.Collections.Generic;
using System;
using System.Linq;
using TMPro;

[CreateAssetMenu(fileName ="Saved Game Asset",menuName ="Arimaa/Saved Game Asset")]
public class SavedGameAsset : ScriptableObject
{
#if UNITY_EDITOR
    [SerializeField] private bool _debug;
#endif

    [SerializeField] private string _folderName = "FileData";
    [SerializeField] private List<TextAsset> _variants;

    private string _defaultFilename = "";
    private SavedGame _savedGame;
    private string _recordedGame = "";

    public List<BoardState> Current { get { return _savedGame.History; } private set { _savedGame.History = value; } }
    public bool HasLoadedGame { get; private set; }
    public bool HasRecordedGame { get; private set; }
    public bool HasVariantGame { get; private set; }
    public string FileName { get { return _defaultFilename; } }

    private string Filename { get { return _defaultFilename + ".json"; } }
    private string DataPath
    {
        get
        {
            string datapath = "";
#if UNITY_EDITOR
            datapath = Application.dataPath;
#else
                datapath = Application.persistentDataPath;
#endif
            return datapath;
        }
    }
    private string DirectoryPath
    {
        get
        {
            string folderPath = Path.Combine(DataPath, _folderName);
            if (!Directory.Exists(folderPath)) { Directory.CreateDirectory(folderPath); }
            return folderPath;
        }
    }
    private string FilePath { get { return Path.Combine(DirectoryPath, Filename); } }
    private bool IsSaved { get { return File.Exists(FilePath); } }

    public void CopyBufferToLabel(TMP_Text movesLabel) 
    {
        movesLabel.text = GUIUtility.systemCopyBuffer;
    }
    public void LoadRecordedGame(TMP_Text movesLabel) 
    {
        HasRecordedGame = true;
        _recordedGame = movesLabel.text;
    }
    public void LoadVariantGame(TMP_Dropdown variantDropdown)
    {
        if (variantDropdown.value == 0) { return; }
        HasVariantGame = true;
        _recordedGame = _variants[variantDropdown.value - 1].text;
    }
    public string GetRecordedGame() { return _recordedGame; }
    public void Save(List<BoardState> history, string filename)
    {
        _defaultFilename = filename;
        _savedGame = new SavedGame(history);
        
        string json = _savedGame.ToJSON();
        
        if (!Directory.Exists(DirectoryPath)) { Directory.CreateDirectory(DirectoryPath); }
        File.WriteAllText(FilePath, json);

#if UNITY_EDITOR
        if (_debug) { MonoBehaviour.print("Settings Saved:" + json); }
#endif
    }
    public void Load(string filename)
    {
        _defaultFilename = filename;
        if (!IsSaved)
        {
#if UNITY_EDITOR
            if (_debug) { MonoBehaviour.print("Cant' load: file does not exist"); }
#endif
            _defaultFilename = "";
            return;
        }

        string json = File.ReadAllText(FilePath);
        _savedGame = new SavedGame(json);
        HasLoadedGame = true;

#if UNITY_EDITOR
        if (_debug) { MonoBehaviour.print("Settings Loaded:" + json); }
#endif
    }
    public void Unload() 
    {
        _savedGame = null;
        _recordedGame = "";
        HasLoadedGame = false;
        HasRecordedGame = false;
        HasVariantGame = false;
    }
    public string[] GetAllSavedFiles() 
    {
        return Directory
            .GetFiles(DirectoryPath)
            .Where((path) => { return Path.GetExtension(path) == ".json" && Path.GetFileNameWithoutExtension(path) != "settings"; })
            .ToArray(); 
    }
    public void DeleteSavedFile(string filename) 
    {
        _defaultFilename = filename;
        if (!IsSaved)
        {
#if UNITY_EDITOR
            if (_debug) { MonoBehaviour.print("Cant' delete: file does not exist"); }
#endif
            _defaultFilename = "";
            return;
        }

        File.Delete(FilePath);
        _defaultFilename = "";
    }

    [Serializable] public class SavedGame 
    {
        public List<BoardState> History;

        public SavedGame(List<BoardState> history) { History = history; }
        public SavedGame(string json) 
        {
            SavedGame game = JsonUtility.FromJson<SavedGame>(json);
            History = game.History;
        }

        public string ToJSON() { return JsonUtility.ToJson(this); }
    }
}
