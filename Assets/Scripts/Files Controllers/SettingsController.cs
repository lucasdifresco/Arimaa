using System;
using System.IO;
using TMPro;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

public class SettingsController : MonoBehaviour
{
#if UNITY_EDITOR
    [SerializeField] private bool _debug;
#endif

    [SerializeField] private string _folderName = "FileData";
    [SerializeField] private string _filename = "settings";
    [SerializeField] private bool _dontSubscribePlayerSettings;
    [SerializeField] private bool _dontSubscribeSettings;
    [SerializeField] private bool _allowVariant;

    [SerializeField] private Toggle _soundOn;
    [SerializeField] private TMP_Dropdown _animationSpeed;
    [SerializeField] private Toggle _flip;
    [SerializeField] private Toggle _enlarge;
    [SerializeField] private Toggle _swipe;
    [SerializeField] private TMP_Dropdown _boardGraphics;
    [SerializeField] private TMP_Dropdown _pieceGraphics;

    [SerializeField] private TMP_Dropdown _goldMode;
    [SerializeField] private TMP_Dropdown _goldLevel;
    [SerializeField] private TMP_Dropdown _silverMode;
    [SerializeField] private TMP_Dropdown _silverLevel;

    public UnityEvent OnSave;
    public UnityEvent OnLoad;

    private Settings _settings = null;

    public Settings Current { get { return _settings; } private set { _settings = value; } }
    private string Filename { get { return _filename + ".json"; } }
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

    private void OnEnable()
    {
        Load();
    }
    private void Start()
    {
        _soundOn.onValueChanged.AddListener((value) => { Save(); });
        
        if (_dontSubscribeSettings) { return; }

        _animationSpeed.onValueChanged.AddListener((value) => { Save(); });
        _flip.onValueChanged.AddListener((value) => { Save(); });
        _enlarge.onValueChanged.AddListener((value) => { Save(); });
        _swipe.onValueChanged.AddListener((value) => { Save(); });
        _boardGraphics.onValueChanged.AddListener((value) => { Save(); });
        _pieceGraphics.onValueChanged.AddListener((value) => { Save(); });

        if (_dontSubscribePlayerSettings) { return; }

        _goldMode.onValueChanged.AddListener((value) => { Save(); });
        _goldLevel.onValueChanged.AddListener((value) => { Save(); });
        _silverMode.onValueChanged.AddListener((value) => { Save(); });
        _silverLevel.onValueChanged.AddListener((value) => { Save(); });
    }

    public void UpdateFlipSetting()
    {
        if (Current.GoldMode == 0 && Current.SilverMode == 0) { _flip.isOn = true; }
        else { _flip.isOn = false; }

        Save();
    }
    public void SetHvH()
    {
        _goldMode.value = 0;
        _silverMode.value = 0;

        Save();
    }

    public void Save()
    {
        if (Current == null)
        {
            Current = new Settings(
            (_soundOn == null) ? false : _soundOn.isOn,
            (_animationSpeed == null) ? 1 : _animationSpeed.value,
            (_flip == null) ? false : _flip.isOn,
            (_enlarge == null) ? false : _enlarge.isOn,
            (_swipe == null) ? false : _swipe.isOn,
            (_boardGraphics == null) ? 0 : _boardGraphics.value,
            (_pieceGraphics == null) ? 0 : _pieceGraphics.value,
            (_goldMode == null) ? 0 : _goldMode.value,
            (_goldLevel == null) ? 0 : _goldLevel.value,
            (_silverMode == null) ? 0 : _silverMode.value,
            (_silverLevel == null) ? 0 : _silverLevel.value
            );
        }
        else 
        {        
            Current = new Settings(
                (_soundOn == null) ? Current.SoundOn : _soundOn.isOn,
                (_animationSpeed == null) ? Current.AnimationSpeed : _animationSpeed.value,
                (_flip == null) ? Current.Flip : _flip.isOn,
                (_enlarge == null) ? Current.Enlarge : _enlarge.isOn,
                (_swipe == null) ? Current.Swipe : _swipe.isOn,
                (_boardGraphics == null) ? Current.Board : _boardGraphics.value,
                (_pieceGraphics == null) ? Current.Piece : _pieceGraphics.value,
                (_goldMode == null) ? Current.GoldMode : _goldMode.value,
                (_goldLevel == null) ? Current.GoldLevel : _goldLevel.value,
                (_silverMode == null) ? Current.SilverMode : _silverMode.value,
                (_silverLevel == null) ? Current.SilverLevel : _silverLevel.value
                );
        }

        string json = Current.ToJSON();
        if (!Directory.Exists(DirectoryPath)) { Directory.CreateDirectory(DirectoryPath); }
        File.WriteAllText(FilePath, json);

        OnSave?.Invoke();

#if UNITY_EDITOR
        if (_debug) { print("Settings Saved:" + json); }
#endif
    }
    public void Load()
    {
        if (!IsSaved)
        {
#if UNITY_EDITOR
            if (_debug) { print("Cant' load settings: file does not exist"); }
#endif

            Save();
        }

        string json = File.ReadAllText(FilePath);
        Current = new Settings(json);

        if (_soundOn != null) { _soundOn.isOn = Current.SoundOn; }
        if (_animationSpeed != null) { _animationSpeed.value = Current.AnimationSpeed; }
        if (_flip != null) { _flip.isOn = Current.Flip; }
        if (_enlarge != null) { _enlarge.isOn = Current.Enlarge; }
        if (_swipe != null) { _swipe.isOn = Current.Swipe; }
        if (_boardGraphics != null) { _boardGraphics.value = Current.Board; }
        if (_pieceGraphics != null) { _pieceGraphics.value = Current.Piece; }

        if (_goldMode != null) { _goldMode.value = Current.GoldMode; }
        if (_goldLevel != null) { _goldLevel.value = Current.GoldLevel; }
        if (_silverMode != null) { _silverMode.value = Current.SilverMode; }
        if (_silverLevel != null) { _silverLevel.value = Current.SilverLevel; }

        OnLoad?.Invoke();

#if UNITY_EDITOR
        if (_debug) { print("Settings Loaded:" + json); }
#endif
    }

    [Serializable] public class Settings 
    {
        public bool SoundOn;
        public int AnimationSpeed;
        public bool Flip;
        public bool Enlarge;
        public bool Swipe;
        public int Board;
        public int Piece;

        public int GoldMode;
        public int GoldLevel;
        public int SilverMode;
        public int SilverLevel;

        public Settings(string json) 
        {
            Settings settings = JsonUtility.FromJson<Settings>(json);

            SoundOn = settings.SoundOn;
            AnimationSpeed = settings.AnimationSpeed;
            Flip = settings.Flip;
            Enlarge = settings.Enlarge;
            Swipe = settings.Swipe;
            Board = settings.Board;
            Piece = settings.Piece;

            GoldMode = settings.GoldMode;
            GoldLevel = settings.GoldLevel;
            SilverMode = settings.SilverMode;
            SilverLevel = settings.SilverLevel;
        }
        public Settings(bool soundOn, int animationSpeed, bool flip, bool enlarge, bool swipe, int board, int piece, int goldMode, int goldLevel, int silverMode, int silverLevel)
        {
            SoundOn = soundOn;
            AnimationSpeed = animationSpeed;
            Flip = flip;
            Enlarge = enlarge;
            Swipe = swipe;
            Board = board;
            Piece = piece;
            
            GoldMode = goldMode;
            GoldLevel = goldLevel;
            SilverMode = silverMode;
            SilverLevel = silverLevel;
        }

        public string ToJSON() { return JsonUtility.ToJson(this); }
    }
}
