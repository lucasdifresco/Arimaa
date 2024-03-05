using System.IO;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class GameLoader : MonoBehaviour
{
    [SerializeField] private SavedGameAsset _saveAsset;
    [SerializeField] private GameObject _loadButtonPrefab;
    [SerializeField] private int _sceneToLoad = 2;
    [SerializeField] private SceneController _sceneController;

    private void OnEnable() { UpdateButtons(); }

    private void UpdateButtons() 
    {
        int childCount = transform.childCount;
        for (int i = childCount - 1; i >= 0; i--) { Destroy(transform.GetChild(i).gameObject); }

        string[] files = _saveAsset.GetAllSavedFiles();

        foreach (string file in files)
        {
            if (file == "Settings") { continue; }
            CreateNewButton(Path.GetFileNameWithoutExtension(file));
        }
    }

    private void CreateNewButton(string filename) 
    {
        Button button = Instantiate(_loadButtonPrefab, transform).GetComponent<Button>();
        button.onClick.AddListener(() => 
        {
            _saveAsset.Load(filename);
            _sceneController.LoadScene(_sceneToLoad);
        });
        button.GetComponentInChildren<TMP_Text>().text = filename;
        button.transform.GetChild(1).GetComponent<Button>().onClick.AddListener(() => 
        {
            _saveAsset.DeleteSavedFile(filename); 
            UpdateButtons();
        });
    }
}
