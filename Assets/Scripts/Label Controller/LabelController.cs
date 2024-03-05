using TMPro;
using UnityEngine;


[RequireComponent(typeof(TMP_Text))]
public class LabelController : MonoBehaviour
{
    [SerializeField] private string _genericText = "#";
    [SerializeField] private string _replaceChar = "#";
    [SerializeField] private float _numberMultiplier = 1f;
    [SerializeField] private string _numberFormat = "F0";

    private TMP_Text _label;
    private string _text;

    private string Label 
    {
        get { return _text; } 
        set 
        {
            _text = value;
            if (_label != null) { _label.text = _text; }
        }
    }

    private void OnEnable()
    {
        if (_label == null) { _label = GetComponent<TMP_Text>(); }
        Label = _text;
    }

    public void SetLabel(int amount) 
    {
        amount *= (int)_numberMultiplier;
        Label = _genericText.Replace(_replaceChar, amount.ToString(_numberFormat));
    }
    public void SetLabel(float amount)
    {
        amount *= _numberMultiplier;
        Label = _genericText.Replace(_replaceChar, amount.ToString(_numberFormat));
    }
    public void SetLabel(string text)
    {
        Label = _genericText.Replace(_replaceChar, text);
    }
}

