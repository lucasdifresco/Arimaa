using UnityEngine;
using UnityEngine.UI;

public class VolumeIconController : MonoBehaviour
{
    [SerializeField] private SpritesAsset _spritesAsset;
    [SerializeField] private SettingsController _settings;
    [SerializeField] private SoundMasterAsset _soundMaster;

    private Image _image;

    private void Awake() { _image = GetComponent<Image>(); }
    private void Start() { UpdateSprite(); }
    private void OnEnable()
    {
        _settings.OnSave.AddListener(UpdateSprite);
        _settings.OnLoad.AddListener(UpdateSprite);
    }
    private void OnDisable()
    {
        _settings.OnSave.RemoveListener(UpdateSprite);
        _settings.OnLoad.RemoveListener(UpdateSprite);
    }
    public void UpdateSprite()
    {
        if(_settings.Current == null) { return; }
        if (_settings.Current.SoundOn) 
        {
            _image.sprite = _spritesAsset.VolumeHigh;
            _soundMaster.Unmute();
        }
        else
        {
            _image.sprite = _spritesAsset.VolumeMuted;
            _soundMaster.Mute();
        }
    }

}
