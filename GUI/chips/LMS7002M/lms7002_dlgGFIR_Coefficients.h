#ifndef __lms7002_dlgGFIR_Coefficients__
#define __lms7002_dlgGFIR_Coefficients__

#include "lms7002_wxgui.h"
#include "limesuiteng/types.h"

#include <cstdint>
#include <vector>

namespace lime {
class LMS7002M;
}

class lms7002_dlgGFIR_Coefficients : public wxDialog
{
  protected:
    // Handlers for dlgGFIR_Coefficients events.
    void OnLoadFromFile(wxCommandEvent& event);
    void OnSaveToFile(wxCommandEvent& event);
    void OnClearTable(wxCommandEvent& event);
    void OnspinCoefCountChange(wxSpinEvent& event);
    void OnBtnOkClick(wxCommandEvent& event);
    void OnBtnCancelClick(wxCommandEvent& event);

  public:
    lms7002_dlgGFIR_Coefficients(wxWindow* parent, wxWindowID id, const wxString& title);
    void SetCoefficients(const std::vector<double>& coefficients);
    std::vector<double> GetCoefficients();

    int ReadCoefficients(lime::TRXDir direction, uint8_t gfirIndex, lime::LMS7002M* lmsControl);
    void WriteCoefficients(lime::TRXDir direction, uint8_t gfirIndex, lime::LMS7002M* lmsControl);

    wxButton* btnLoadFromFile;
    wxButton* btnSaveToFile;
    wxButton* btnClearTable;
    wxStaticText* ID_STATICTEXT3;
    wxSpinCtrl* spinCoefCount;
    wxGrid* gridCoef;
    wxButton* btnOk;
    wxButton* btnCancel;
};

#endif // __lms7002_dlgGFIR_Coefficients__
