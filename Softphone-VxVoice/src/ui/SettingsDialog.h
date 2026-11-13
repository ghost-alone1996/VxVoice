#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include "SettingsManager.h"

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

private slots:
    void onBrowseRecordingPath();
    void onOpenRecordingFolder();
    void onSaveClicked();
    void onToggleRingtonePreview();
    void onTestAudio();
    void onCheckGitUpdates();
    void onApplyGitUpdate();

private:
    void setupUi();
    QWidget* createGeneralTab();
    QWidget* createAudioTab();
    QWidget* createCodecsTab();
    QWidget* createRecordingTab();
    QWidget* createAboutTab();

    void loadSettings();
    void saveSettings();
    void refreshAudioDeviceLists();

    QTabWidget *m_tabs{nullptr};

    // Aba Sobre & Atualização
    QLabel *m_lblUpdateStatus{nullptr};
    QPushButton *m_btnCheckUpdates{nullptr};
    QPushButton *m_btnApplyUpdate{nullptr};

    // Aba Geral
    QCheckBox *m_chkStartWithSystem{nullptr};
    QCheckBox *m_chkStartMinimized{nullptr};
    QCheckBox *m_chkMinimizeToTrayOnClose{nullptr};
    QComboBox *m_comboRingtone{nullptr};
    QPushButton *m_btnPlayRingtone{nullptr};
    QComboBox *m_comboTheme{nullptr};

    // Aba Áudio e Voz
    QComboBox *m_comboInputDevice{nullptr};
    QComboBox *m_comboOutputDevice{nullptr};
    QPushButton *m_btnRefreshDevices{nullptr};
    QSlider *m_sliderInputVolume{nullptr};
    QSlider *m_sliderOutputVolume{nullptr};
    QLabel *m_lblInputVolVal{nullptr};
    QLabel *m_lblOutputVolVal{nullptr};
    QCheckBox *m_chkEchoCancellation{nullptr};
    QCheckBox *m_chkAutoGainControl{nullptr};
    QCheckBox *m_chkVoiceActivityDetection{nullptr};
    QPushButton *m_btnTestAudio{nullptr};

    // Aba Codecs
    QListWidget *m_codecsListWidget{nullptr};

    // Aba Gravação
    QLineEdit *m_recordingPathEdit{nullptr};
    QPushButton *m_btnBrowseRecordPath{nullptr};
    QPushButton *m_btnOpenRecordFolder{nullptr};
    QComboBox *m_comboRecordFormat{nullptr};
    QCheckBox *m_chkAutoRecord{nullptr};

    QPushButton *m_btnSave{nullptr};
    QPushButton *m_btnCancel{nullptr};

    bool m_isPreviewPlaying{false};
};

#endif // SETTINGSDIALOG_H
