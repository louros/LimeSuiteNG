#ifndef LIME_SDR_CONFIGURATION_VIEW
#define LIME_SDR_CONFIGURATION_VIEW

#include <wx/panel.h>
#include "commonWxForwardDeclarations.h"
#include <map>

#include "ISOCPanel.h"
#include "limesuiteng/types.h"
#include "limesuiteng/SDRDevice.h"

constexpr int MAX_GUI_CHANNELS_COUNT = 2;

struct ChannelConfigGUI {
    wxCheckBox* enable;
    wxChoice* path;
    wxChoice* gain;
    wxChoice* gainValues;
    wxTextCtrl* lpf;
    wxTextCtrl* nco;
};

struct SDRConfigGUI {
    wxStaticBox* titledBox;
    wxTextCtrl* rxLO;
    wxTextCtrl* txLO;
    wxCheckBox* tdd;
    wxTextCtrl* sampleRate;
    wxChoice* decimation;
    wxChoice* interpolation;
    ChannelConfigGUI rx[MAX_GUI_CHANNELS_COUNT];
    ChannelConfigGUI tx[MAX_GUI_CHANNELS_COUNT];

    std::map<int, lime::eGainTypes> rxSelectionToValue;
    std::map<int, lime::eGainTypes> txSelectionToValue;
};

class SOCConfig_view : public wxPanel
{
  public:
    SOCConfig_view(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL);
    void Setup(lime::SDRDevice* sdrDevice, int index);
    void UpdateGainValues(const wxCommandEvent& event);

    void SubmitConfig(const wxCommandEvent& event);

  protected:
    SDRConfigGUI gui;
    lime::SDRDevice* sdrDevice{};
    int socIndex;

  private:
    void UpdateGain(const wxCommandEvent& event, const ChannelConfigGUI& channelGui, lime::TRXDir direction);
};

class SDRConfiguration_view : public ISOCPanel
{
  public:
    SDRConfiguration_view(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL);
    void Setup(lime::SDRDevice* device);

  protected:
    SDRConfiguration_view() = delete;
    std::vector<SOCConfig_view*> socGUI;
    lime::SDRDevice* sdrDevice;
    wxFlexGridSizer* mainSizer;
};

#endif // LIME_SDR_CONFIGURATION_VIEW
