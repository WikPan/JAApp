#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/statbmp.h>
#include <wx/valnum.h>
#include <wx/listctrl.h>
#include <fstream>
#include <cmath>
#include <vector>
#include <chrono>
#include <thread>
#include <windows.h>
#include <memory>
#include <immintrin.h>


// Typ funkcji gamma z DLL
typedef void (*GammaCorrectionFunc)(unsigned char* imageData, int pixelCount, unsigned char* LUT);

class MyApp : public wxApp {
public:
    virtual bool OnInit();
};

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);

private:
    wxTextCtrl* gammaInput;
    wxChoice* libraryChoice;
    wxChoice* threadCountChoice;
    wxStaticText* timeOutput;
    wxStaticBitmap* imagePreview;
    wxListCtrl* timeList;
    wxString currentImagePath;
    int imageWidth, imageHeight;

    void OnSelectFile(wxCommandEvent& event);
    void OnProcessImage(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);

    bool LoadBMP(const wxString& filename, unsigned char*& imageData, int& width, int& height);
    bool SaveBMP(const wxString& filename, unsigned char* imageData, int width, int height);
    void GenerateLUT(float gamma, unsigned char* LUT);
    void DisplayImage(const wxString& filename);
    void ProcessImageInThreads(unsigned char* imageData, int pixelCount, unsigned char* LUT, int threadCount, GammaCorrectionFunc gammaCorrectionFunc);

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_BUTTON(1001, MainFrame::OnSelectFile)
EVT_BUTTON(1002, MainFrame::OnProcessImage)
EVT_BUTTON(wxID_EXIT, MainFrame::OnExit)
wxEND_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit() {
    MainFrame* frame = new MainFrame("Gamma Correction App");
    frame->Show(true);
    return true;
}

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1000, 800)),
    imageWidth(0), imageHeight(0) {

    wxPanel* panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* fileSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* fileButton = new wxButton(panel, 1001, "Select Image");
    fileSizer->Add(fileButton, 0, wxALL, 5);
    sizer->Add(fileSizer, 0, wxALIGN_CENTER);

    wxBoxSizer* gammaSizer = new wxBoxSizer(wxHORIZONTAL);
    gammaSizer->Add(new wxStaticText(panel, wxID_ANY, "Gamma Value:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gammaInput = new wxTextCtrl(panel, wxID_ANY, "2.2", wxDefaultPosition, wxDefaultSize, 0, wxFloatingPointValidator<float>());
    gammaSizer->Add(gammaInput, 0, wxALL, 5);
    sizer->Add(gammaSizer, 0, wxALIGN_CENTER);

    wxBoxSizer* librarySizer = new wxBoxSizer(wxHORIZONTAL);
    librarySizer->Add(new wxStaticText(panel, wxID_ANY, "Library:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    libraryChoice = new wxChoice(panel, wxID_ANY);
    libraryChoice->Append("Assembler");
    libraryChoice->Append("C");
    libraryChoice->SetSelection(0);
    librarySizer->Add(libraryChoice, 0, wxALL, 5);
    sizer->Add(librarySizer, 0, wxALIGN_CENTER);

    wxBoxSizer* threadSizer = new wxBoxSizer(wxHORIZONTAL);
    threadSizer->Add(new wxStaticText(panel, wxID_ANY, "Threads:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    threadCountChoice = new wxChoice(panel, wxID_ANY);
    threadSizer->Add(threadCountChoice, 0, wxALL, 5);

    int numThreads = std::thread::hardware_concurrency();
    for (int i = 1; i <= 64; i *= 2) {
        threadCountChoice->Append(wxString::Format("%d", i));
    }
    threadCountChoice->SetSelection(0);

    sizer->Add(threadSizer, 0, wxALIGN_CENTER);

    wxButton* processButton = new wxButton(panel, 1002, "Process Image");
    sizer->Add(processButton, 0, wxALL | wxALIGN_CENTER, 5);

    timeOutput = new wxStaticText(panel, wxID_ANY, "Processing time: ", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    sizer->Add(timeOutput, 0, wxALL, 5);

    wxBoxSizer* timeListSizer = new wxBoxSizer(wxHORIZONTAL);
    timeList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(400, 300), wxLC_REPORT);
    timeList->InsertColumn(0, "Threads", wxLIST_FORMAT_LEFT, 80);
    timeList->InsertColumn(1, "Time (s)", wxLIST_FORMAT_LEFT, 100);
    timeList->InsertColumn(2, "Gamma", wxLIST_FORMAT_LEFT, 100);
    timeListSizer->Add(timeList, 1, wxEXPAND | wxALL, 5);
    sizer->Add(timeListSizer, 0, wxALIGN_CENTER);

    imagePreview = new wxStaticBitmap(panel, wxID_ANY, wxBitmap(), wxDefaultPosition, wxSize(256, 256));
    sizer->Add(imagePreview, 0, wxALL | wxALIGN_CENTER, 5);

    wxButton* exitButton = new wxButton(panel, wxID_EXIT, "Exit");
    sizer->Add(exitButton, 0, wxALL | wxALIGN_CENTER, 5);

    panel->SetSizer(sizer);
}

void MainFrame::OnSelectFile(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, "Open BMP file", "", "", "BMP files (*.bmp)|*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL) return;

    currentImagePath = openFileDialog.GetPath();
    DisplayImage(currentImagePath);
}

void MainFrame::OnProcessImage(wxCommandEvent& event) {
    if (currentImagePath.IsEmpty()) {
        wxMessageBox("No image loaded!", "Error", wxICON_ERROR);
        return;
    }

    double gamma;
    if (!gammaInput->GetValue().ToDouble(&gamma) || gamma <= 0) {
        wxMessageBox("Invalid gamma value!", "Error", wxICON_ERROR);
        return;
    }

    unsigned char LUT[256];
    GenerateLUT(gamma, LUT);

    int choice = libraryChoice->GetSelection();
    HMODULE hDll = nullptr;
    GammaCorrectionFunc gammaCorrectionFunc = nullptr;

    if (choice == 0) {
        hDll = LoadLibrary(L"JADll.dll");
        if (hDll) gammaCorrectionFunc = (GammaCorrectionFunc)GetProcAddress(hDll, "ApplyGammaCorrectionASM");
    }
    else if (choice == 1) {
        hDll = LoadLibrary(L"ApplyGammaCorrection_C.dll");
        if (hDll) gammaCorrectionFunc = (GammaCorrectionFunc)GetProcAddress(hDll, "ApplyGammaCorrection_C");
    }

    if (!hDll || !gammaCorrectionFunc) {
        wxMessageBox("Failed to load library or function!", "Error", wxICON_ERROR);
        if (hDll) FreeLibrary(hDll);
        return;
    }

    double totalElapsed = 0.0;
    for (int i = 0; i < 5; ++i) {
        unsigned char* rawImageData = nullptr;
        if (!LoadBMP(currentImagePath, rawImageData, imageWidth, imageHeight)) {
            wxMessageBox("Failed to reload image!", "Error", wxICON_ERROR);
            FreeLibrary(hDll);
            return;
        }

        int pixelCount = imageWidth * imageHeight;

        auto start = std::chrono::high_resolution_clock::now();

        ProcessImageInThreads(rawImageData, pixelCount, LUT, std::stoi(threadCountChoice->GetStringSelection().ToStdString()), gammaCorrectionFunc);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        totalElapsed += elapsed.count();

        delete[] rawImageData;
    }

    double averageTime = totalElapsed / 5.0;

    wxString timeText;
    timeText.Printf("Average processing time: %.4f seconds", averageTime);
    timeOutput->SetLabel(timeText);

    long index = timeList->InsertItem(timeList->GetItemCount(), threadCountChoice->GetStringSelection());
    timeList->SetItem(index, 1, wxString::Format("%.4f", averageTime));
    timeList->SetItem(index, 2, wxString::Format("%.2f", gamma));

    unsigned char* finalImageData = nullptr;
    if (!LoadBMP(currentImagePath, finalImageData, imageWidth, imageHeight)) {
        wxMessageBox("Failed to reload image for saving!", "Error", wxICON_ERROR);
        FreeLibrary(hDll);
        return;
    }

    int pixelCount = imageWidth * imageHeight;
    ProcessImageInThreads(finalImageData, pixelCount, LUT, std::stoi(threadCountChoice->GetStringSelection().ToStdString()), gammaCorrectionFunc);

    wxString outputFilename = wxFileName(currentImagePath).GetPath() + "/output.bmp";
    if (!SaveBMP(outputFilename, finalImageData, imageWidth, imageHeight)) {
        wxMessageBox("Failed to save image!", "Error", wxICON_ERROR);
        delete[] finalImageData;
        FreeLibrary(hDll);
        return;
    }

    delete[] finalImageData;
    DisplayImage(outputFilename);
    FreeLibrary(hDll);
}



void MainFrame::OnExit(wxCommandEvent& event) {
    Close(true);
}

void MainFrame::ProcessImageInThreads(unsigned char* imageData, int pixelCount, unsigned char* LUT, int threadCount, GammaCorrectionFunc gammaCorrectionFunc) {
    int chunkSize = pixelCount / threadCount;
    std::vector<std::thread> threads;

    for (int i = 0; i < threadCount; ++i) {
        int startIdx = i * chunkSize;
        int endIdx = (i == threadCount - 1) ? pixelCount : startIdx + chunkSize;
        threads.emplace_back([=]() {
            gammaCorrectionFunc(imageData + startIdx * 3, endIdx - startIdx, LUT);
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

bool MainFrame::LoadBMP(const wxString& filename, unsigned char*& imageData, int& width, int& height) {
    std::ifstream file(filename.ToStdString(), std::ios::binary);
    if (!file) return false;

    unsigned char header[54];
    file.read(reinterpret_cast<char*>(header), 54);

    width = *reinterpret_cast<int*>(&header[18]);
    height = *reinterpret_cast<int*>(&header[22]);

    int imageSize = 3 * width * height;
    imageData = new unsigned char[imageSize];
    file.read(reinterpret_cast<char*>(imageData), imageSize);
    file.close();
    return true;
}

bool MainFrame::SaveBMP(const wxString& filename, unsigned char* imageData, int width, int height) {
    std::ofstream file(filename.ToStdString(), std::ios::binary);
    if (!file) return false;

    unsigned char header[54] = {
        0x42, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0,
        40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0
    };

    int fileSize = 54 + 3 * width * height;
    int imageSize = 3 * width * height;

    std::memcpy(&header[2], &fileSize, 4);
    std::memcpy(&header[18], &width, 4);
    std::memcpy(&header[22], &height, 4);
    std::memcpy(&header[34], &imageSize, 4);

    file.write(reinterpret_cast<char*>(header), 54);
    file.write(reinterpret_cast<char*>(imageData), imageSize);
    file.close();
    return true;
}

void MainFrame::GenerateLUT(float gamma, unsigned char* LUT) {
    const int vecSize = 8; // AVX przetwarza 8 wartoœci float jednoczeœnie

    // Przygotowanie wektorowego gamma i innych sta³ych
    __m256 gammaVec = _mm256_set1_ps(gamma);
    __m256 multiplier = _mm256_set1_ps(255.0f);
    __m256 half = _mm256_set1_ps(0.5f);

    for (int i = 0; i < 256; i += vecSize) {
        // Za³aduj normalizowane wartoœci jako wektor
        __m256 values = _mm256_set_ps(
            (i + 7) / 255.0f, (i + 6) / 255.0f, (i + 5) / 255.0f, (i + 4) / 255.0f,
            (i + 3) / 255.0f, (i + 2) / 255.0f, (i + 1) / 255.0f, i / 255.0f
        );


        __m256 powered = _mm256_pow_ps(values, gammaVec); // Funkcja AVX dla potêgowania (mo¿e wymagaæ dodatkowej biblioteki)

 
        __m256 result = _mm256_fmadd_ps(powered, multiplier, half);

        __m256i intResult = _mm256_cvtps_epi32(result);

        int lutValues[vecSize];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(lutValues), intResult);

        for (int j = 0; j < vecSize; ++j) {
            LUT[i + j] = static_cast<unsigned char>(lutValues[j]);
        }
    }
}

void MainFrame::DisplayImage(const wxString& filename) {
    wxImage image;
    if (!image.LoadFile(filename, wxBITMAP_TYPE_BMP)) {
        wxMessageBox("Failed to load image for display!", "Error", wxICON_ERROR);
        return;
    }
    image.Rescale(256, 256);
    imagePreview->SetBitmap(wxBitmap(image));
    Layout();
}
