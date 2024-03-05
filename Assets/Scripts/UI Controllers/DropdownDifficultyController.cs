using TMPro;
using UnityEngine;

public class DropdownDifficultyController : MonoBehaviour
{
    [SerializeField] private int indexToEnable;
    [SerializeField] private TMP_Dropdown _mode;
    [SerializeField] private GameObject _Level;

    public void CheckDropdown() 
    {
        if (_mode.value == indexToEnable) { _Level.SetActive(true); }
        else { _Level.SetActive(false); }
    }
}
