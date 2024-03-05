using UnityEngine;
using UnityEngine.Events;
using UnityEngine.SceneManagement;

public class SceneController : MonoBehaviour
{
    [SerializeField] private UnityEvent OnAwake;
    private void Awake() { OnAwake?.Invoke(); }
    public void ReloadScene() { SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex); }
    public void LoadScene(int buildIndex) { SceneManager.LoadScene(buildIndex); }
    public void AddScene(int buildIndex) 
    {
        if (IsSceneLoaded(buildIndex)) { print("scene is loaded"); return; }
        SceneManager.LoadSceneAsync(buildIndex, LoadSceneMode.Additive); 
    }
    public void CloseScene(int buildIndex) 
    {
        if (!IsSceneLoaded(buildIndex)) { return; }
        SceneManager.UnloadSceneAsync(buildIndex); 
    }

    public void ExitApp() { Application.Quit(); }
    public void OpenLink(string link) { Application.OpenURL(link); }
    private bool IsSceneLoaded(int buildIndex) { return SceneManager.GetSceneByBuildIndex(buildIndex).isLoaded; }
}